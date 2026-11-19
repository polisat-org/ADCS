/* gyro_mpu9250.cpp — ver gyro_mpu9250.h */
#include "gyro_mpu9250.h"
#include "config.h"
#include <Wire.h>

// Registradores (MPU-9250 Register Map, rev. 1.6)
static const uint8_t REG_SMPLRT_DIV   = 0x19;
static const uint8_t REG_CONFIG       = 0x1A;
static const uint8_t REG_GYRO_CONFIG  = 0x1B;
static const uint8_t REG_GYRO_ZOUT_H  = 0x47;
static const uint8_t REG_PWR_MGMT_1   = 0x6B;
static const uint8_t REG_PWR_MGMT_2   = 0x6C;
static const uint8_t REG_WHO_AM_I     = 0x75;

static const float GYRO_LSB_PER_DPS = 65.5f;   // ±500 °/s
static const float RAW_TO_RADS = (PI / 180.0f) / GYRO_LSB_PER_DPS;

bool GyroMPU9250::writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool GyroMPU9250::readRegs(uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

bool GyroMPU9250::begin(uint8_t endereco) {
  addr = endereco;
  Wire.begin();
  Wire.setClock(400000);
  // Sem timeout, um ruído no barramento trava o Wire para sempre (com o motor
  // possivelmente ligado). 3 ms e reset automático do TWI.
  Wire.setWireTimeout(3000, true);

  if (!writeReg(REG_PWR_MGMT_1, 0x80)) return false;   // reset
  delay(100);
  if (!writeReg(REG_PWR_MGMT_1, 0x01)) return false;   // clock = PLL do giroscópio
  if (!writeReg(REG_PWR_MGMT_2, 0x00)) return false;   // todos os eixos ligados
  delay(10);

  if (!readRegs(REG_WHO_AM_I, &id, 1)) return false;
  // 0x71 = MPU-9250, 0x73 = MPU-9255, 0x70 = MPU-6500 (clones comuns). Um ID
  // diferente só gera aviso: os registradores do giroscópio são os mesmos.

  if (!writeReg(REG_CONFIG, 0x03)) return false;        // DLPF 41 Hz, Fs interno 1 kHz
  if (!writeReg(REG_SMPLRT_DIV, 9)) return false;       // 1 kHz / (1+9) = 100 Hz
  if (!writeReg(REG_GYRO_CONFIG, 0x08)) return false;   // ±500 °/s
  delay(50);   // tempo para o DLPF assentar
  return true;
}

bool GyroMPU9250::readRawZ(int16_t &raw) {
  uint8_t b[2];
  if (!readRegs(REG_GYRO_ZOUT_H, b, 2)) return false;
  raw = (int16_t)(((uint16_t)b[0] << 8) | b[1]);
  return true;
}

bool GyroMPU9250::readZ(float &omegaZ) {
  int16_t raw;
  if (!readRawZ(raw)) return false;
  omegaZ = GYRO_Z_SIGN * (raw * RAW_TO_RADS - biasZ);
  return true;
}

bool GyroMPU9250::calibrateBias(uint16_t amostras, float maxSpan) {
  float soma = 0.0f, minimo = 1e9f, maximo = -1e9f;
  for (uint16_t i = 0; i < amostras; i++) {
    int16_t raw;
    if (!readRawZ(raw)) return false;
    float w = raw * RAW_TO_RADS;
    soma += w;
    if (w < minimo) minimo = w;
    if (w > maximo) maximo = w;
    delay(10);   // uma amostra nova do sensor a cada 10 ms
  }
  if (maximo - minimo > maxSpan) return false;   // estava girando
  biasZ = soma / amostras;
  return true;
}
