/* adcs_firmware.ino — Firmware ADCS do CubeSat POLISAT (Arduino Nano)
 *
 * Equipe: POLISAT · Subsistema: ADCS · Cubedesign 2026
 *
 * Constantes e pinagem: config.h
 *
 * DEPURAÇÃO (USB, 115200): comandos '2' / '1', 'C' calibra o giroscópio
 * (parado, modo 1), '?' imprime o estado. No modo 2 sai uma linha CSV a
 * 10 Hz:  t[s], omega[rpm], torque[mN·m]
 */
#include "config.h"
#include "gyro_mpu9250.h"
#include "escon.h"
#include "detumbling.h"

GyroMPU9250 gyro;
Escon escon;
Detumbling detumbling;

uint32_t proximoCicloUs = 0;
uint32_t ultimoDebugMs  = 0;

const float RADS_TO_RPM = 9.5493f;

// Coloca todo pino que mexe em hardware externo num estado seguro: motor
// desabilitado com comando zero, LDRs desligados.
void pinosSeguros() {
  escon.begin();

  pinMode(PIN_LDR_BANK_A, OUTPUT);
  digitalWrite(PIN_LDR_BANK_A, LOW);
  pinMode(PIN_LDR_BANK_B, OUTPUT);
  digitalWrite(PIN_LDR_BANK_B, LOW);
}

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

void imprimirEstado() {
  Serial.print(F("# modo="));       Serial.print(detumbling.active() ? 2 : 1);
  Serial.print(F(" giro_ok="));     Serial.print(detumbling.sensorOk());
  Serial.print(F(" omega_rpm="));   Serial.print(detumbling.omega() * RADS_TO_RPM, 2);
  Serial.print(F(" na_faixa="));    Serial.print(detumbling.inRange());
  Serial.print(F(" t_faixa_s="));   Serial.print(detumbling.timeToRangeMs() * 0.001f, 2);
  Serial.print(F(" escon_ready=")); Serial.print(escon.ready());
  Serial.print(F(" Kp="));          Serial.println(KP, 6);
}

// Executa um comando de 1 byte, venha ele da UART do OBC ou do USB.
void executarComando(char c) {
  switch (c) {
    case '2': detumbling.start(); break;
    case '1': detumbling.stop(F("comando 1")); break;
    case 'C':
    case 'c':
      if (detumbling.active()) Serial.println(F("ERRO: saia do modo 2 antes de calibrar"));
      else calibrarGyro();
      break;
    case '?': imprimirEstado(); break;
    default: break;   // CR, LF e ruído
  }
}

void setup() {
  pinosSeguros();   // antes de qualquer outra coisa
  Serial.begin(DEBUG_BAUD);
  Serial.println(F("# ADCS POLISAT: pinos em estado seguro"));

  bool gyroOk = gyro.begin(MPU_ADDR);
  if (!gyroOk) {
    Serial.println(F("ERRO: MPU-9250 nao respondeu no I2C; detumbling bloqueado"));
  } else {
    Serial.print(F("# MPU WHO_AM_I = 0x"));
    Serial.println(gyro.whoAmI(), HEX);
    calibrarGyro();
  }
  detumbling.begin(&gyro, &escon, gyroOk);

  Serial.println(F("# comandos: 2=detumbling  1=sai  C=calibra  ?=estado"));
  proximoCicloUs = micros();
}

void loop() {
  while (Serial.available() > 0) executarComando(Serial.read());

  // Cadência fixa de 100 Hz (delay(10) atrasaria pelo tempo do próprio ciclo).
  uint32_t agoraUs = micros();
  if ((int32_t)(agoraUs - proximoCicloUs) < 0) return;
  proximoCicloUs += CONTROL_PERIOD_US;
  // Se um ciclo atrasou demais (ex.: calibração), recomeça a cadência.
  if ((int32_t)(agoraUs - proximoCicloUs) > (int32_t)(5 * CONTROL_PERIOD_US)) {
    proximoCicloUs = agoraUs + CONTROL_PERIOD_US;
  }

  detumbling.update();

  uint32_t agoraMs = millis();
  if (detumbling.active() && agoraMs - ultimoDebugMs >= DEBUG_PERIOD_MS) {
    ultimoDebugMs = agoraMs;
    Serial.print(detumbling.elapsedMs() * 0.001f, 2);        Serial.print(',');
    Serial.print(detumbling.omega() * RADS_TO_RPM, 3);       Serial.print(',');
    Serial.println(detumbling.torque() * 1000.0f, 3);
  }
}
