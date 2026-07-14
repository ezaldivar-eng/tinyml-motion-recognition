#include "collector.h"

static int   _currentLabel  = -1;
static int   _sampleCount   = 0;
static bool  _recording     = false;
static float _window[WINDOW_SIZE][6]; // ax,ay,az,gx,gy,gz

void collector_init() {
    Serial.println("=== Dataset Collector ===");
    Serial.println("Commands:");
    Serial.println("  s -> record SHAKE");
    Serial.println("  r -> record ROTATE");
    Serial.println("  i -> record IDLE");
    Serial.println("  x -> stop recording");
    Serial.println("  h -> print header");
    Serial.println("========================");
    Serial.println("gesture,ax,ay,az,gx,gy,gz,ax_rms,ay_rms,az_rms,gx_rms,gy_rms,gz_rms");
}

void collector_check_command() {
    if (!Serial.available()) return;
    char cmd = Serial.read();
    switch (cmd) {
        case 's': _currentLabel = 0; _recording = true; _sampleCount = 0;
            Serial.println("# Recording SHAKE — perform gesture repeatedly"); break;
        case 'r': _currentLabel = 1; _recording = true; _sampleCount = 0;
            Serial.println("# Recording ROTATE — perform gesture repeatedly"); break;
        case 'i': _currentLabel = 2; _recording = true; _sampleCount = 0;
            Serial.println("# Recording IDLE — keep still"); break;
        case 'x': _recording = false; _currentLabel = -1; _sampleCount = 0;
            Serial.println("# Stopped"); break;
        case 'h':
            Serial.println("gesture,ax,ay,az,gx,gy,gz,ax_rms,ay_rms,az_rms,gx_rms,gy_rms,gz_rms"); break;
    }
}

static float rms(float* values, int n) {
    float sum = 0;
    for (int i = 0; i < n; i++) sum += values[i] * values[i];
    return sqrtf(sum / n);
}

void collector_add_sample(float ax, float ay, float az,
                          float gx, float gy, float gz) {
    if (!_recording) return;

    _window[_sampleCount][0] = ax;
    _window[_sampleCount][1] = ay;
    _window[_sampleCount][2] = az;
    _window[_sampleCount][3] = gx;
    _window[_sampleCount][4] = gy;
    _window[_sampleCount][5] = gz;
    _sampleCount++;

    if (_sampleCount >= WINDOW_SIZE) {
        // Extract features
        float ax_buf[WINDOW_SIZE], ay_buf[WINDOW_SIZE], az_buf[WINDOW_SIZE];
        float gx_buf[WINDOW_SIZE], gy_buf[WINDOW_SIZE], gz_buf[WINDOW_SIZE];

        float ax_mean=0, ay_mean=0, az_mean=0;
        float gx_mean=0, gy_mean=0, gz_mean=0;

        for (int i = 0; i < WINDOW_SIZE; i++) {
            ax_buf[i] = _window[i][0]; ax_mean += ax_buf[i];
            ay_buf[i] = _window[i][1]; ay_mean += ay_buf[i];
            az_buf[i] = _window[i][2]; az_mean += az_buf[i];
            gx_buf[i] = _window[i][3]; gx_mean += gx_buf[i];
            gy_buf[i] = _window[i][4]; gy_mean += gy_buf[i];
            gz_buf[i] = _window[i][5]; gz_mean += gz_buf[i];
        }

        ax_mean /= WINDOW_SIZE; ay_mean /= WINDOW_SIZE; az_mean /= WINDOW_SIZE;
        gx_mean /= WINDOW_SIZE; gy_mean /= WINDOW_SIZE; gz_mean /= WINDOW_SIZE;

        // Print CSV row: label + means + RMS
        Serial.print(_currentLabel); Serial.print(",");
        Serial.print(ax_mean, 4); Serial.print(",");
        Serial.print(ay_mean, 4); Serial.print(",");
        Serial.print(az_mean, 4); Serial.print(",");
        Serial.print(gx_mean, 4); Serial.print(",");
        Serial.print(gy_mean, 4); Serial.print(",");
        Serial.print(gz_mean, 4); Serial.print(",");
        Serial.print(rms(ax_buf, WINDOW_SIZE), 4); Serial.print(",");
        Serial.print(rms(ay_buf, WINDOW_SIZE), 4); Serial.print(",");
        Serial.print(rms(az_buf, WINDOW_SIZE), 4); Serial.print(",");
        Serial.print(rms(gx_buf, WINDOW_SIZE), 4); Serial.print(",");
        Serial.print(rms(gy_buf, WINDOW_SIZE), 4); Serial.print(",");
        Serial.println(rms(gz_buf, WINDOW_SIZE), 4);

        _sampleCount = 0;
    }
}

bool collector_is_recording() {
    return _recording;
}