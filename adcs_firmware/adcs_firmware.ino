/* adcs_firmware.ino — Firmware ADCS do CubeSat POLISAT (Arduino Nano)
 *
 * Equipe: POLISAT · Subsistema: ADCS · Cubedesign 2026
 *
 * Constantes e pinagem: config.h · Protocolo com o OBC: obc_link.h
 *
 * OBC (UART D12/D11): comandos como inteiros de 1 byte
 *   1 = inicia detumbling   2 = encerra detumbling
 *   3 = inicia busca de azimute (envia "270.5" a cada 1 s enquanto houver luz)
 *   4 = encerra busca
 *
 * DEPURAÇÃO (USB, 115200): os mesmos comandos como dígitos ASCII '1'..'4',
 * mais 'C' (calibra o giroscópio, parado e sem detumbling), '?' (estado) e
 * 'L' (lê e imprime os LDRs). Com o detumbling ativo sai uma linha CSV a
 * 10 Hz:  t[s], omega[rpm], torque[mN·m]
 */
#include "config.h"
#include "gyro_mpu9250.h"
#include "escon.h"
#include "detumbling.h"
#include "sun_sensor.h"
#include "obc_link.h"

GyroMPU9250 gyro;
Escon escon;
Detumbling detumbling;
SunSensor sun;
ObcLink obc;

bool     buscandoAzimute = false;
uint32_t proximoCicloUs  = 0;
uint32_t ultimoDebugMs   = 0;
uint32_t ultimaAtitudeMs = 0;
uint32_t ultimoEnvioMs   = 0;
float    ultimoAzEnviado = ATT_INVALID;
uint32_t ultimoEstadoMs  = 0;
bool     estadoPendente  = true;   // manda o estado logo ao ligar (avisa o OBC que subiu)

const float RADS_TO_RPM = 9.5493f;

// Coloca todo pino que mexe em hardware externo num estado seguro: motor
// desabilitado com comando zero, LDRs desligados.
void pinosSeguros() {
  escon.begin();
  sun.begin();
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

void iniciarBusca() {
  if (buscandoAzimute) return;
  float descarta;
  sun.takeAverage(descarta);   // zera o acumulador: só valem amostras novas
  buscandoAzimute = true;
  ultimoEnvioMs = millis();
  Serial.println(F("# busca de azimute ATIVA"));
}

void pararBusca() {
  if (!buscandoAzimute) return;
  buscandoAzimute = false;
  Serial.println(F("# busca de azimute PARADA (comando 4)"));
}

void imprimirEstado() {
  char az[AZ_MSG_LEN + 1];
  Serial.print(F("# detumbling="));  Serial.print(detumbling.active());
  Serial.print(F(" busca_az="));     Serial.print(buscandoAzimute);
  Serial.print(F(" giro_ok="));      Serial.print(detumbling.sensorOk());
  Serial.print(F(" omega_rpm="));    Serial.print(detumbling.omega() * RADS_TO_RPM, 2);
  Serial.print(F(" na_faixa="));     Serial.print(detumbling.inRange());
  Serial.print(F(" t_faixa_s="));    Serial.print(detumbling.timeToRangeMs() * 0.001f, 2);
  Serial.print(F(" escon_ready="));  Serial.print(escon.ready());
  Serial.print(F(" ultimo_az="));
  if (ultimoAzEnviado == ATT_INVALID) {
    Serial.println(F("nenhum"));
  } else {
    ObcLink::formatAzimuth(ultimoAzEnviado, az);
    Serial.println(az);
  }
}

// Diagnóstico dos LDRs: tensão [V] e leitura normalizada pela calibração.
// Para recalibrar: mesma luz em cada face, anote V e troque LDR_CAL.
void imprimirLdrs() {
  static const char *const nomes[4] = {"FRONTAL", "DIREITO", "TRASEIRO", "ESQUERDO"};
  sun.sample(false);   // não acumula: o L é só diagnóstico
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print(F("# "));   Serial.print(nomes[i]);
    Serial.print(F(" V="));  Serial.print(sun.volts(i), 3);
    Serial.print(F(" n="));  Serial.println(sun.norm(i), 3);
  }
  Serial.print(F("# x="));   Serial.print(sun.x(), 3);
  Serial.print(F(" y="));    Serial.print(sun.y(), 3);
  Serial.print(F(" az="));   Serial.print(sun.azimuthDeg(), 2);
  Serial.println(sun.valid() ? F(" deg") : F(" (luz insuficiente)"));
}

// Comandos do protocolo (1..4). Vêm do OBC como inteiro e do USB como dígito.
void executarComandoMissao(uint8_t cmd) {
  switch (cmd) {
    case CMD_DETUMBLING_INICIA: detumbling.start(); break;
    case CMD_DETUMBLING_PARA:   detumbling.stop(F("comando 2")); break;
    case CMD_AZIMUTE_INICIA:    iniciarBusca(); break;
    case CMD_AZIMUTE_PARA:      pararBusca(); break;
    default:                    break;
  }
}

