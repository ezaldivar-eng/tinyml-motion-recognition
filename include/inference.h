#pragma once

// Result of the most recently classified window.
// Matches the label encoding used during dataset capture:
// shake=0, rotate=1, idle=2 (see collector.h / capture_dataset.py)
enum class GestureClass {
    SHAKE = 0,
    ROTATE = 1,
    IDLE = 2,
    UNKNOWN = -1  // not enough samples yet to complete a window
};

// Initializes the TFLite Micro interpreter. Call once in setup(),
// after collector_init().
void inference_init();

// Feeds one calibrated IMU sample (same values passed to collector_add_sample).
// Internally accumulates a window of WINDOW_SIZE samples; once full,
// automatically runs inference and updates the result returned by
// inference_last_result().
void inference_add_sample(float ax, float ay, float az,
                           float gx, float gy, float gz);

// Predicted class for the most recently completed window. UNKNOWN if no
// window has completed yet since boot.
GestureClass inference_last_result();

// Confidence (0.0-1.0) of the most recent prediction, dequantized from the
// model's int8 output by the MicroTFLite library itself. Useful for
// debug/logging, not critical for the gesture logic itself.
float inference_last_confidence();

// True exactly once per newly completed window classification.
// Calling this consumes (clears) the flag — call it once per loop, and
// only act on inference_last_result() when this returns true. Without
// this, the same classification would be re-read on every loop iteration
// (every ~20ms) for the entire ~1s window duration.
bool inference_has_new_result();