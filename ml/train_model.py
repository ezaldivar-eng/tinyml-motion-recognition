#!/usr/bin/env python3
"""
train_model.py

Entrena el clasificador Dense(64->32->3) sobre el dataset de gestos,
lo cuantiza a int8 (TFLite Micro) y exporta:
  - models/gesture_model.keras       (modelo float, para referencia/reentrenamiento)
  - models/gesture_model.tflite      (modelo cuantizado int8)
  - models/model_data.h              (arreglo C para el firmware)
  - models/scaler_params.h           (media/desviación por feature, para
                                       normalizar en el ESP32 ANTES de inferencia)

Uso (desde la carpeta ml/):
    python train_model.py

Requisitos:
    pip install tensorflow pandas scikit-learn numpy
"""

import numpy as np
import pandas as pd
import tensorflow as tf
from pathlib import Path
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import confusion_matrix, classification_report

DATA_DIR = Path(__file__).parent / "data"
MODELS_DIR = Path(__file__).parent / "models"
FEATURE_COLUMNS = [
    "ax", "ay", "az", "gx", "gy", "gz",
    "ax_rms", "ay_rms", "az_rms", "gx_rms", "gy_rms", "gz_rms",
]
LABEL_NAMES = {0: "shake", 1: "rotate", 2: "idle"}
RANDOM_SEED = 42


def load_dataset():
    frames = [pd.read_csv(p) for p in DATA_DIR.glob("*.csv")]
    if not frames:
        raise SystemExit(f"No se encontraron CSVs en {DATA_DIR}. Corre capture_dataset.py primero.")
    df = pd.concat(frames, ignore_index=True)
    print(f"Dataset cargado: {len(df)} ventanas totales")
    print(df["label"].map(LABEL_NAMES).value_counts(), "\n")
    return df


def build_model():
    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=(len(FEATURE_COLUMNS),)),
        tf.keras.layers.Dense(64, activation="relu"),
        tf.keras.layers.Dense(32, activation="relu"),
        tf.keras.layers.Dense(3, activation="softmax"),
    ])
    model.compile(
        optimizer="adam",
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"],
    )
    return model


def quantize_int8(model, x_train_scaled):
    def representative_dataset():
        for row in x_train_scaled[:100].astype(np.float32):
            yield [row.reshape(1, -1)]

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    return converter.convert()


def export_c_array(tflite_bytes, out_path, var_name="g_gesture_model"):
    lines = [
        "// Auto-generado por train_model.py — no editar a mano.",
        "#pragma once",
        f"alignas(8) const unsigned char {var_name}[] = {{",
    ]
    hex_bytes = [f"0x{b:02x}" for b in tflite_bytes]
    for i in range(0, len(hex_bytes), 12):
        lines.append("  " + ", ".join(hex_bytes[i:i + 12]) + ",")
    lines.append("};")
    lines.append(f"const unsigned int {var_name}_len = {len(tflite_bytes)};")
    out_path.write_text("\n".join(lines))
    print(f"Modelo exportado como C array en {out_path} ({len(tflite_bytes)} bytes)")


def export_scaler_header(scaler, out_path):
    means = ", ".join(f"{v:.6f}f" for v in scaler.mean_)
    stds = ", ".join(f"{v:.6f}f" for v in scaler.scale_)
    content = f"""// Auto-generado por train_model.py — no editar a mano.
// Aplicar en el ESP32 ANTES de pasar las features al intérprete TFLite:
//   normalized[i] = (raw[i] - FEATURE_MEAN[i]) / FEATURE_STD[i];
#pragma once

const float FEATURE_MEAN[12] = {{ {means} }};
const float FEATURE_STD[12]  = {{ {stds} }};
"""
    out_path.write_text(content)
    print(f"Parámetros de normalización exportados en {out_path}")


def main():
    MODELS_DIR.mkdir(parents=True, exist_ok=True)

    df = load_dataset()
    X = df[FEATURE_COLUMNS].values
    y = df["label"].values

    x_train, x_val, y_train, y_val = train_test_split(
        X, y, test_size=0.2, random_state=RANDOM_SEED, stratify=y
    )

    scaler = StandardScaler()
    x_train_scaled = scaler.fit_transform(x_train)
    x_val_scaled = scaler.transform(x_val)

    model = build_model()
    model.summary()

    history = model.fit(
        x_train_scaled, y_train,
        validation_data=(x_val_scaled, y_val),
        epochs=50,
        batch_size=16,
        verbose=2,
    )

    print("\n=== Evaluación en validación ===")
    y_pred = np.argmax(model.predict(x_val_scaled), axis=1)
    print(classification_report(y_val, y_pred, target_names=[LABEL_NAMES[i] for i in sorted(LABEL_NAMES)]))
    print("Matriz de confusión:")
    print(confusion_matrix(y_val, y_pred))

    keras_path = MODELS_DIR / "gesture_model.keras"
    model.save(keras_path)
    print(f"\nModelo Keras guardado en {keras_path}")

    tflite_bytes = quantize_int8(model, x_train_scaled)
    tflite_path = MODELS_DIR / "gesture_model.tflite"
    tflite_path.write_bytes(tflite_bytes)
    print(f"Modelo TFLite int8 guardado en {tflite_path}")

    export_c_array(tflite_bytes, MODELS_DIR / "model_data.h")
    export_scaler_header(scaler, MODELS_DIR / "scaler_params.h")

    print("\nListo. Siguiente paso: integrar model_data.h y scaler_params.h al firmware "
          "gesture-recognition y dimensionar el tensor arena de TFLite Micro.")


if __name__ == "__main__":
    main()
