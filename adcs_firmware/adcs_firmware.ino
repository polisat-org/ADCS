/* adcs_firmware.ino — Firmware ADCS do CubeSat POLISAT (Arduino Nano)
 *
 * Equipe: POLISAT · Subsistema: ADCS · Cubedesign 2026
 *
 * Constantes e pinagem: config.h
 */
#include "config.h"
#include "gyro_mpu9250.h"
#include "escon.h"

// Coloca todo pino que mexe em hardware externo num estado seguro: motor
// desabilitado com comando zero, LDRs desligados.
Escon escon;

void pinosSeguros() {
  escon.begin();

  pinMode(PIN_LDR_BANK_A, OUTPUT);
  digitalWrite(PIN_LDR_BANK_A, LOW);
  pinMode(PIN_LDR_BANK_B, OUTPUT);
  digitalWrite(PIN_LDR_BANK_B, LOW);
}

GyroMPU9250 gyro;
bool gyroOk = false;

void calibrarGyro() {
  Serial.println(F("# calibrando bias do giroscopio (mantenha parado)..."));
  if (gyro.calibrateBias(GYRO_CAL_SAMPLES, GYRO_CAL_MAX_SPAN)) {
    Serial.print(F("# bias Z = "));
    Serial.print(gyro.bias() * RAD_TO_DEG, 3);
    Serial.println(F(" deg/s"));
  } else {
    Serial.println(F("ERRO: calibracao recusada (em movimento ou falha I2C); bias mantido"));
  }
}

void setup() {
  pinosSeguros();   // antes de qualquer outra coisa
  Serial.begin(DEBUG_BAUD);
  Serial.println(F("# ADCS POLISAT: pinos em estado seguro"));

  gyroOk = gyro.begin(MPU_ADDR);
  if (!gyroOk) {
    Serial.println(F("ERRO: MPU-9250 nao respondeu no I2C; detumbling bloqueado"));
  } else {
    Serial.print(F("# MPU WHO_AM_I = 0x"));
    Serial.println(gyro.whoAmI(), HEX);
    calibrarGyro();
  }
}

void loop() {
}
