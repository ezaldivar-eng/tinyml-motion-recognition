# TinyML IMU Data Acquisition — ESP32

Low-level IMU interface and raw accelerometer data acquisition pipeline on an ESP32 DOIT DevKit V1, built as the foundation for a TinyML motion classification project.

> **Status:** Phase 1 complete — hardware validated, raw accelerometer data streaming over serial.

---

## Overview

This project establishes a direct I2C communication layer between an ESP32 and a commercially labeled MPU-6050 IMU module, **without relying on vendor libraries**. Hardware characterization revealed that the installed sensor is not a genuine MPU-6050 (WHO_AM_I = `0x72` instead of `0x68`), but is a pin-compatible replacement with an identical register layout. Full communication and data acquisition were validated at the register level.

The end goal of the broader project is on-device motion inference using TinyML models trained on datasets collected through this pipeline.

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
│   └── main.cpp          # Main firmware
├── include/              # Header files (PlatformIO default)
├── lib/                  # Local libraries (PlatformIO default)
├── test/                 # Unit tests (PlatformIO default)
├── docs/
│   ├── TR-001.docx       # Technical Report 001 — Hardware Characterization
│   └── TR-001.md         # Technical Report 001 — Markdown version
├── platformio.ini
└── README.md
```

---

## Setup

**Requirements:**
- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- ESP32 DOIT DevKit V1
- IMU module wired as described above

**Build and flash:**

```bash
pio run --target upload
```

**Monitor serial output:**

```bash
pio device monitor
```

Baud rate: `115200`

---

## Firmware Description

The firmware (`src/main.cpp`) performs the following:

1. Initializes I2C on GPIO21 (SDA) and GPIO22 (SCL).
2. Wakes the sensor by writing `0x00` to the power management register (`0x6B`).
3. Reads identification registers (`WHO_AM_I`, `0x47`, `0x00`) and prints them over serial.
4. Continuously reads the raw accelerometer output registers (`0x3B`–`0x40`) and streams AX, AY, AZ values at ~5 Hz.

No external IMU library is used. All register access is handled directly via the Arduino `Wire` library.

**Expected serial output on boot:**

```
=== Identificacion del sensor ===
WHO_AM_I (0x75): 0x72
REG 0x47:        0x??
REG 0x00:        0x??
=================================
Sensor listo
AX=-1876 AY=524 AZ=17556
AX=-1881 AY=519 AZ=17603
...
```

---

## Key Findings (Phase 1)

- The sensor responds at I2C address `0x68` but returns `WHO_AM_I = 0x72`, indicating it is not a genuine MPU-6050.
- Despite this, the register map is compatible: power management, WHO_AM_I, and accelerometer output registers behave as expected.
- Stationary noise observed: AX ≈ −1900 ± 100, AY ≈ 500 ± 70, AZ ≈ 17600 ± 150.
- No communication instability detected.

Full methodology documented in [`docs/TR-001.md`](docs/TR-001.md).

---

## Roadmap

- [x] I2C communication validated
- [x] Register read/write confirmed
- [x] Sensor wake-up sequence implemented
- [x] Raw accelerometer data streaming
- [ ] Identify exact IMU model (gyroscope register mapping)
- [ ] Gyroscope data acquisition
- [ ] Sensor calibration and offset correction
- [ ] Noise analysis and digital filtering
- [ ] Dataset collection framework
- [ ] Feature extraction pipeline
- [ ] TinyML model training
- [ ] On-device inference

---

## Documentation

| Document | Description |
|---|---|
| [TR-001](docs/TR-001.md) | Hardware characterization and low-level interface validation |

---

## Author

**Eric Zuriel Zaldivar Peñaflor** — 2183082002  
Universidad Autónoma Metropolitana, Unidad Azcapotzalco  
Division of Basic Sciences and Engineering  
Microelectronics — Prof. José Luis Zamorano Flores
