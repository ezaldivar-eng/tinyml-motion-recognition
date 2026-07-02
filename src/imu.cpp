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

    uint8_t gyroConfig = imu_read_register(0x1B);
    Serial.print("GYRO_CONFIG (0x1B): 0x"); Serial.println(gyroConfig, HEX);
}

void imu_calibrate(ImuOffset &offset, int samples) {
    Serial.println("Calibrating — keep sensor still...");
    delay(500);

    int32_t sumAX = 0, sumAY = 0, sumAZ = 0;
    int32_t sumGX = 0, sumGY = 0, sumGZ = 0;

    for (int i = 0; i < samples; i++) {
        // Accelerometer 0x3B–0x40
        Wire.beginTransmission(IMU_ADDR);
        Wire.write(0x3B);
        Wire.endTransmission(false);
        Wire.requestFrom(IMU_ADDR, 6);

        if (Wire.available() >= 6) {
            sumAX += (int16_t)((Wire.read() << 8) | Wire.read());
            sumAY += (int16_t)((Wire.read() << 8) | Wire.read());
            sumAZ += (int16_t)((Wire.read() << 8) | Wire.read());
        }

        // Gyroscope 0x43–0x48
        Wire.beginTransmission(IMU_ADDR);
        Wire.write(0x43);
        Wire.endTransmission(false);
        Wire.requestFrom(IMU_ADDR, 6);

        if (Wire.available() >= 6) {
            sumGX += (int16_t)((Wire.read() << 8) | Wire.read());
            sumGY += (int16_t)((Wire.read() << 8) | Wire.read());
            sumGZ += (int16_t)((Wire.read() << 8) | Wire.read());
        }

        delay(5);
    }

    offset.ax = sumAX / samples;
    offset.ay = sumAY / samples;
    offset.az = (sumAZ / samples) - (int16_t)ACCEL_SCALE;

    // Gyro offsets — should be ~0 at rest
    offset.gx = sumGX / samples;
    offset.gy = sumGY / samples;
    offset.gz = sumGZ / samples;

    Serial.print("Offset AX="); Serial.print(offset.ax);
    Serial.print(" AY="); Serial.print(offset.ay);
    Serial.print(" AZ="); Serial.println(offset.az);
    Serial.print("Offset GX="); Serial.print(offset.gx);
    Serial.print(" GY="); Serial.print(offset.gy);
    Serial.print(" GZ="); Serial.println(offset.gz);
    Serial.println("Calibration done.");
}

void imu_read(ImuData &data, const ImuOffset &offset) {
    // Accelerometer
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, 6);

    if (Wire.available() >= 6) {
        int16_t ax = (Wire.read() << 8) | Wire.read();
        int16_t ay = (Wire.read() << 8) | Wire.read();
        int16_t az = (Wire.read() << 8) | Wire.read();

        data.ax = ((ax - offset.ax) / ACCEL_SCALE) * 9.81f;
        data.ay = ((ay - offset.ay) / ACCEL_SCALE) * 9.81f;
        data.az = ((az - offset.az) / ACCEL_SCALE) * 9.81f;
    }

    // Gyroscope
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, 6);

    if (Wire.available() >= 6) {
        int16_t gx = (Wire.read() << 8) | Wire.read();
        int16_t gy = (Wire.read() << 8) | Wire.read();
        int16_t gz = (Wire.read() << 8) | Wire.read();

        data.gx = (gx - offset.gx) / GYRO_SCALE;
        data.gy = (gy - offset.gy) / GYRO_SCALE;
        data.gz = (gz - offset.gz) / GYRO_SCALE;
    }
}