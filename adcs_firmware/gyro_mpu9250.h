/* gyro_mpu9250.h — Driver mínimo do giroscópio do MPU-9250 (I2C)
 *
 * Fala direto com os registradores pela Wire, sem biblioteca externa. Só o
 * eixo Z (eixo da roda) é usado pelo controle.
 *
 * Configuração: ±500 °/s (60 rpm = 360 °/s estoura a faixa de ±250 °/s),
 * DLPF de 41 Hz, amostragem interna de 100 Hz (casa com o loop de controle).
 */
#pragma once
#include <Arduino.h>

class GyroMPU9250 {
public:
  // Inicializa o sensor. Retorna false se ele não responder no I2C.
  bool begin(uint8_t addr);

  // Lê ω_z e grava em rad/s (bias já descontado, sinal GYRO_Z_SIGN aplicado).
  // Retorna false em falha de I2C.
  bool readZ(float &omegaZ);

  // Média de 'amostras' leituras com o satélite PARADO. Recusa (e mantém o
  // bias anterior) se a leitura variar mais que 'maxSpan' rad/s.
  bool calibrateBias(uint16_t amostras, float maxSpan);

  float bias() const { return biasZ; }
  uint8_t whoAmI() const { return id; }

private:
  bool writeReg(uint8_t reg, uint8_t val);
  bool readRegs(uint8_t reg, uint8_t *buf, uint8_t n);
  bool readRawZ(int16_t &raw);

  uint8_t addr = 0x68;
  uint8_t id = 0;
  float biasZ = 0.0f;   // [rad/s]
};
