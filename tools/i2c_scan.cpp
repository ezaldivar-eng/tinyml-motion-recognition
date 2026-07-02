#include <Arduino.h>
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    Wire.begin(21, 22);

    Serial.println("=== I2C Bus Scan ===");
    uint8_t count = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.print("Device found at 0x");
            Serial.println(addr, HEX);
            count++;
        }
    }
    Serial.print("Devices found: ");
    Serial.println(count);
    Serial.println("===================");
}

void loop() {}