// Recuperação pelo OBC (comando 5): motor desligado e reinício do firmware. Sem watchdog de propósito:
// o bootloader antigo dos Nano (ATmegaBOOT) entra em laço de reinício quando o watchdog dispara.
void reiniciar() {
  detumbling.stop(F("comando 5: reinicio pelo OBC"));
  Serial.println(F("# reiniciando (comando 5 do OBC)"));
  Serial.flush();
  asm volatile("jmp 0");
}

// Do OBC só valem os inteiros 1..4. Zeros extras do vetor comandoADCS e
// qualquer outro byte são ignorados (nada de comando de depuração por aqui:
// um byte corrompido não pode, por exemplo, disparar uma calibração).
void comandoObc(uint8_t b) {
  if (b >= CMD_DETUMBLING_INICIA && b <= CMD_REINICIA) {
    Serial.print(F("# OBC: comando "));
    Serial.println(b);
    if (b == CMD_REINICIA) reiniciar();
    executarComandoMissao(b);
    estadoPendente = true;            // confirma o comando no próximo ciclo livre
  }
}

void comandoUsb(char c) {
  if (c >= '1' && c <= '4') {
    executarComandoMissao(c - '0');
    return;
  }
  switch (c) {
    case 'C':
    case 'c':
      if (detumbling.active()) Serial.println(F("ERRO: encerre o detumbling (2) antes de calibrar"));
      else calibrarGyro();
      break;
    case '?': imprimirEstado(); break;
    case 'L':
    case 'l': imprimirLdrs(); break;
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
  obc.begin();

  Serial.println(F("# comandos: 1/2=inicia/encerra detumbling  3/4=inicia/encerra azimute  C ? L"));
  proximoCicloUs = micros();
}

void loop() {
  int c;
  while ((c = obc.readCommand()) >= 0) comandoObc((uint8_t)c);
  while (Serial.available() > 0) comandoUsb(Serial.read());

  // Cadência fixa de 100 Hz (delay(10) atrasaria pelo tempo do próprio ciclo).
  uint32_t agoraUs = micros();
  if ((int32_t)(agoraUs - proximoCicloUs) < 0) return;
  proximoCicloUs += CONTROL_PERIOD_US;
  // Se um ciclo atrasou demais (ex.: calibração), recomeça a cadência.
  if ((int32_t)(agoraUs - proximoCicloUs) > (int32_t)(5 * CONTROL_PERIOD_US)) {
    proximoCicloUs = agoraUs + CONTROL_PERIOD_US;
  }

  // O controle roda primeiro no ciclo.
  detumbling.update();

  uint32_t agoraMs = millis();
  bool enviouNoCiclo = false;   // LDR ou azimute já usaram este ciclo de 10 ms
  if (buscandoAzimute) {
    // Num ciclo: OU lê os LDRs (~4.5 ms) OU envia o azimute (~5.2 ms), para
    // não estourar os 10 ms. Se os dois vencem juntos, o envio vai no próximo.
    bool leuLdr = false;
    if (agoraMs - ultimaAtitudeMs >= ATT_SAMPLE_PERIOD_MS) {
      ultimaAtitudeMs = agoraMs;
      sun.sample();
      leuLdr = true;
      enviouNoCiclo = true;
    }
    if (!leuLdr && agoraMs - ultimoEnvioMs >= AZ_TX_PERIOD_MS) {
      ultimoEnvioMs = agoraMs;
      float az;
      if (sun.takeAverage(az)) {
        obc.sendAzimuth(az);
        enviouNoCiclo = true;
        ultimoAzEnviado = az;
        char txt[AZ_MSG_LEN + 1];
        ObcLink::formatAzimuth(az, txt);
        Serial.print(F("# azimute enviado: "));
        Serial.println(txt);
      } else {
        // Sem luz suficiente: nada vai para o OBC; a busca continua.
        Serial.println(F("# buscando azimute: luz insuficiente"));
      }
    }
  }

  // Estado para o OBC (confirmação de comando e heartbeat de 2 s), num ciclo sem LDR nem azimute,
  // para caber nos 10 ms do loop. A variável enviouNoCiclo vem do bloco da busca acima.
  if (!enviouNoCiclo && (estadoPendente || agoraMs - ultimoEstadoMs >= ESTADO_PERIOD_MS)) {
    obc.sendStatus(detumbling.active(), buscandoAzimute, detumbling.falha());
    ultimoEstadoMs = agoraMs;
    estadoPendente = false;
  }

  if (detumbling.active() && agoraMs - ultimoDebugMs >= DEBUG_PERIOD_MS) {
    ultimoDebugMs = agoraMs;
    Serial.print(detumbling.elapsedMs() * 0.001f, 2);        Serial.print(',');
    Serial.print(detumbling.omega() * RADS_TO_RPM, 3);       Serial.print(',');
    Serial.println(detumbling.torque() * 1000.0f, 3);
  }
}
