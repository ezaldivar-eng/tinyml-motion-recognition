#include <Arduino.h>
#include "imu.h"
#include "filter.h"

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

    orientation.roll  = 0.0f;
    orientation.pitch = 0.0f;

    lastTime = millis();
    Serial.println("Sensor listo");
}

void loop() {
    unsigned long now = millis();
    float dt = (now - lastTime) / 1000.0f;
    lastTime = now;

    imu_read(data, offset);
    filter_update(orientation, data.ax, data.ay, data.az,
                               data.gx, data.gy, data.gz, dt);

    Serial.print("AX="); Serial.print(data.ax, 3);
    Serial.print(" AY="); Serial.print(data.ay, 3);
    Serial.print(" AZ="); Serial.print(data.az, 3);
    Serial.print(" | GX="); Serial.print(data.gx, 3);
    Serial.print(" GY="); Serial.print(data.gy, 3);
    Serial.print(" GZ="); Serial.print(data.gz, 3);
    Serial.print(" | Roll="); Serial.print(orientation.roll, 2);
    Serial.print(" Pitch="); Serial.println(orientation.pitch, 2);

    delay(200);
}
