#include <Arduino.h>
#include <Wire.h>

// put function declarations here:
//int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  //int result = myFunction(2, 3);

Serial.begin(115200);
     Serial.begin(115200);
    delay(2000);

    Wire.begin(21,22);

    // Despertar sensor
    Wire.beginTransmission(0x68);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();

    Serial.println("Sensor listo");
}

void loop() {

  // put your main code here, to run repeatedly:

    Wire.beginTransmission(0x68);
    Wire.write(0x3B);
    Wire.endTransmission(false);

    Wire.requestFrom(0x68, 6);

    if (Wire.available() >= 6) {

        int16_t ax =
            (Wire.read() << 8) | Wire.read();

        int16_t ay =
            (Wire.read() << 8) | Wire.read();

        int16_t az =
            (Wire.read() << 8) | Wire.read();

        Serial.print("AX=");
        Serial.print(ax);

        Serial.print(" AY=");
        Serial.print(ay);

        Serial.print(" AZ=");
        Serial.println(az);
    }

    delay(200);
}


// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}