# TinyML Gesture Recognition — ESP32

Low-level IMU interface, full 6-axis calibrated data acquisition, orientation estimation, on-device dataset collector, and a quantized TinyML model running real-time gesture inference — all on an ESP32 DOIT DevKit V1.

> **Status:** Phase 5 complete — on-device inference running continuously alongside the dataset collector, with confirmed LED feedback per gesture. See [Known Issues](#known-issues) for the current classification boundary between `idle` and `shake`.

---

## Overview

This project establishes a direct I2C communication layer between an ESP32 and a commercially labeled MPU-6050 IMU module, **without relying on vendor libraries**. Hardware characterization revealed that the installed sensor is not a genuine MPU-6050 (WHO_AM_I = `0x72` instead of `0x68`), but is a pin-compatible replacement with an identical register layout.

Full 6-axis acquisition (accelerometer + gyroscope), offset calibration, scale conversion to physical units (m/s² and °/s), orientation estimation via complementary filter, an on-device windowed feature extractor, and a trained TinyML classifier running real-time int8 inference have all been validated.

The end goal is a gesture-recognition bracelet: an ESP32 + IMU classifies three arm gestures (**shake**, **rotate**, **idle**) via an on-device neural network, plus a fourth gesture (**flick_up**, rule-based, not yet implemented) and drives independent LED feedback per gesture.

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 DOIT DevKit V1 |
| IMU (labeled) | MPU-6050 module |
| IMU (actual) | Unknown InvenSense/TDK-compatible — WHO_AM_I: `0x72` |
| Interface | I2C |
| I2C Address | `0x68` |
| LED feedback | 2x LED, GPIO4 (shake) / GPIO23 (rotate); GPIO33 reserved for `flick_up` |
| OLED (planned) | SSD1306 — hardware defective, integration deferred |

### IMU Wiring

| ESP32 Pin | IMU Pin |
|---|---|
| 3V3 | VCC |
| GND | GND |
| GPIO21 | SDA |
| GPIO22 | SCL |

### LED Wiring

| ESP32 Pin | Gesture | Notes |
|---|---|---|
| GPIO4 | shake | 220–330Ω resistor in series to GND |
| GPIO23 | rotate | 220–330Ω resistor in series to GND |
| GPIO33 | *(reserved for `flick_up`)* | not yet wired |

**Ground note:** LED failures were originally traced to a bad ground connection taken from the breadboard rail shared with the IMU (the partially failed rails already noted below), not a GPIO or firmware issue — resolved with a direct jumper to the ESP32's own GND pin. GPIO25/GPIO26 remain electrically valid alternatives if you rewire; GPIO4/GPIO23 are simply where the LEDs ended up during debugging.

---

## Project Structure

Each independent firmware lives in its own subdirectory of `src/` and builds as its own PlatformIO environment via `build_src_filter` — this keeps the main application, the I2C diagnostic tool, and the (currently blocked) OLED bring-up test from ever cross-contaminating a build.

```
├── src/
│   ├── gesture-recognition/    # Main firmware
│   │   ├── main.cpp            # Init and read loop orchestration
│   │   ├── imu.cpp             # IMU hardware logic
│   │   ├── filter.cpp          # Complementary filter implementation
│   │   ├── collector.cpp       # Session-based dataset collector + feature extraction
│   │   └── inference.cpp       # On-device TFLite Micro inference
│   ├── i2c_scan/
│   │   └── i2c_scan.cpp        # Standalone I2C bus diagnostic
│   └── ssd1306_test/
│       └── ssd1306_test.cpp    # OLED bring-up test (blocked — hardware defective)
├── include/
│   ├── imu.h                   # IMU public interface
│   ├── filter.h                 # Filter public interface and Orientation struct
│   ├── collector.h             # Collector public interface, window/sample-rate constants
│   ├── inference.h             # Inference module public interface
│   ├── model_data.h            # Quantized model as C array (synced from ml/models/)
│   └── scaler_params.h         # Feature normalization params (synced from ml/models/)
├── ml/                          # Data capture, training, and quantization pipeline
│   ├── capture_dataset.py      # Serial-based session capture
│   ├── eda.py                   # Class separability + normalization analysis
│   ├── train_model.py          # Trains, quantizes, and exports the model
│   ├── data/                    # Captured CSVs (shake.csv, rotate.csv, idle.csv)
│   └── models/                  # gesture_model.tflite, model_data.h, scaler_params.h
├── docs/
│   ├── TR-001.pdf / TR-001.md            # Technical Report 001 (English)
│   └── TR-001_ES.docx / TR-001_ES.md      # Technical Report 001 (Spanish)
├── platformio.ini
└── README.md
```

---

## Setup

**Requirements:**
- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- ESP32 DOIT DevKit V1
- IMU module wired as described above
- For the `ml/` pipeline: `pip install pyserial pandas matplotlib scikit-learn tensorflow`

**Build and flash (main gesture-recognition firmware):**

```bash
pio run -e gesture-recognition --target upload
```

This is also the default environment — `pio run --target upload` with no `-e` builds this one.

**Run I2C bus scan:**

```bash
pio run -e i2c-scan --target upload
```

**OLED bring-up test** (blocked pending replacement hardware):

```bash
pio run -e ssd1306-test --target upload
```

**Monitor serial output:**

```bash
pio device monitor
```

Baud rate: `115200`

---

## Firmware Description

On boot, the main (`gesture-recognition`) firmware:

1. Initializes I2C on GPIO21 (SDA) and GPIO22 (SCL).
2. Wakes the sensor by writing `0x00` to the power management register (`0x6B`).
3. Runs a 200-sample offset calibration routine — **keep sensor still during this phase**.
4. Initializes the complementary filter with alpha = 0.98.
5. Initializes the TFLite Micro interpreter (`inference_init()`).
6. Continuously reads calibrated AX, AY, AZ (m/s²), GX, GY, GZ (°/s), and estimates Roll/Pitch (°) at 50 Hz.
7. Checks for serial dataset-collector commands and, when a session is active, streams labeled feature windows over serial.
8. When **not** capturing a labeled session, feeds each sample to the inference module and drives the corresponding LED when a new classification is available.

No external IMU library is used. All register access is handled directly via the Arduino `Wire` library.

### Dataset Collector

The collector runs continuously alongside the main sensing loop and is controlled via single-character serial commands:

| Command | Action |
|---|---|
| `s` | Start/continue a SHAKE-labeled recording session |
| `r` | Start/continue a ROTATE-labeled recording session |
| `i` | Start/continue an IDLE-labeled recording session |
| `x` | Stop the current session |
| `h` | Reprint the CSV header |

Each completed 1-second window (50 samples at 50 Hz) is reduced on-device to 12 features — mean and RMS of all six axes — and emitted as a CSV row: `label,ax,ay,az,gx,gy,gz,ax_rms,ay_rms,az_rms,gx_rms,gy_rms,gz_rms`. Gestures must be recognizable regardless of arm orientation, so absolute Roll/Pitch is intentionally excluded from the feature set. Full design rationale in [`docs/TR-001.md`](docs/TR-001.md).

Captured dataset: **365 windows** (120 shake, 125 rotate, 120 idle), across 5 arm orientations × 5 sessions per class.

### On-Device Inference

`inference.cpp` mirrors the collector's windowing logic but runs continuously whenever `collector_is_recording()` is false, so live inference and labeled dataset capture never share the same sample stream.

- Model: `Dense(64, relu) → Dense(32, relu) → Dense(3, softmax)`, 3,011 parameters.
- Quantized post-training to full int8 (8,200 bytes) via the TFLite Converter.
- Runtime: [`johnosbb/MicroTFLite`](https://github.com/johnosbb/MicroTFLite) — the official `espressif/esp-tflite-micro` failed to build under PlatformIO (its `flatbuffers`/`gemmlowp` dependencies aren't vendored outside the ESP-IDF component manager); MicroTFLite packages everything needed and also handles int8 quantization/dequantization internally.
- Tensor arena: 8,192 bytes.
- Validation accuracy: 100% (73/73) — **with a known caveat**: the train/validation split was done at the window level, not the session level, which likely inflates this figure due to intra-session autocorrelation. See `docs/TR-001.md` §16.4 for the full discussion.
- `inference_has_new_result()` gates the main loop so a classification is only acted on once per completed window (~1s), instead of being re-read on every ~20ms loop iteration for the whole window duration.

---

## Complementary Filter

Orientation is estimated by fusing accelerometer and gyroscope data:

```
orientation = alpha * gyroscope_integration + (1 - alpha) * accelerometer_angle
```

With alpha = 0.98:
- The gyroscope contributes 98% — smooth, responsive to motion.
- The accelerometer contributes 2% — corrects gyroscope drift over time.

Roll and pitch are estimated in degrees. Yaw is excluded (requires magnetometer). Orientation is used for the rule-based flick-up gesture only (not yet implemented) — it is never fed into the ML feature set.

**Verified at rest:** Roll/Pitch within ±0.35°.

---

## Key Findings

- The sensor responds at I2C address `0x68` but returns WHO_AM_I = `0x72` — not a genuine MPU-6050.
- ACCEL_CONFIG and GYRO_CONFIG default to `0x00`: ±2g and ±250°/s ranges confirmed.
- Calibrated output at rest: AX ≈ AY ≈ 0 m/s², AZ ≈ 9.81 m/s², GX ≈ GY ≈ GZ ≈ 0 °/s.
- Complementary filter noise at rest: ±0.35°.
- Class separability confirmed via EDA: `shake`, `rotate`, `idle` form visually disjoint clusters in feature space, with `gy_rms` as the single most discriminative feature.
- LED failures traced to a shared bad ground on the breadboard, not GPIO/firmware — see [LED Wiring](#led-wiring) above.

Full methodology in [`docs/TR-001.md`](docs/TR-001.md) and [`docs/TR-001_ES.docx`](docs/TR-001_ES.docx).

---

## Known Issues

- **`idle` vs `shake` boundary:** normal, non-gesture arm movement is occasionally classified as `shake`. Root cause: `idle` was only captured as static, stationary poses across 5 orientations — never ambient low-level motion. This is a dataset coverage gap, not a firmware defect.
- **`flick_up`** is designed (rule-based pitch threshold) but not implemented; GPIO33 is reserved and unwired.
- **Validation split** is window-level, not session-level — see [On-Device Inference](#on-device-inference) above.
- `include/model_data.h` and `include/scaler_params.h` are manually synced copies of `ml/models/*.h` — must be re-copied by hand after any retraining.

---

## Roadmap

- [x] I2C communication validated
- [x] Register read/write confirmed
- [x] Sensor wake-up sequence implemented
- [x] Raw accelerometer data streaming
- [x] Gyroscope register characterization
- [x] Full 6-axis data acquisition
- [x] Offset calibration
- [x] Scale conversion to physical units (m/s², °/s)
- [x] Firmware modularization (imu.h / imu.cpp)
- [x] I2C diagnostic tool (src/i2c_scan)
- [x] Digital filtering (complementary filter)
- [x] Roll/pitch orientation estimation
- [x] Dataset collector firmware (session-based, on-device feature extraction)
- [x] Multi-environment build structure (build_src_filter isolation)
- [x] Training data collection (365 windows: shake/rotate/idle)
- [x] Exploratory data analysis and feature normalization
- [x] TinyML model training (Dense 64→32→3) and int8 quantization
- [x] On-device inference integration and tensor arena sizing
- [x] LED feedback confirmed working (GPIO4/GPIO23)
- [ ] Expand `idle` dataset with ambient, non-gesture motion
- [ ] Minimum magnitude/velocity threshold before triggering `shake`
- [ ] Session-level train/validation split
- [ ] `flick_up` rule-based implementation (GPIO33)
- [ ] OLED display integration (pending replacement hardware; build environment ready)

---

## Documentation

| Document | Description |
|---|---|
| [TR-001.md](docs/TR-001.md) / [TR-001.pdf](docs/TR-001.pdf) | Full technical report (English) — hardware characterization through Phase 5 (TinyML training and on-device inference) |
| [TR-001_ES.docx](docs/TR-001_ES.docx) / [TR-001_ES.md](docs/TR-001_ES.md) | Full technical report (Spanish) |

---

## Author

**Eric Zuriel Zaldivar Peñaflor** — 2183082002
Universidad Autónoma Metropolitana, Unidad Azcapotzalco
Division of Basic Sciences and Engineering
Microelectronics — Prof. José Luis Zamorano Flores
