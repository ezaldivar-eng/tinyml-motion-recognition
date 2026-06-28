#include "imu.h"

uint8_t imu_read_register(uint8_t reg) {
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, 1);
    if (Wire.available()) return Wire.read();
    return 0xFF;
}

void imu_write_register(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void imu_init() {
    Wire.begin(21, 22);
    imu_write_register(0x6B, 0x00); // wake up
    delay(100);

    uint8_t whoAmI = imu_read_register(0x75);
    Serial.print("WHO_AM_I (0x75): 0x"); Serial.println(whoAmI, HEX);

    // Verify accelerometer config register (default should be 0x00 = ±2g)
    uint8_t accelConfig = imu_read_register(0x1C);
    Serial.print("ACCEL_CONFIG (0x1C): 0x"); Serial.println(accelConfig, HEX);
}

void imu_calibrate(ImuOffset &offset, int samples) {
    Serial.println("Calibrating — keep sensor still...");
    delay(500);

    int32_t sumX = 0, sumY = 0, sumZ = 0;

    for (int i = 0; i < samples; i++) {
        Wire.beginTransmission(IMU_ADDR);
        Wire.write(0x3B);
        Wire.endTransmission(false);
        Wire.requestFrom(IMU_ADDR, 6);

        if (Wire.available() >= 6) {
            int16_t ax = (Wire.read() << 8) | Wire.read();
            int16_t ay = (Wire.read() << 8) | Wire.read();
            int16_t az = (Wire.read() << 8) | Wire.read();
            sumX += ax;
            sumY += ay;
            sumZ += az;
        }
        delay(5);
    }

    offset.ax = sumX / samples;
    offset.ay = sumY / samples;
    // AZ offset: remove gravity (1g = ACCEL_SCALE)
    offset.az = (sumZ / samples) - (int16_t)ACCEL_SCALE;

    Serial.print("Offset AX="); Serial.print(offset.ax);
    Serial.print(" AY="); Serial.print(offset.ay);
    Serial.print(" AZ="); Serial.println(offset.az);
    Serial.println("Calibration done.");
}

void imu_read(ImuData &data, const ImuOffset &offset) {
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, 6);

    if (Wire.available() >= 6) {
        int16_t ax = (Wire.read() << 8) | Wire.read();
        int16_t ay = (Wire.read() << 8) | Wire.read();
        int16_t az = (Wire.read() << 8) | Wire.read();

        // Apply offset and convert to m/s²
        data.ax = ((ax - offset.ax) / ACCEL_SCALE) * 9.81f;
        data.ay = ((ay - offset.ay) / ACCEL_SCALE) * 9.81f;
        data.az = ((az - offset.az) / ACCEL_SCALE) * 9.81f;
    }
}