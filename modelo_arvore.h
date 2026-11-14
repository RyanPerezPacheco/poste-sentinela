// ARQUIVO GERADO AUTOMATICAMENTE por ia/treinar_modelo.py - não editar à mão.
// Árvore de Decisão treinada em Python e convertida para C (roda 100% no ESP32).
#pragma once

const char* NOMES_ESTADO[] = {
  "NORMAL",
  "LAMPADA_QUEIMADA",
  "ACESA_DE_DIA",
  "QUEDA_ENERGIA",
  "POSTE_INCLINADO",
  "POSTE_CAIDO"
};

int classificarEstado(float lux_ambiente, float lux_lampada,
                      float tensao_rede, float inclinacao) {
  if (tensao_rede <= 187.5175f) {
    if (inclinacao <= 13.3366f) {
      return 3;  // QUEDA_ENERGIA
    } else {
      if (inclinacao <= 44.9186f) {
        return 4;  // POSTE_INCLINADO
      } else {
        return 5;  // POSTE_CAIDO
      }
    }
  } else {
    if (inclinacao <= 14.9306f) {
      if (lux_ambiente <= 41.6020f) {
        if (lux_lampada <= 46.7742f) {
          return 1;  // LAMPADA_QUEIMADA
        } else {
          return 0;  // NORMAL
        }
      } else {
        if (lux_lampada <= 45.1807f) {
          return 0;  // NORMAL
        } else {
          if (lux_ambiente <= 65.4642f) {
            return 2;  // ACESA_DE_DIA
          } else {
            return 2;  // ACESA_DE_DIA
          }
        }
      }
    } else {
      if (inclinacao <= 44.6680f) {
        return 4;  // POSTE_INCLINADO
      } else {
        return 5;  // POSTE_CAIDO
      }
    }
  }
}
