#pragma once
#include <Arduino.h>
#include <Wire.h>

#define IMU_ADDR 0x68

// Accelerometer scale factor for ±2g range (default)
#define ACCEL_SCALE 16384.0f

// Gyroscope scale factor for ±250°/s range (default)
#define GYRO_SCALE 131.0f

struct ImuData {
    float ax, ay, az; // m/s²
    float gx, gy, gz; // °/s
};

struct ImuOffset {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
};

void imu_init();
void imu_calibrate(ImuOffset &offset, int samples = 200);
void imu_read(ImuData &data, const ImuOffset &offset);
uint8_t imu_read_register(uint8_t reg);
void imu_write_register(uint8_t reg, uint8_t value);