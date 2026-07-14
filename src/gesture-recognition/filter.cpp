#include "filter.h"

static float _alpha = 0.98f;

void filter_init(float alpha) {
    _alpha = alpha;
}

void filter_update(Orientation &orientation, float ax, float ay, float az,
                   float gx, float gy, float gz, float dt) {

    // Roll and pitch from accelerometer (degrees)
    float accel_roll  = atan2f(ay, az) * 180.0f / M_PI;
    float accel_pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / M_PI;

    // Integrate gyroscope (degrees)
    float gyro_roll  = orientation.roll  + gx * dt;
    float gyro_pitch = orientation.pitch + gy * dt;

    // Complementary filter fusion
    orientation.roll  = _alpha * gyro_roll  + (1.0f - _alpha) * accel_roll;
    orientation.pitch = _alpha * gyro_pitch + (1.0f - _alpha) * accel_pitch;
}