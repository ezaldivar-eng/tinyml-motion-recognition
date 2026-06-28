#include <Arduino.h>
#include "imu.h"

ImuOffset offset;
ImuData data;

void setup() {
  // put your setup code here, to run once:
  //int result = myFunction(2, 3);

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
    Serial.print(" AZ="); Serial.println(data.az, 3);

    delay(200);
}