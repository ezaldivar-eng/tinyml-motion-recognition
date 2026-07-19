#!/usr/bin/env python3
"""
eda.py

Análisis exploratorio del dataset de gestos capturado con capture_dataset.py.
Objetivo: confirmar separabilidad de clases y decidir la estrategia de
normalización antes de entrenar.

Uso (desde la carpeta ml/):
    python eda.py

Requisitos:
    pip install pandas matplotlib scikit-learn
"""

import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

DATA_DIR = Path(__file__).parent / "data"
FEATURE_COLUMNS = [
    "ax", "ay", "az", "gx", "gy", "gz",
    "ax_rms", "ay_rms", "az_rms", "gx_rms", "gy_rms", "gz_rms",
]
LABEL_NAMES = {0: "shake", 1: "rotate", 2: "idle"}


def load_dataset():
    frames = []
    for csv_path in DATA_DIR.glob("*.csv"):
        df = pd.read_csv(csv_path)
        frames.append(df)
    if not frames:
        raise SystemExit(f"No se encontraron CSVs en {DATA_DIR}")
    full = pd.concat(frames, ignore_index=True)
    full["gesture_name"] = full["label"].map(LABEL_NAMES)
    return full


def print_class_balance(df):
    print("=== Balance de clases ===")
    counts = df["gesture_name"].value_counts()
    print(counts)
    print(f"Total de ventanas: {len(df)}\n")
    max_count, min_count = counts.max(), counts.min()
    if max_count / min_count > 1.5:
        print(
            "AVISO: desbalance notable entre clases "
            f"({min_count} vs {max_count}). Considera igualar antes de entrenar "
            "(recortar la clase más grande o capturar más de la más chica).\n"
        )


def print_stats_by_class(df):
    print("=== Estadísticas por clase (media y desviación estándar) ===")
    stats = df.groupby("gesture_name")[FEATURE_COLUMNS].agg(["mean", "std"])
    print(stats.T)
    print()


def plot_boxplots(df):
    fig, axes = plt.subplots(3, 4, figsize=(18, 10))
    axes = axes.flatten()
    for i, col in enumerate(FEATURE_COLUMNS):
        df.boxplot(column=col, by="gesture_name", ax=axes[i])
        axes[i].set_title(col)
        axes[i].set_xlabel("")
    plt.suptitle("Distribución de features por clase")
    plt.tight_layout()
    out_path = Path(__file__).parent / "eda_boxplots.png"
    plt.savefig(out_path, dpi=120)
    print(f"Boxplots guardados en {out_path}")


def plot_rms_scatter(df):
    # Las features RMS son las que más deberían separar shake/rotate/idle,
    # ya que capturan energía de movimiento (idle debería quedar cerca de 0).
    fig, ax = plt.subplots(figsize=(8, 6))
    colors = {"shake": "tab:red", "rotate": "tab:blue", "idle": "tab:green"}
    for name, group in df.groupby("gesture_name"):
        ax.scatter(group["ax_rms"], group["gx_rms"], label=name,
                   alpha=0.6, color=colors.get(name))
    ax.set_xlabel("ax_rms")
    ax.set_ylabel("gx_rms")
    ax.set_title("Separabilidad: ax_rms vs gx_rms por clase")
    ax.legend()
    out_path = Path(__file__).parent / "eda_scatter_rms.png"
    plt.savefig(out_path, dpi=120)
    print(f"Scatter guardado en {out_path}")


def suggest_normalization(df):
    print("=== Sugerencia de normalización ===")
    ranges = df[FEATURE_COLUMNS].agg(["min", "max"])
    print(ranges)
    span = ranges.loc["max"] - ranges.loc["min"]
    print(
        "\nLas features mezclan unidades distintas (m/s^2 en accel, deg/s en "
        "gyro) y magnitudes muy diferentes entre sí (revisa el rango arriba). "
        "Con esa disparidad, StandardScaler (media 0, desviación 1) por "
        "feature es la opción por defecto recomendada antes de entrenar el "
        "Dense(64->32->3) — evita que las features de mayor magnitud (típicamente "
        "las del giroscopio) dominen el gradiente durante el entrenamiento.\n"
        "Decisión final: confirmar visualmente con los boxplots que ninguna "
        "clase queda con varianza anormalmente distinta antes de aplicar.\n"
    )


def main():
    df = load_dataset()
    print_class_balance(df)
    print_stats_by_class(df)
    plot_boxplots(df)
    plot_rms_scatter(df)
    suggest_normalization(df)
    print("EDA completo. Revisa eda_boxplots.png y eda_scatter_rms.png antes de entrenar.")


if __name__ == "__main__":
    main()
