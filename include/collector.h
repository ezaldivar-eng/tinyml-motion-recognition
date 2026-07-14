#pragma once
#include <Arduino.h>

#define WINDOW_SIZE  50
#define SAMPLE_DELAY 20

void collector_init();
void collector_check_command();
void collector_add_sample(float ax, float ay, float az,
                          float gx, float gy, float gz);
bool collector_is_recording();