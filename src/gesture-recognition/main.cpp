#include <Arduino.h>
#include "imu.h"
#include "filter.h"
#include "collector.h"
#include "inference.h"

#define LED_SHAKE_PIN  23
#define LED_ROTATE_PIN 4

ImuOffset offset;
ImuData data;
Orientation orientation;
unsigned long lastTime = 0;

// Toggle state per gesture — each new detection of that class flips its LED.
// Independent: shake and rotate never affect each other's LED.
bool ledShakeState = false;
bool ledRotateState = false;

void toggleLed(int pin, bool& state) {
    state = !state;
    digitalWrite(pin, state ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(LED_SHAKE_PIN, OUTPUT);
    pinMode(LED_ROTATE_PIN, OUTPUT);
    digitalWrite(LED_SHAKE_PIN, LOW);
    digitalWrite(LED_ROTATE_PIN, LOW);
    ledShakeState = false;
    ledRotateState = false;

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

        // Only act once per newly completed window — not once per loop
        // iteration — otherwise the same classification (held for ~1s
        // between windows) would toggle the LED dozens of times.
        if (inference_has_new_result()) {
            GestureClass g = inference_last_result();
            switch (g) {
                case GestureClass::SHAKE:
                    Serial.println("shake");
                    toggleLed(LED_SHAKE_PIN, ledShakeState);
                    break;
                case GestureClass::ROTATE:
                    Serial.println("rotate");
                    toggleLed(LED_ROTATE_PIN, ledRotateState);
                    break;
                case GestureClass::IDLE:
                    // no output — idle is the "nothing happening" class
                    break;
                default:
                    break;
            }
        }
    }

    delay(SAMPLE_DELAY);
}