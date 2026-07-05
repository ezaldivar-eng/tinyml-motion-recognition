#pragma once
#include <Arduino.h>

struct Orientation {
    float roll;   // deg — rotation around X
    float pitch;  // deg — rotation around Y
};

void filter_init(float alpha = 0.98f);
void filter_update(Orientation &orientation, float ax, float ay, float az,
                   float gx, float gy, float gz, float dt);