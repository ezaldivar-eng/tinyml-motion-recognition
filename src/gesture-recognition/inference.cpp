#include "inference.h"
#include "collector.h"       // WINDOW_SIZE
#include "model_data.h"      // g_gesture_model
#include "scaler_params.h"   // FEATURE_MEAN, FEATURE_STD

#include <Arduino.h>
#include <cmath>
#include <MicroTFLite.h>

namespace {

// Conservative starting point for a ~3K parameter model (2 Dense layers +
// softmax). If ModelInit() fails on the serial monitor, increase this in
// 1024-byte steps.
constexpr int kTensorArenaSize = 8 * 1024;
alignas(16) uint8_t g_tensor_arena[kTensorArenaSize];

bool g_model_ready = false;

// Window accumulator — same mean/RMS logic as collector.cpp, but running
// continuously (not gated by capture sessions).
float g_ax[WINDOW_SIZE], g_ay[WINDOW_SIZE], g_az[WINDOW_SIZE];
float g_gx[WINDOW_SIZE], g_gy[WINDOW_SIZE], g_gz[WINDOW_SIZE];
int g_sample_count = 0;

GestureClass g_last_result = GestureClass::UNKNOWN;
float g_last_confidence = 0.0f;

// True from the moment a window finishes classifying until the main loop
// consumes it via inference_has_new_result(). Prevents the same result
// from being acted on repeatedly while it's held between windows.
bool g_new_result = false;

float mean_of(const float* buf, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++) sum += buf[i];
    return sum / n;
}

float rms_of(const float* buf, int n) {
    float sum_sq = 0.0f;
    for (int i = 0; i < n; i++) sum_sq += buf[i] * buf[i];
    return sqrtf(sum_sq / n);
}

void run_inference_on_window() {
    // 1. Reduce the window to the same 12 features, in the same order,
    //    that capture_dataset.py wrote to the training CSV:
    //    ax,ay,az,gx,gy,gz,ax_rms,ay_rms,az_rms,gx_rms,gy_rms,gz_rms
    float raw_features[12] = {
        mean_of(g_ax, WINDOW_SIZE), mean_of(g_ay, WINDOW_SIZE), mean_of(g_az, WINDOW_SIZE),
        mean_of(g_gx, WINDOW_SIZE), mean_of(g_gy, WINDOW_SIZE), mean_of(g_gz, WINDOW_SIZE),
        rms_of(g_ax, WINDOW_SIZE),  rms_of(g_ay, WINDOW_SIZE),  rms_of(g_az, WINDOW_SIZE),
        rms_of(g_gx, WINDOW_SIZE),  rms_of(g_gy, WINDOW_SIZE),  rms_of(g_gz, WINDOW_SIZE),
    };

    // 2. Normalize EXACTLY as in train_model.py (StandardScaler):
    //    normalized = (raw - mean) / std
    float normalized[12];
    for (int i = 0; i < 12; i++) {
        normalized[i] = (raw_features[i] - FEATURE_MEAN[i]) / FEATURE_STD[i];
    }

    // 3. Feed the model. MicroTFLite's ModelSetInput() detects that the
    //    input tensor is int8 and quantizes automatically — no manual
    //    scale/zero_point math needed here.
    for (int i = 0; i < 12; i++) {
        if (!ModelSetInput(normalized[i], i)) {
            Serial.println("inference: ModelSetInput failed");
            g_last_result = GestureClass::UNKNOWN;
            return;
        }
    }

    // 4. Run inference
    if (!ModelRunInference()) {
        Serial.println("inference: ModelRunInference failed");
        g_last_result = GestureClass::UNKNOWN;
        return;
    }

    // 5. Read the 3 output probabilities (already dequantized by the
    //    library) and take the argmax.
    float best_val = ModelGetOutput(0);
    int best_idx = 0;
    for (int i = 1; i < 3; i++) {
        float val = ModelGetOutput(i);
        if (val > best_val) {
            best_val = val;
            best_idx = i;
        }
    }

    g_last_confidence = best_val;
    g_last_result = static_cast<GestureClass>(best_idx);
    g_new_result = true;  // signal: this is a fresh classification
}

}  // namespace

void inference_init() {
    g_model_ready = ModelInit(g_gesture_model, g_tensor_arena, kTensorArenaSize);
    if (!g_model_ready) {
        Serial.println("inference: ModelInit failed — check kTensorArenaSize");
        return;
    }
    Serial.println("inference: ready");
}

void inference_add_sample(float ax, float ay, float az,
                           float gx, float gy, float gz) {
    if (!g_model_ready) return;

    g_ax[g_sample_count] = ax; g_ay[g_sample_count] = ay; g_az[g_sample_count] = az;
    g_gx[g_sample_count] = gx; g_gy[g_sample_count] = gy; g_gz[g_sample_count] = gz;
    g_sample_count++;

    if (g_sample_count >= WINDOW_SIZE) {
        run_inference_on_window();
        g_sample_count = 0;  // non-overlapping window, same as training dataset
    }
}

GestureClass inference_last_result() {
    return g_last_result;
}

float inference_last_confidence() {
    return g_last_confidence;
}

bool inference_has_new_result() {
    bool r = g_new_result;
    g_new_result = false;
    return r;
}