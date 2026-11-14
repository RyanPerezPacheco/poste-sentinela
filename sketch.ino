/*
 * POSTE SENTINELA - CitizenTech | FIAP Global Solution 2026.2 | Maker Lab
 * ---------------------------------------------------------------------
 * ESP32 instalado no poste de iluminação pública que:
 *   - lê 5 sensores (luz ambiente, luz da lâmpada, tensão da rede,
 *     inclinação do poste e presença de pessoas);
 *   - usa uma Árvore de Decisão (IA treinada em Python, ver ia/) para
 *     classificar o estado do poste;
 *   - age sozinho: acende/apaga e ajusta o brilho da lâmpada, dispara
 *     alarme e registra alertas, sem comando manual.
 */
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "modelo_arvore.h"

// ---------- Pinos ----------
const int PINO_LDR_AMBIENTE = 34;  // LDR virado para o céu
const int PINO_LDR_LAMPADA  = 35;  // LDR dentro da luminária (mede só a lâmpada)
const int PINO_TENSAO_REDE  = 32;  // potenciômetro simula o sensor de tensão (0-250 V)
const int PINO_PIR          = 27;  // sensor de presença
const int PINO_LAMPADA      = 25;  // LED = lâmpada do poste (brilho por PWM)
const int PINO_BUZZER       = 26;  // alarme local

// ---------- Parâmetros ----------
const float LUX_NOITE          = 50.0;   // abaixo disso é noite
const float TENSAO_MINIMA      = 180.0;  // abaixo disso a rede caiu
const int   BRILHO_ECONOMIA    = 70;     // ~27% quando a rua está vazia
const int   BRILHO_TOTAL       = 255;    // 100% quando há gente passando
const unsigned long INTERVALO_LEITURA_MS = 500;
const int   LEITURAS_CONFIRMACAO = 3;    // estado só muda após 3 leituras iguais

// Estados (mesma ordem de NOMES_ESTADO em modelo_arvore.h)
enum Estado { NORMAL, LAMPADA_QUEIMADA, ACESA_DE_DIA, QUEDA_ENERGIA,
              POSTE_INCLINADO, POSTE_CAIDO };

Adafruit_MPU6050 mpu;
Adafruit_SSD1306 tela(128, 64, &Wire, -1);
bool mpuOk = false, telaOk = false;

int estadoConfirmado = NORMAL;
int estadoCandidato  = NORMAL;
int contagemCandidato = 0;
unsigned long ultimaLeitura = 0;
unsigned long totalAlertas = 0;

// ---------- Leitura dos sensores ----------

// Converte a leitura do módulo LDR do Wokwi em lux (fórmula da documentação do Wokwi)
float lerLux(int pino) {
  const float GAMMA = 0.7, RL10 = 50.0;
  float tensao = analogRead(pino) / 4095.0 * 3.3;
  if (tensao >= 3.29) tensao = 3.29;                 // evita divisão por zero
  float resistencia = 2000.0 * tensao / (1.0 - tensao / 3.3);
  if (resistencia < 1.0) return 100000.0;            // luz máxima
  return pow(RL10 * 1e3 * pow(10, GAMMA) / resistencia, 1.0 / GAMMA);
}

float lerTensaoRede() {
  return analogRead(PINO_TENSAO_REDE) / 4095.0 * 250.0;
}

// Ângulo entre o eixo do poste (Z) e a vertical, em graus
float lerInclinacao() {
  if (!mpuOk) return 0;
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float ax = a.acceleration.x, ay = a.acceleration.y, az = a.acceleration.z;
  float modulo = sqrt(ax * ax + ay * ay + az * az);
  if (modulo < 0.1) return 0;
  return acos(constrain(az / modulo, -1.0, 1.0)) * 180.0 / PI;
}

// ---------- Ações autônomas ----------

void controlarLampada(int estado, float luxAmbiente, float tensao, bool presenca) {
  bool noite = luxAmbiente < LUX_NOITE;
  bool podeAcender = noite && tensao >= TENSAO_MINIMA && estado != POSTE_CAIDO;
  int brilho = 0;
  if (podeAcender) brilho = presenca ? BRILHO_TOTAL : BRILHO_ECONOMIA;
  analogWrite(PINO_LAMPADA, brilho);
}

