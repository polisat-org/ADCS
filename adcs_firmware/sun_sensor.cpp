/* sun_sensor.cpp — ver sun_sensor.h */
#include "sun_sensor.h"
#include "config.h"

void SunSensor::begin() {
  // Referência analógica: a padrão do Nano (5 V). Não chamar
  // analogReference(DEFAULT): o nome DEFAULT não existe em todos os cores.
  pinMode(PIN_LDR_BANK_A, OUTPUT);
  digitalWrite(PIN_LDR_BANK_A, LOW);
  pinMode(PIN_LDR_BANK_B, OUTPUT);
  digitalWrite(PIN_LDR_BANK_B, LOW);
}

float SunSensor::readVolts(uint8_t pino) {
  analogRead(pino);   // descarta: a 1ª conversão após trocar o mux é imprecisa
  uint32_t soma = 0;
  for (uint8_t i = 0; i < LDR_OVERSAMPLE; i++) soma += analogRead(pino);
  return (soma / (float)LDR_OVERSAMPLE) * (ADC_VREF / 1023.0f);
}

float SunSensor::toDeg360(float yy, float xx) {
  float a = atan2(yy, xx) * RAD_TO_DEG;
  if (a < 0.0f) a += 360.0f;
  if (a >= 360.0f) a -= 360.0f;
  return a;
}

void SunSensor::sample(bool acumular) {
  // Banco A (Q1): FRONTAL em A0, DIREITO em A1. O banco B fica desligado,
  // senão os LDRs do mesmo nó analógico se somariam.
  digitalWrite(PIN_LDR_BANK_B, LOW);
  digitalWrite(PIN_LDR_BANK_A, HIGH);
  delayMicroseconds(LDR_SETTLE_US);
  v[LDR_FRONTAL] = readVolts(PIN_LDR_SENSE_0);
  v[LDR_DIREITO] = readVolts(PIN_LDR_SENSE_1);
  digitalWrite(PIN_LDR_BANK_A, LOW);

  // Banco B (Q2): TRASEIRO em A0, ESQUERDO em A1.
  digitalWrite(PIN_LDR_BANK_B, HIGH);
  delayMicroseconds(LDR_SETTLE_US);
  v[LDR_TRASEIRO] = readVolts(PIN_LDR_SENSE_0);
  v[LDR_ESQUERDO] = readVolts(PIN_LDR_SENSE_1);
  digitalWrite(PIN_LDR_BANK_B, LOW);

  for (uint8_t i = 0; i < 4; i++) {
    float r = v[i] / LDR_CAL[i];
    n[i] = (LDR_GAMMA == 1.0f) ? r : pow(r, 1.0f / LDR_GAMMA);
  }

  vx = n[LDR_FRONTAL] - n[LDR_TRASEIRO];
  vy = n[LDR_DIREITO] - n[LDR_ESQUERDO];
  ok = (sqrt(vx * vx + vy * vy) >= LDR_MIN_SIGNAL);
  az = ok ? toDeg360(vy, vx) : ATT_INVALID;

  if (acumular && ok) {
    somaX += vx;
    somaY += vy;
    nSomas++;
  }
}

bool SunSensor::takeAverage(float &azDeg) {
  if (nSomas == 0) {
    azDeg = ATT_INVALID;
    return false;
  }
  float mx = somaX / nSomas, my = somaY / nSomas;
  somaX = somaY = 0;
  nSomas = 0;
  if (sqrt(mx * mx + my * my) < LDR_MIN_SIGNAL) {
    azDeg = ATT_INVALID;
    return false;
  }
  azDeg = toDeg360(my, mx);
  return true;
}
