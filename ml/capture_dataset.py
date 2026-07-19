#!/usr/bin/env python3
"""
capture_dataset.py

Captura ventanas de gestos (shake/rotate/idle) desde la ESP32 vía serial
y las guarda en ml/data/<clase>.csv

Protocolo con el firmware (collector.h / collector.cpp):
  - Comandos de un solo caracter enviados por serial: s, r, i, x, h
  - El firmware emite una fila CSV por cada ventana completa (1s @ 50Hz):
        label,ax,ay,az,gx,gy,gz,ax_rms,ay_rms,az_rms,gx_rms,gy_rms,gz_rms
    -> 13 columnas: label + 12 features. (bug histórico: NO son 12, son 13)

Este script:
  1. Abre el puerto serial.
  2. Envía el comando de la clase elegida para iniciar sesión.
  3. Escucha líneas CSV durante --duration segundos.
  4. Envía 'x' para cerrar la sesión.
  5. Descarta la PRIMERA y ÚLTIMA ventana recibidas en la sesión
     (contienen la transición reposo->gesto y gesto->reposo).
  6. Agrega el resto (append) a ml/data/<clase>.csv

Uso:
  python capture_dataset.py --class shake  --port COM3 --duration 10
  python capture_dataset.py --class rotate --port COM3 --duration 10
  python capture_dataset.py --class idle   --port COM3 --duration 10

Requisitos:
  pip install pyserial
"""

import argparse
import csv
import sys
import time
from pathlib import Path

try:
    import serial
except ImportError:
    sys.exit(
        "Falta pyserial. Instala con:\n"
        "    pip install pyserial\n"
    )

BAUD_RATE = 115200
EXPECTED_COLUMNS = 13  # label + 12 features (ax,ay,az,gx,gy,gz,ax_rms,ay_rms,az_rms,gx_rms,gy_rms,gz_rms)

HEADER = [
    "label",
    "ax", "ay", "az",
    "gx", "gy", "gz",
    "ax_rms", "ay_rms", "az_rms",
    "gx_rms", "gy_rms", "gz_rms",
]

CLASS_COMMANDS = {
    "shake": "s",
    "rotate": "r",
    "idle": "i",
}

LABEL_NAMES = {
    "0": "shake",
    "1": "rotate",
    "2": "idle",
}

DATA_DIR = Path(__file__).parent / "data"


def parse_args():
    parser = argparse.ArgumentParser(description="Captura de dataset de gestos por serial.")
    parser.add_argument(
        "--class", dest="gesture_class", required=True, choices=CLASS_COMMANDS.keys(),
        help="Clase a capturar: shake, rotate o idle."
    )
    parser.add_argument("--port", required=True, help="Puerto serial de la ESP32 (ej. COM3).")
    parser.add_argument(
        "--duration", type=int, default=10,
        help="Duración de la sesión en segundos (default: 10)."
    )
    parser.add_argument(
        "--baud", type=int, default=BAUD_RATE,
        help=f"Baud rate (default: {BAUD_RATE})."
    )
    return parser.parse_args()


def is_valid_row(fields):
    """Valida cantidad de columnas y que las 12 features sean numéricas."""
    if len(fields) != EXPECTED_COLUMNS:
        return False
    try:
        [float(x) for x in fields[1:]]
    except ValueError:
        return False
    return True


def capture_session(ser, command, duration):
    """Envía el comando de inicio, escucha por `duration` segundos, cierra sesión."""
    raw_rows = []

    # Limpia cualquier dato viejo en el buffer antes de empezar
    ser.reset_input_buffer()

    print(f"Enviando comando de inicio: '{command}'")
    ser.write(command.encode())

    start = time.time()
    while time.time() - start < duration:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue
        fields = line.split(",")
        if is_valid_row(fields):
            raw_rows.append(fields)
            label_display = LABEL_NAMES.get(fields[0], fields[0])
            print(f"  ventana recibida ({len(raw_rows)}): {label_display} (label={fields[0]})")
        # Filas que no matchean (headers, ruido, líneas parciales) se ignoran silenciosamente

    print("Enviando comando de cierre: 'x'")
    ser.write(b"x")
    time.sleep(0.3)  # margen para que el firmware procese el cierre

    return raw_rows


def trim_edges(rows):
    """Descarta primera y última ventana de la sesión (contaminación de borde)."""
    if len(rows) <= 2:
        print(
            f"AVISO: solo se recibieron {len(rows)} ventanas, "
            "no hay suficientes para recortar bordes. Sesión descartada."
        )
        return []
    return rows[1:-1]


def append_to_csv(gesture_class, rows):
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    out_path = DATA_DIR / f"{gesture_class}.csv"
    file_exists = out_path.exists()

    with open(out_path, "a", newline="") as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(HEADER)
        writer.writerows(rows)

    print(f"Guardadas {len(rows)} ventanas limpias en {out_path}")
    return out_path


def main():
    args = parse_args()
    command = CLASS_COMMANDS[args.gesture_class]

    print(f"Conectando a {args.port} @ {args.baud} baud...")
    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except serial.SerialException as e:
        sys.exit(f"No se pudo abrir el puerto {args.port}: {e}")

    time.sleep(2)  # margen para que el ESP32 termine el reset tras abrir el puerto

    print(f"Clase: {args.gesture_class} | Duración de sesión: {args.duration}s")
    print("Realiza el gesto de forma repetitiva y continua ahora...")

    raw_rows = capture_session(ser, command, args.duration)
    ser.close()

    print(f"Ventanas crudas recibidas: {len(raw_rows)}")

    clean_rows = trim_edges(raw_rows)
    if not clean_rows:
        print("Nada que guardar en esta sesión.")
        return

    append_to_csv(args.gesture_class, clean_rows)


if __name__ == "__main__":
    main()
