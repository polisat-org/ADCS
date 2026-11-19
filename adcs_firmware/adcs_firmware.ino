/* adcs_firmware.ino — Firmware ADCS do CubeSat POLISAT (Arduino Nano)
 *
 * Equipe: POLISAT · Subsistema: ADCS · Cubedesign 2026
 *
 * Constantes e pinagem: config.h
 */
#include "config.h"

// Coloca todo pino que mexe em hardware externo num estado seguro: motor
// desabilitado com comando zero, LDRs desligados.
void pinosSeguros() {
  pinMode(PIN_ESCON_ENABLE, OUTPUT);
  digitalWrite(PIN_ESCON_ENABLE, LOW);
  pinMode(PIN_ESCON_DIR, OUTPUT);
  digitalWrite(PIN_ESCON_DIR, LOW);
  pinMode(PIN_ESCON_PWM, OUTPUT);
  analogWrite(PIN_ESCON_PWM, (255L * DUTY_MIN_PCT) / 100);
  pinMode(PIN_ESCON_READY, INPUT_PULLUP);

  pinMode(PIN_LDR_BANK_A, OUTPUT);
  digitalWrite(PIN_LDR_BANK_A, LOW);
  pinMode(PIN_LDR_BANK_B, OUTPUT);
  digitalWrite(PIN_LDR_BANK_B, LOW);
}

void setup() {
  pinosSeguros();
  Serial.begin(DEBUG_BAUD);
  Serial.println(F("# ADCS POLISAT: pinos em estado seguro"));
}

void loop() {
}
