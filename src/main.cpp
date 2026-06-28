#include <Arduino.h>
#include <Wire.h>

// put function declarations here:
//int myFunction(int, int);


#define IMU_ADDR 0x68

uint8_t readRegister(uint8_t reg) {
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, 1);
    if (Wire.available()) return Wire.read();
    return 0xFF;
}

void setup() {
  // put your setup code here, to run once:
  //int result = myFunction(2, 3);
  
    Serial.begin(115200);
    delay(2000);

    Wire.begin(21, 22);

    // Despertar sensor
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(100);

    // Identificación del chip
    uint8_t whoAmI = readRegister(0x75);
    uint8_t reg47  = readRegister(0x47);
    uint8_t reg00  = readRegister(0x00);

    Serial.println("=== Identificacion del sensor ===");
    Serial.print("WHO_AM_I (0x75): 0x"); Serial.println(whoAmI, HEX);
    Serial.print("REG 0x47:        0x"); Serial.println(reg47,  HEX);
    Serial.print("REG 0x00:        0x"); Serial.println(reg00,  HEX);
    Serial.println("=================================");
    Serial.println("Sensor listo");
}

void loop() {
    Wire.beginTransmission(IMU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(IMU_ADDR, 6);

    if (Wire.available() >= 6) {
        int16_t ax = (Wire.read() << 8) | Wire.read();
        int16_t ay = (Wire.read() << 8) | Wire.read();
        int16_t az = (Wire.read() << 8) | Wire.read();

        Serial.print("AX="); Serial.print(ax);
        Serial.print(" AY="); Serial.print(ay);
        Serial.print(" AZ="); Serial.println(az);
    }

    delay(200);
}