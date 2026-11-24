/* teste_pwm_motor.ino — Teste da roda de reação por PWM (malha aberta)
 *
 * Equipe: POLISAT · Subsistema: ADCS · Cubedesign 2026
 *
 * Sketch SEPARADO do firmware principal: só comanda o motor pelo monitor
 * serial, sem giroscópio, LDRs ou OBC. Usa o MESMO driver do firmware
 * (escon.h / escon.cpp, copiados sem mudança de adcs_firmware/) e as mesmas
 * constantes (config.h é um recorte de adcs_firmware/config.h).
 *
 * MALHA ABERTA: o ESCON está em Current Controller, então o comando define
 * a CORRENTE (torque) na roda, não a velocidade. Com comando fixo a roda
 * acelera até o limite do ESCON. Todo comando volta sozinho para zero depois
 * de TEMPO_MAX_MS. Rode com o satélite PRESO na bancada.
 *
 * Gravar: abra este .ino no Arduino IDE (a pasta inteira compila junto),
 * Board = Arduino Nano, Processor = ATmega328P (Old Bootloader nos clones).
 *
 * Monitor serial: 115200, final de linha "Nova linha" (ou "Ambos NL & CR").
 *   e          habilita o ESCON (PWM em zero antes do Enable)
 *   d          desabilita (comando zero + Enable LOW)
 *   -50..50    corrente em % do fundo de escala (2,0 A); o sinal é o sentido.
 *              Ex.: 20 -> 0,4 A no sentido +, -20 -> 0,4 A no sentido -
 *   0          comando zero (PWM 10 %): a roda fica em roda livre
 *   ?          estado
 */
#include "config.h"
#include "escon.h"

Escon escon;
int cmdPct = 0;               // comando atual em % do fundo de escala
uint32_t cmdInicioMs = 0;

// Mesma conta de Escon::setTorque, só para mostrar o duty no monitor.
int dutyCounts(float tau) {
  const int zero = (255L * DUTY_MIN_PCT) / 100;
  const int full = (255L * DUTY_MAX_PCT) / 100;
  return zero + (int)(fabs(tau) / TAU_FS * (full - zero) + 0.5f);
}

void aplicar(int pct) {
  cmdPct = constrain(pct, -(int)CMD_MAX_PCT, (int)CMD_MAX_PCT);
  escon.setTorque(cmdPct * TAU_FS / 100.0f);
  cmdInicioMs = millis();
}

void estado() {
  float tau = escon.lastTorque();
  int duty = dutyCounts(tau);
  Serial.print(F("# ligado="));      Serial.print(escon.enabled());
  Serial.print(F(" cmd="));          Serial.print(cmdPct);
  Serial.print(F("% I="));           Serial.print(fabs(tau) / MOTOR_KT, 3);
  Serial.print(F("A tau="));         Serial.print(tau * 1000.0f, 2);
  Serial.print(F("mNm duty="));      Serial.print(duty * 100.0f / 255.0f, 1);
  Serial.print(F("% ("));            Serial.print(duty);
  Serial.print(F("/255) dir="));     Serial.print(digitalRead(PIN_ESCON_DIR));
  Serial.print(F(" ready="));        Serial.println(escon.ready());
}

void comando(const char *linha) {
  if (linha[0] == '\0') return;
  switch (linha[0]) {
    case 'e': case 'E':
      escon.enable();
      cmdPct = 0;
      Serial.println(F("# ESCON habilitado (comando zero)"));
      return;
    case 'd': case 'D':
      escon.disable();
      cmdPct = 0;
      Serial.println(F("# ESCON desabilitado"));
      return;
    case '?':
      estado();
      return;
  }

  char *fim;
  long v = strtol(linha, &fim, 10);
  if (fim == linha || *fim != '\0') {
    Serial.print(F("ERRO: comando desconhecido: "));
    Serial.println(linha);
    return;
  }
  if (!escon.enabled()) {
    Serial.println(F("ERRO: ESCON desabilitado; mande 'e' antes"));
    return;
  }
  if (labs(v) > CMD_MAX_PCT) {
    Serial.print(F("# limitado a +/-"));
    Serial.print(CMD_MAX_PCT);
    Serial.println(F(" % (CMD_MAX_PCT em config.h)"));
    v = constrain(v, -(long)CMD_MAX_PCT, (long)CMD_MAX_PCT);
  }
  aplicar((int)v);
  estado();
}

void setup() {
  escon.begin();               // estado seguro: Enable LOW, PWM em 10 %
  Serial.begin(DEBUG_BAUD);
  Serial.println(F("# teste PWM do motor (malha aberta, ESCON em Current Controller)"));
  Serial.print(F("# comandos: e  d  ?  -"));
  Serial.print(CMD_MAX_PCT);   Serial.print(F("..")); Serial.print(CMD_MAX_PCT);
  Serial.print(F(" (% de "));  Serial.print(DRIVER_I_FS, 1);
  Serial.print(F(" A; volta a zero apos "));
  Serial.print(TEMPO_MAX_MS / 1000.0f, 1);
  Serial.println(F(" s)"));
}

void loop() {
  static char buf[12];
  static uint8_t n = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      buf[n] = '\0';
      comando(buf);
      n = 0;
    } else if (n < sizeof(buf) - 1) {
      buf[n++] = c;
    }
  }

  if (cmdPct != 0 && millis() - cmdInicioMs >= TEMPO_MAX_MS) {
    aplicar(0);
    Serial.println(F("# tempo maximo: comando voltou a zero"));
  }

  if (escon.readyFault()) {
    escon.disable();
    cmdPct = 0;
    Serial.println(F("ERRO: ESCON sem Ready; motor desabilitado"));
  }
}
