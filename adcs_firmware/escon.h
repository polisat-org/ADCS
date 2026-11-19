/* escon.h — Interface com o maxon ESCON Module 24/2
 *
 * Modo esperado no ESCON Studio (ver Simulador/escon_studio_config.md):
 *   Current Controller · Enable & Direction · PWM Set Value em DigIN1
 *   10 % = 0 A, 90 % = DRIVER_I_FS · Enable em DigIN2 (high active)
 *   Direction em DigIN/OUT3 · Ready em DigIN/OUT4 (opcional)
 *
 * O comando é TORQUE na roda [N·m]: τ = Kt·I, e o ESCON fecha a malha de
 * corrente. O sinal do torque vira o pino de direção; o módulo vira duty.
 */
#pragma once
#include <Arduino.h>

class Escon {
public:
  void begin();          // pinos e PWM em 10 % (comando zero), motor desabilitado
  void enable();         // PWM já em zero antes do Enable (recomendação maxon)
  void disable();        // comando zero + Enable LOW
  void setTorque(float tauRoda);   // [N·m], saturado em ±TAU_FS

  bool enabled() const { return ligado; }
  bool ready() const;    // true = sem erro (ou Ready não configurado)
  bool readyFault() const;   // Ready baixo além da tolerância, com motor ligado
  float lastTorque() const { return tauAtual; }

private:
  bool ligado = false;
  uint32_t enableMs = 0;
  float tauAtual = 0.0f;
};
