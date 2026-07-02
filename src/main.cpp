#include <Arduino.h>
#include "imu.h"

ImuOffset offset;
ImuData data;

void setup() {
    Serial.begin(115200);
    delay(2000);

    imu_init();
    imu_calibrate(offset);

    Serial.println("Sensor listo");
}

void loop() {
    imu_read(data, offset);

    Serial.print("AX="); Serial.print(data.ax, 3);
    Serial.print(" AY="); Serial.print(data.ay, 3);
    Serial.print(" AZ="); Serial.print(data.az, 3);
    Serial.print(" | GX="); Serial.print(data.gx, 3);
    Serial.print(" GY="); Serial.print(data.gy, 3);
    Serial.print(" GZ="); Serial.println(data.gz, 3);

    delay(200);
}