void tocarAlarme(int estado) {
  switch (estado) {
    case POSTE_CAIDO:     tone(PINO_BUZZER, 2000, 800); break;  // emergência
    case POSTE_INCLINADO: tone(PINO_BUZZER, 1500, 300); break;
    case QUEDA_ENERGIA:   tone(PINO_BUZZER, 1000, 150); break;
    default: break;  // falhas de lâmpada não precisam de alarme na rua
  }
}

// Registro estruturado (JSON) - é o que seria enviado à central/prefeitura
void registrarEvento(int estado, float luxA, float luxL, float tensao,
                     float incl, bool presenca) {
  Serial.printf(
    "{\"evento\":\"%s\",\"t_ms\":%lu,\"lux_ambiente\":%.1f,\"lux_lampada\":%.1f,"
    "\"tensao_rede\":%.0f,\"inclinacao\":%.1f,\"presenca\":%s,\"alertas\":%lu}\n",
    NOMES_ESTADO[estado], millis(), luxA, luxL, tensao, incl,
    presenca ? "true" : "false", totalAlertas);
}

void atualizarTela(int estado, float luxA, float luxL, float tensao,
                   float incl, bool presenca) {
  if (!telaOk) return;
  tela.clearDisplay();
  tela.setTextColor(SSD1306_WHITE);
  tela.setTextSize(1);
  tela.setCursor(0, 0);
  tela.println("POSTE SENTINELA #001");
  tela.drawLine(0, 9, 127, 9, SSD1306_WHITE);
  tela.setCursor(0, 13);
  tela.printf("Amb:%6.0flx Lamp:%4.0f\n", luxA, luxL);
  tela.printf("Rede:%3.0fV  Incl:%4.1fg\n", tensao, incl);
  tela.printf("Pessoas: %s\n", presenca ? "SIM" : "nao");
  tela.drawLine(0, 39, 127, 39, SSD1306_WHITE);
  tela.setCursor(0, 44);
  tela.println(estado == NORMAL ? "Status: OK" : "!! ALERTA !!");
  tela.println(NOMES_ESTADO[estado]);
  tela.display();
}

// ---------- Programa principal ----------

void setup() {
  Serial.begin(115200);
  pinMode(PINO_PIR, INPUT);
  pinMode(PINO_LAMPADA, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);

  mpuOk = mpu.begin();
  if (!mpuOk) Serial.println("{\"erro\":\"MPU6050 nao encontrado - inclinacao = 0\"}");

  telaOk = tela.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!telaOk) Serial.println("{\"erro\":\"Display OLED nao encontrado\"}");

  Serial.println("{\"evento\":\"INICIADO\",\"poste\":\"001\"}");
}

void loop() {
  if (millis() - ultimaLeitura < INTERVALO_LEITURA_MS) return;
  ultimaLeitura = millis();

  // 1. Percepção
  float luxA   = lerLux(PINO_LDR_AMBIENTE);
  float luxL   = lerLux(PINO_LDR_LAMPADA);
  float tensao = lerTensaoRede();
  float incl   = lerInclinacao();
  bool presenca = digitalRead(PINO_PIR) == HIGH;

  // 2. Interpretação pela IA (árvore de decisão embarcada)
  int previsto = classificarEstado(luxA, luxL, tensao, incl);

  // 3. Confirmação: evita alarme falso por leitura isolada com ruído
  if (previsto == estadoCandidato) {
    contagemCandidato++;
  } else {
    estadoCandidato = previsto;
    contagemCandidato = 1;
  }
  if (contagemCandidato >= LEITURAS_CONFIRMACAO && estadoCandidato != estadoConfirmado) {
    estadoConfirmado = estadoCandidato;
    if (estadoConfirmado != NORMAL) totalAlertas++;
    registrarEvento(estadoConfirmado, luxA, luxL, tensao, incl, presenca);
    tocarAlarme(estadoConfirmado);
  }

  // 4. Ação autônoma
  controlarLampada(estadoConfirmado, luxA, tensao, presenca);
  atualizarTela(estadoConfirmado, luxA, luxL, tensao, incl, presenca);
}
