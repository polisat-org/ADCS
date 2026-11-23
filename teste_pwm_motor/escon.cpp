/* escon.cpp — ver escon.h */
#include "escon.h"
#include "config.h"

// Duty em contagens de 8 bits (analogWrite). 10 % -> 26, 90 % -> 230: 204
// passos de ~9.8 mA (0.058 mN·m) cada.
static const int DUTY_ZERO = (255L * DUTY_MIN_PCT) / 100;
static const int DUTY_FULL = (255L * DUTY_MAX_PCT) / 100;

void Escon::begin() {
  pinMode(PIN_ESCON_ENABLE, OUTPUT);
  digitalWrite(PIN_ESCON_ENABLE, LOW);
  pinMode(PIN_ESCON_DIR, OUTPUT);
  digitalWrite(PIN_ESCON_DIR, LOW);
  pinMode(PIN_ESCON_PWM, OUTPUT);
  analogWrite(PIN_ESCON_PWM, DUTY_ZERO);
  // Pull-up: se DigOUT4 não estiver configurado como Ready, lê alto e não
  // gera falso alarme.
  pinMode(PIN_ESCON_READY, INPUT_PULLUP);
  ligado = false;
  tauAtual = 0.0f;
}

void Escon::enable() {
  analogWrite(PIN_ESCON_PWM, DUTY_ZERO);
  tauAtual = 0.0f;
  digitalWrite(PIN_ESCON_ENABLE, HIGH);
  if (!ligado) enableMs = millis();
  ligado = true;
}

void Escon::disable() {
  analogWrite(PIN_ESCON_PWM, DUTY_ZERO);
  digitalWrite(PIN_ESCON_ENABLE, LOW);
  tauAtual = 0.0f;
  ligado = false;
}

void Escon::setTorque(float tauRoda) {
  if (!ligado) return;
  float tau = constrain(tauRoda, -TAU_FS, TAU_FS);
  tauAtual = tau;

  bool sentidoPositivo = (tau >= 0.0f);
  if (INVERT_DIR) sentidoPositivo = !sentidoPositivo;
  digitalWrite(PIN_ESCON_DIR, sentidoPositivo ? HIGH : LOW);

  float fracao = fabs(tau) / TAU_FS;   // 0..1
  int counts = DUTY_ZERO + (int)(fracao * (DUTY_FULL - DUTY_ZERO) + 0.5f);
  analogWrite(PIN_ESCON_PWM, constrain(counts, DUTY_ZERO, DUTY_FULL));
}

bool Escon::ready() const {
  return digitalRead(PIN_ESCON_READY) == HIGH;
}

bool Escon::readyFault() const {
  if (!ESCON_READY_CHECK || !ligado) return false;
  if (millis() - enableMs < ESCON_READY_GRACE_MS) return false;
  return !ready();
}
