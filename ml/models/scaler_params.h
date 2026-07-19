// Auto-generado por train_model.py — no editar a mano.
// Aplicar en el ESP32 ANTES de pasar las features al intérprete TFLite:
//   normalized[i] = (raw[i] - FEATURE_MEAN[i]) / FEATURE_STD[i];
#pragma once

const float FEATURE_MEAN[12] = { 0.073077f, -0.068366f, 9.896164f, 0.363952f, 1.380785f, 0.018679f, 2.568002f, 1.198752f, 10.211305f, 25.649422f, 78.436200f, 15.817171f };
const float FEATURE_STD[12]  = { 0.728315f, 1.470722f, 0.624241f, 4.792384f, 14.704509f, 3.121751f, 2.072028f, 1.871198f, 0.904290f, 28.133307f, 70.508385f, 12.233228f };
