/* detumbling.cpp — ver detumbling.h */
#include "detumbling.h"

void Detumbling::begin(GyroMPU9250 *g, Escon *e, bool sensorOk) {
  gyro = g;
  escon = e;
  gyroOk = sensorOk;
  ativo = false;
  escon->disable();
}

bool Detumbling::start() {
  if (ativo) return true;
  if (!gyroOk) {
    Serial.println(F("ERRO: giroscopio indisponivel, detumbling bloqueado"));
    return false;
  }
  ativo = true;
  atingiu = false;
  amostrasNaFaixa = 0;
  falhasSeguidas = 0;
  tFaixaMs = 0;
  inicioMs = millis();
  escon->enable();
  Serial.println(F("# MODO 2: detumbling ATIVO"));
  return true;
}

void Detumbling::stop(const __FlashStringHelper *motivo) {
  escon->disable();
  tauCmd = 0.0f;
  bool estavaAtivo = ativo;
  ativo = false;
  if (estavaAtivo) {
    Serial.print(F("# MODO 1: detumbling PARADO ("));
    Serial.print(motivo);
    Serial.println(')');
  }
}

void Detumbling::update() {
  if (!gyroOk) return;

  float w;
  if (!gyro->readZ(w)) {
    // Uma falha isolada mantém o último comando; várias seguidas abortam.
    if (ativo && ++falhasSeguidas >= GYRO_MAX_FAILS) stop(F("falha de leitura do giroscopio"));
    return;
  }
  falhasSeguidas = 0;

  if (!filtroIniciado) {
    omegaFilt = w;
    filtroIniciado = true;
  } else {
    omegaFilt += LPF_ALPHA * (w - omegaFilt);
  }

  if (!ativo) return;

  // Lei proporcional. O torque tem o sinal de ω: a roda acelera no mesmo
  // sentido do corpo e o corpo recebe a reação contrária.
  tauCmd = KP * omegaFilt;
  escon->setTorque(tauCmd);

  if (fabs(omegaFilt) <= OMEGA_THRESHOLD) {
    if (amostrasNaFaixa < SETTLE_SAMPLES) amostrasNaFaixa++;
    if (amostrasNaFaixa >= SETTLE_SAMPLES && !atingiu) {
      atingiu = true;
      tFaixaMs = millis() - inicioMs;
      Serial.print(F("# faixa de 5 rpm atingida em "));
      Serial.print(tFaixaMs * 0.001f, 2);
      Serial.println(F(" s (controle segue ativo ate o comando 1)"));
    }
  } else {
    amostrasNaFaixa = 0;
  }

  if (!atingiu && millis() - inicioMs > MANEUVER_TIMEOUT_MS) {
    stop(F("TIMEOUT: verifique sentido da roda, Enable e eixo do giroscopio"));
  } else if (escon->readyFault()) {
    stop(F("ESCON sinalizou erro no Ready"));
  }
}
