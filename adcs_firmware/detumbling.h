/* detumbling.h — Controle de detumbling no eixo Z (roda de reação)
 *
 * LEI DE CONTROLE (igual a params.m / detumbling_sim.m)
 *   τ_roda = Kp · ω_corpo        [N·m]
 *   O corpo recebe −τ_roda, logo ω(t) = ω0 · exp(−Kp/I_b · t).
 *
 * SEMÂNTICA
 *   Ativo (comando 1 do OBC): lê o giroscópio a 100 Hz e comanda a roda para
 *   levar e MANTER |ω| na faixa (≤ 5 rpm) até o comando 2. Não há timeout nem
 *   aviso ao OBC: quem decide o fim é a prova. Atingir a faixa não desliga o
 *   controle: sem ele, o atrito da roda devolve momento ao corpo.
 *   Inativo: motor desabilitado; o giroscópio continua sendo lido para a
 *   telemetria de depuração.
 *
 * SEGURANÇA (desliga o motor e fica inativo)
 *   - |ω| > OMEGA_ABORT por OVERSPEED_SAMPLES seguidas: o controle está
 *     acelerando o corpo (sentido da roda ou do giroscópio invertido)
 *   - GYRO_MAX_FAILS leituras seguidas do giroscópio com falha
 *   - Ready do ESCON baixo (se ESCON_READY_CHECK)
 */
#pragma once
#include <Arduino.h>
#include "gyro_mpu9250.h"
#include "escon.h"
#include "config.h"

class Detumbling {
public:
  void begin(GyroMPU9250 *g, Escon *e, bool sensorOk);

  bool start();                                 // comando 1
  void stop(const __FlashStringHelper *motivo); // comando 2 ou segurança

  // Chamar a cada CONTROL_PERIOD_US. Lê o giroscópio e, se ativo, controla.
  void update();

  bool  active()    const { return ativo; }
  bool  inRange()   const { return amostrasNaFaixa >= SETTLE_SAMPLES; }
  bool  sensorOk()  const { return gyroOk; }
  float omega()     const { return omegaFilt; }   // [rad/s] filtrado
  float torque()    const { return tauCmd; }      // [N·m] comandado
  uint32_t elapsedMs() const { return ativo ? millis() - inicioMs : 0; }
  // Tempo até entrar na faixa na última manobra (0 = ainda não entrou).
  uint32_t timeToRangeMs() const { return tFaixaMs; }
  // Por que o motor parou sozinho na última manobra (FALHA_* de config.h; 0 = nenhuma/por comando).
  uint8_t falha() const { return gyroOk ? codigoFalha : FALHA_SEM_GIROSCOPIO; }

private:
  GyroMPU9250 *gyro = nullptr;
  Escon *escon = nullptr;
  bool gyroOk = false;

  bool ativo = false;
  bool atingiu = false;       // entrou na faixa ao menos uma vez nesta manobra
  bool filtroIniciado = false;
  float omegaFilt = 0.0f;
  float tauCmd = 0.0f;
  uint16_t amostrasNaFaixa = 0;
  uint16_t amostrasSobrevel = 0;
  uint8_t falhasSeguidas = 0;
  uint32_t inicioMs = 0;
  uint32_t tFaixaMs = 0;
  uint8_t codigoFalha = 0;
};
