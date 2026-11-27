/* teste_corrente.ino — Teste EPS + ADCS: motor a +0,2 A por 5 s, depois -0,2 A por 5 s
 *
 * Equipe: POLISAT · Subsistema: ADCS · Cubedesign 2026
 *
 * Sketch SEPARADO do firmware principal: mesmo driver do ESCON (cópia de adcs_firmware/escon.h/.cpp),
 * mesmos pinos da PCB_LASC_v1.0 e a MESMA configuração do ESCON do detumbling (Current Controller,
 * PWM 10 % = 0 A, 90 % = 2,0 A). Nada muda no ESCON Studio.
 *
 * Perfil (uma vez por "s"):
 *   (sentido: o mesmo do firmware, INVERT_DIR = true; "+" é o sentido positivo do detumbling)
 *   ESPERA  0 A, ESCON habilitado por 0,5 s (PWM em zero antes de qualquer corrente)
 *   FASE +  +I_TESTE_A por T_FASE_MS  (torque +Kt·I na roda)
 *   FASE -  -I_TESTE_A por T_FASE_MS
 *   FIM     0 A e ESCON desabilitado (roda livre; para pelo atrito)
 *
 * Com 0,2 A: torque 1,18 mN·m, a roda acelera ~40 rad/s² (sem atrito ~1900 rpm em 5 s). Na fase -,
 * o mesmo torque ao contrário freia e, como o atrito ajuda a frear, a roda passa de 0 e termina
 * girando ao contrário (~-500 rpm com o atrito estimado de 0,2 mN·m). Isso é esperado.
 * Resolução do PWM: ~9,8 mA por passo, então 0,2 A sai como 0,196 A.
 *
 * Monitor serial: 115200, "Nova linha". Comandos:
 *   s  inicia o perfil          x  aborta (0 A e desabilita na hora)          ?  parâmetros
 *
 * Telemetria (10 Hz): D,t_ms,fase,i_mA,duty,dir,ready,enable
 *   fase: 0 parado, 1 espera, 2 positiva, 3 negativa
 *   i_mA = corrente COMANDADA (o ESCON fecha a malha; a corrente real depende dele e da alimentação)
 * Eventos: "# ..." (início e fim de cada fase, com o tempo em ms desde o "s").
 */
#include "config.h"
#include "escon.h"

enum Fase : uint8_t { PARADO = 0, ESPERA = 1, POSITIVA = 2, NEGATIVA = 3 };
const uint32_t ESPERA_MS = 500;

Escon escon;
Fase fase = PARADO;
uint32_t inicioMs = 0, faseMs = 0, ultTelemMs = 0;
float iCmd = 0.0f;   // [A] comando atual com sinal
bool autoPendente = AUTO_INICIO;

const int DUTY_ZERO = (255L * DUTY_MIN_PCT) / 100;
const int DUTY_FULL = (255L * DUTY_MAX_PCT) / 100;

int dutyDe(float i) {
  float f = fabs(i) / DRIVER_I_FS;
  return constrain(DUTY_ZERO + (int)(f * (DUTY_FULL - DUTY_ZERO) + 0.5f), DUTY_ZERO, DUTY_FULL);
}

void comandaCorrente(float i) {
  float tau = constrain(i * MOTOR_KT, -TAU_CMD_MAX, TAU_CMD_MAX);
  iCmd = tau / MOTOR_KT;
  escon.setTorque(tau);
}

void evento(const __FlashStringHelper *msg) {
  Serial.print(F("# t=")); Serial.print(millis() - inicioMs); Serial.print(F(" ms ")); Serial.println(msg);
}

void entra(Fase f) {
  fase = f;
  faseMs = millis();
  switch (f) {
    case ESPERA:   comandaCorrente(0.0f);        evento(F("ESCON habilitado, 0 A")); break;
    case POSITIVA: comandaCorrente(+I_TESTE_A);  evento(F("fase +: +0,2 A")); break;
    case NEGATIVA: comandaCorrente(-I_TESTE_A);  evento(F("fase -: -0,2 A")); break;
    default: break;
  }
}

void para(const __FlashStringHelper *msg) {
  escon.disable();
  iCmd = 0.0f;
  fase = PARADO;
  evento(msg);
}

void inicia() {
  if (fase != PARADO) { Serial.println(F("# ja esta rodando ('x' aborta)")); return; }
  autoPendente = false;
  inicioMs = millis();
  escon.enable();   // PWM em zero antes do Enable (recomendação maxon)
  entra(ESPERA);
}

void maquina() {
  if (fase == PARADO) return;
  uint32_t dt = millis() - faseMs;
  if (fase == ESPERA && dt >= ESPERA_MS) entra(POSITIVA);
  else if (fase == POSITIVA && dt >= T_FASE_MS) entra(NEGATIVA);
  else if (fase == NEGATIVA && dt >= T_FASE_MS) para(F("fim: 0 A, ESCON desabilitado (roda livre)"));
  if (escon.readyFault()) para(F("ABORTADO: ESCON sem Ready (falha)"));
}

void telemetria() {
  if (millis() - ultTelemMs < TELEM_MS) return;
  ultTelemMs = millis();
  Serial.print(F("D,"));  Serial.print(fase == PARADO ? 0 : millis() - inicioMs);
  Serial.print(',');      Serial.print((int)fase);
  Serial.print(',');      Serial.print((int)(iCmd * 1000.0f));
  Serial.print(',');      Serial.print(escon.enabled() ? dutyDe(iCmd) : DUTY_ZERO);
  Serial.print(',');      Serial.print(digitalRead(PIN_ESCON_DIR));
  Serial.print(',');      Serial.print(escon.ready() ? 1 : 0);
  Serial.print(',');      Serial.println(escon.enabled() ? 1 : 0);
}

void parametros() {
  Serial.print(F("# perfil: +")); Serial.print(I_TESTE_A, 3); Serial.print(F(" A por ")); Serial.print(T_FASE_MS);
  Serial.print(F(" ms, -")); Serial.print(I_TESTE_A, 3); Serial.print(F(" A por ")); Serial.print(T_FASE_MS);
  Serial.print(F(" ms · duty ")); Serial.print(dutyDe(I_TESTE_A)); Serial.print(F("/255 (zero = "));
  Serial.print(DUTY_ZERO); Serial.print(F(") · torque ")); Serial.print(I_TESTE_A * MOTOR_KT * 1000.0f, 3);
  Serial.println(F(" mN.m"));
}

void setup() {
  Serial.begin(DEBUG_BAUD);
  escon.begin();   // PWM em 10 % (0 A) e Enable LOW
  Serial.println(F("\n# teste_corrente: EPS + ADCS. Comandos: s inicia, x aborta, ? parametros"));
  Serial.println(F("# telemetria: D,t_ms,fase,i_mA,duty,dir,ready,enable"));
  parametros();
  if (AUTO_INICIO) { Serial.print(F("# inicio automatico em ")); Serial.print(AUTO_ATRASO_MS); Serial.println(F(" ms")); }
}

void loop() {
  static char linha[16]; static uint8_t n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') { if (n < sizeof(linha) - 1) linha[n++] = c; continue; }
    linha[n] = '\0'; n = 0;
    if (!strcmp(linha, "s")) inicia();
    else if (!strcmp(linha, "x")) { autoPendente = false; if (fase != PARADO) para(F("ABORTADO pelo comando x")); else escon.disable(); }
    else if (!strcmp(linha, "?")) parametros();
    else if (linha[0]) Serial.println(F("# comandos: s, x, ?"));
  }
  if (autoPendente && millis() >= AUTO_ATRASO_MS) inicia();
  maquina();
  telemetria();
}
