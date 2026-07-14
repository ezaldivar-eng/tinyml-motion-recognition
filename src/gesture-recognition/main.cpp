#include <Arduino.h>
#include "imu.h"
#include "filter.h"
#include "collector.h"

ImuOffset offset;
ImuData data;
Orientation orientation;
unsigned long lastTime = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);

    imu_init();
    imu_calibrate(offset);
    filter_init(0.98f);
    collector_init();

    orientation.roll  = 0.0f;
    orientation.pitch = 0.0f;
    lastTime = millis();
}

void loop() {
    unsigned long now = millis();
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;

    collector_check_command();

    imu_read(data, offset);
    filter_update(orientation, data.ax, data.ay, data.az,
                               data.gx, data.gy, data.gz, dt);

    collector_add_sample(data.ax, data.ay, data.az,
                         data.gx, data.gy, data.gz);

    delay(SAMPLE_DELAY);
}