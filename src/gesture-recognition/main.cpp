#include <Arduino.h>
#include "imu.h"
#include "filter.h"
#include "collector.h"
#include "inference.h"

#define LED_SHAKE_PIN  25
#define LED_ROTATE_PIN 26
#define LED_ON_DURATION_MS 300

ImuOffset offset;
ImuData data;
Orientation orientation;
unsigned long lastTime = 0;

unsigned long ledShakeOffAt = 0;
unsigned long ledRotateOffAt = 0;

void triggerLed(int pin, unsigned long& offAtVar) {
    digitalWrite(pin, HIGH);
    offAtVar = millis() + LED_ON_DURATION_MS;
}

void updateLeds() {
    unsigned long now = millis();
    if (ledShakeOffAt != 0 && now >= ledShakeOffAt) {
        digitalWrite(LED_SHAKE_PIN, LOW);
        ledShakeOffAt = 0;
    }
    if (ledRotateOffAt != 0 && now >= ledRotateOffAt) {
        digitalWrite(LED_ROTATE_PIN, LOW);
        ledRotateOffAt = 0;
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(LED_SHAKE_PIN, OUTPUT);
    pinMode(LED_ROTATE_PIN, OUTPUT);
    digitalWrite(LED_SHAKE_PIN, LOW);
    digitalWrite(LED_ROTATE_PIN, LOW);

    imu_init();
    imu_calibrate(offset);
    filter_init(0.98f);
    collector_init();
    inference_init();

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

    // Inference only runs when not actively capturing a labeled dataset
    // session, so training data and live predictions never mix.
    if (!collector_is_recording()) {
        inference_add_sample(data.ax, data.ay, data.az,
                             data.gx, data.gy, data.gz);

        GestureClass g = inference_last_result();
        if (g != GestureClass::UNKNOWN) {
            switch (g) {
                case GestureClass::SHAKE:
                    Serial.println("shake");
                    triggerLed(LED_SHAKE_PIN, ledShakeOffAt);
                    break;
                case GestureClass::ROTATE:
                    Serial.println("rotate");
                    triggerLed(LED_ROTATE_PIN, ledRotateOffAt);
                    break;
                case GestureClass::IDLE:
                    // no output — idle is the "nothing happening" class
                    break;
                default:
                    break;
            }
        }
    }

    updateLeds();
    delay(SAMPLE_DELAY);
}