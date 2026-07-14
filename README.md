# TinyML Gesture Recognition — ESP32

Low-level IMU interface, full 6-axis calibrated data acquisition, orientation estimation, and an on-device dataset collector built as the foundation for a TinyML motion classification project on an ESP32 DOIT DevKit V1.

> **Status:** Phase 4 complete — dataset collector firmware operational, multi-environment build structure in place. Zero training data recorded yet; that's the current step.

---

## Overview

This project establishes a direct I2C communication layer between an ESP32 and a commercially labeled MPU-6050 IMU module, **without relying on vendor libraries**. Hardware characterization revealed that the installed sensor is not a genuine MPU-6050 (WHO_AM_I = `0x72` instead of `0x68`), but is a pin-compatible replacement with an identical register layout.

Full 6-axis acquisition (accelerometer + gyroscope), offset calibration, scale conversion to physical units (m/s² and °/s), orientation estimation via complementary filter, and an on-device windowed feature extractor for dataset collection have all been validated.

The end goal is a gesture-recognition bracelet: an ESP32 + IMU classifies three arm gestures (shake, rotate, flick up) with fully on-device TinyML inference and drives independent LED feedback per gesture.

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 DOIT DevKit V1 |
| IMU (labeled) | MPU-6050 module |
| IMU (actual) | Unknown InvenSense/TDK-compatible — WHO_AM_I: `0x72` |
| Interface | I2C |
| I2C Address | `0x68` |
| OLED (planned) | SSD1306 — hardware defective, integration deferred |

### Wiring

| ESP32 Pin | IMU Pin |
|---|---|
| 3V3 | VCC |
| GND | GND |
| GPIO21 | SDA |
| GPIO22 | SCL |

---

## Project Structure

Each independent firmware lives in its own subdirectory of `src/` and builds as its own PlatformIO environment via `build_src_filter` — this keeps the main application, the I2C diagnostic tool, and the (currently blocked) OLED bring-up test from ever cross-contaminating a build.

```
├── src/
│   ├── gesture-recognition/   # Main firmware
│   │   ├── main.cpp           # Init and read loop orchestration
│   │   ├── imu.cpp            # IMU hardware logic
│   │   ├── filter.cpp         # Complementary filter implementation
│   │   └── collector.cpp      # Session-based dataset collector + feature extraction
│   ├── i2c_scan/
│   │   └── i2c_scan.cpp       # Standalone I2C bus diagnostic
│   └── ssd1306_test/
│       └── ssd1306_test.cpp   # OLED bring-up test (blocked — hardware defective)
├── include/
│   ├── imu.h                  # IMU public interface
│   ├── filter.h                # Filter public interface and Orientation struct
│   └── collector.h            # Collector public interface, window/sample-rate constants
├── docs/
│   ├── TR-001.pdf              # Technical Report 001
│   └── TR-001.md                # Technical Report 001 (Markdown)
├── platformio.ini
└── README.md
```

---

## Setup

**Requirements:**
- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- ESP32 DOIT DevKit V1
- IMU module wired as described above

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
5. Continuously reads calibrated AX, AY, AZ (m/s²), GX, GY, GZ (°/s), and estimates Roll/Pitch (°) at 50 Hz.
6. Checks for serial dataset-collector commands and, when a session is active, streams labeled feature windows over serial.

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

---

## Complementary Filter

Orientation is estimated by fusing accelerometer and gyroscope data:

```
orientation = alpha * gyroscope_integration + (1 - alpha) * accelerometer_angle
```

With alpha = 0.98:
- The gyroscope contributes 98% — smooth, responsive to motion.
- The accelerometer contributes 2% — corrects gyroscope drift over time.

Roll and pitch are estimated in degrees. Yaw is excluded (requires magnetometer). Orientation is used for the rule-based flick-up gesture only — it is never fed into the ML feature set.

**Verified at rest:** Roll/Pitch within ±0.35°.

---

## Key Findings

- The sensor responds at I2C address `0x68` but returns WHO_AM_I = `0x72` — not a genuine MPU-6050.
- ACCEL_CONFIG and GYRO_CONFIG default to `0x00`: ±2g and ±250°/s ranges confirmed.
- Scale factors: 16384 LSB/g (accelerometer), 131 LSB/°/s (gyroscope).
- Calibrated output at rest: AX ≈ AY ≈ 0 m/s², AZ ≈ 9.81 m/s², GX ≈ GY ≈ GZ ≈ 0 °/s.
- Complementary filter noise at rest: ±0.35°.
- Gesture signals are expected well above the stationary noise floor (accelerometer ±0.07 m/s², gyroscope ±0.35°/s), supporting class separability for the shake/rotate/idle classifier.

Full methodology in [`docs/TR-001.md`](docs/TR-001.md).

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
- [ ] Training data collection (shake/rotate/idle, ≥200 windows/class)
- [ ] Exploratory data analysis and feature normalization
- [ ] TinyML model training (Dense 64→32→3)
- [ ] Quantization (int8 TFLite) and on-device inference
- [ ] Post-inference gesture-state logic (toggle / double-toggle / continuous)
- [ ] OLED display integration (pending replacement hardware; build environment ready)

---

## Documentation

| Document | Description |
|---|---|
| [TR-001](docs/TR-001.md) | Hardware characterization, firmware modularization, 6-axis acquisition, complementary filter, dataset collector, and build system restructuring — Phases 1–4 |

---

## Author

**Eric Zuriel Zaldivar Peñaflor** — 2183082002
Universidad Autónoma Metropolitana, Unidad Azcapotzalco
Division of Basic Sciences and Engineering
Microelectronics — Prof. José Luis Zamorano Flores
