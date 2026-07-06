# TinyML IMU Data Acquisition — ESP32

Low-level IMU interface, full 6-axis calibrated data acquisition, and orientation estimation pipeline on an ESP32 DOIT DevKit V1, built as the foundation for a TinyML motion classification project.

> **Status:** Phase 3 complete — complementary filter implemented, roll/pitch estimation operational.

---

## Overview

This project establishes a direct I2C communication layer between an ESP32 and a commercially labeled MPU-6050 IMU module, **without relying on vendor libraries**. Hardware characterization revealed that the installed sensor is not a genuine MPU-6050 (WHO_AM_I = `0x72` instead of `0x68`), but is a pin-compatible replacement with an identical register layout.

Full 6-axis acquisition (accelerometer + gyroscope), offset calibration, scale conversion to physical units (m/s² and °/s), and orientation estimation via complementary filter have been validated. The firmware is modularized into separate hardware and application layers.

The end goal is on-device motion inference using TinyML models trained on datasets collected through this pipeline.

---

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 DOIT DevKit V1 |
| IMU (labeled) | MPU-6050 module |
| IMU (actual) | Unknown InvenSense/TDK-compatible — WHO_AM_I: `0x72` |
| Interface | I2C |
| I2C Address | `0x68` |

### Wiring

| ESP32 Pin | IMU Pin |
|---|---|
| 3V3 | VCC |
| GND | GND |
| GPIO21 | SDA |
| GPIO22 | SCL |

---

## Project Structure

```
├── src/
│   ├── main.cpp          # Main firmware — init and read loop
│   ├── imu.cpp           # IMU hardware logic
│   └── filter.cpp        # Complementary filter implementation
├── include/
│   ├── imu.h             # IMU public interface
│   └── filter.h          # Filter public interface and Orientation struct
├── tools/
│   └── i2c_scan.cpp      # Standalone I2C bus diagnostic
├── docs/
│   ├── TR-001.pdf        # Technical Report 001
│   └── TR-001.md         # Technical Report 001 (Markdown)
├── platformio.ini
└── README.md
```

---

## Setup

**Requirements:**
- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- ESP32 DOIT DevKit V1
- IMU module wired as described above

**Build and flash (main firmware):**

```bash
pio run -e esp32doit-devkit-v1 --target upload
```

**Run I2C bus scan:**

```bash
pio run -e i2c-scan --target upload
```

**Monitor serial output:**

```bash
pio device monitor
```

Baud rate: `115200`

---

## Firmware Description

On boot, the firmware:

1. Initializes I2C on GPIO21 (SDA) and GPIO22 (SCL).
2. Wakes the sensor by writing `0x00` to the power management register (`0x6B`).
3. Reads and prints identification and configuration registers (WHO_AM_I, ACCEL_CONFIG, GYRO_CONFIG).
4. Runs a 200-sample offset calibration routine — **keep sensor still during this phase**.
5. Initializes the complementary filter with alpha = 0.98.
6. Continuously reads calibrated AX, AY, AZ (m/s²), GX, GY, GZ (°/s), and estimated Roll/Pitch (°) at ~5 Hz.

No external IMU library is used. All register access is handled directly via the Arduino `Wire` library.

**Expected serial output on boot:**

```
WHO_AM_I (0x75): 0x72
ACCEL_CONFIG (0x1C): 0x0
GYRO_CONFIG (0x1B): 0x0
Calibrating — keep sensor still...
Offset AX=-1411 AY=467 AZ=1261
Offset GX=185 GY=355 GZ=-103
Calibration done.
Sensor listo
AX=0.002 AY=0.001 AZ=9.897 | GX=0.069 GY=0.137 GZ=0.015 | Roll=0.00 Pitch=-0.00
...
```

---

## Complementary Filter

Orientation is estimated by fusing accelerometer and gyroscope data:

```
orientation = alpha * gyroscope_integration + (1 - alpha) * accelerometer_angle
```

With alpha = 0.98:
- The gyroscope contributes 98% — smooth, responsive to motion.
- The accelerometer contributes 2% — corrects gyroscope drift over time.

Roll and pitch are estimated in degrees. Yaw is excluded (requires magnetometer).

**Verified at rest:** Roll/Pitch within ±0.35°.

---

## Key Findings

- The sensor responds at I2C address `0x68` but returns WHO_AM_I = `0x72` — not a genuine MPU-6050.
- ACCEL_CONFIG and GYRO_CONFIG default to `0x00`: ±2g and ±250°/s ranges confirmed.
- Scale factors: 16384 LSB/g (accelerometer), 131 LSB/°/s (gyroscope).
- Calibrated output at rest: AX ≈ AY ≈ 0 m/s², AZ ≈ 9.81 m/s², GX ≈ GY ≈ GZ ≈ 0 °/s.
- Complementary filter noise at rest: ±0.35°.

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
- [x] I2C diagnostic tool (tools/i2c_scan.cpp)
- [x] Digital filtering (complementary filter)
- [x] Roll/pitch orientation estimation
- [ ] OLED display integration (pending replacement hardware)
- [ ] Dataset collection framework
- [ ] Feature extraction pipeline
- [ ] TinyML model training
- [ ] On-device inference

---

## Documentation

| Document | Description |
|---|---|
| [TR-001](docs/TR-001.md) | Hardware characterization, firmware modularization, 6-axis acquisition, and complementary filter — Phases 1–3 |

---

## Author

**Eric Zuriel Zaldivar Peñaflor** — 2183082002  
Universidad Autónoma Metropolitana, Unidad Azcapotzalco  
Division of Basic Sciences and Engineering  
Microelectronics — Prof. José Luis Zamorano Flores
