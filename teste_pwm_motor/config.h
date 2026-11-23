/* config.h — Constantes do teste de PWM do motor
 *
 * Recorte de adcs_firmware/config.h (seções 1 e 4): só os pinos do ESCON e a
 * escala do PWM, com os MESMOS valores. Se mudar algo lá, mude aqui também.
 */
#pragma once
#include <Arduino.h>

/* ── Pinos (netlist da PCB_LASC_v1.0) ─────────────────────────────────────
 *  D3 | /D1IN       | ESCON pino 17 DigIN1     (PWM Set Value)
 *  D4 | /D2IN       | ESCON pino 16 DigIN2     (Enable)
 *  D5 | /D3IN/D1OUT | ESCON pino 15 DigIN/OUT3 (Direction)
 *  D6 | /D4IN/D2OUT | ESCON pino 14 DigIN/OUT4 (Ready, saída do ESCON)
 */
const uint8_t PIN_ESCON_PWM    = 3;    // Timer2, ~490 Hz (ESCON aceita 10 Hz–5 kHz)
const uint8_t PIN_ESCON_ENABLE = 4;
const uint8_t PIN_ESCON_DIR    = 5;
const uint8_t PIN_ESCON_READY  = 6;

/* ── Motor + ESCON 24/2 ───────────────────────────────────────────────────
 * O ESCON tem que estar em CURRENT CONTROLLER, Enable & Direction, PWM Set
 * Value em DigIN1 com 10 % = 0 A e 90 % = DRIVER_I_FS. Fora de 10–90 % (ou
 * PWM parado) o ESCON acusa "PWM Set Value Input out of Range", por isso o
 * comando zero é 10 % e não 0 %.
 */
const float MOTOR_KT    = 5.88e-3f;      // [N·m/A]
const float DRIVER_I_FS = 2.0f;          // [A] = "Current at 90 %" no ESCON Studio
const float TAU_FS      = MOTOR_KT * DRIVER_I_FS;   // [N·m] = 11.76 mN·m

const uint8_t DUTY_MIN_PCT = 10;         // comando zero
const uint8_t DUTY_MAX_PCT = 90;         // fundo de escala
const bool    INVERT_DIR   = false;      // true se a roda acelerar no sentido errado

// Ready (DigIN/OUT4). Lido com pull-up; nível alto = ESCON sem erro.
// Com ESCON_READY_CHECK = true, Ready baixo com o motor ligado desliga o
// motor. Só ative depois de configurar DigOUT4 = "Ready, High active".
const bool     ESCON_READY_CHECK    = false;
const uint16_t ESCON_READY_GRACE_MS = 200;  // tolerância após o Enable

/* ── Segurança do teste (só deste sketch) ─────────────────────────────────
 * MALHA ABERTA: em Current Controller o PWM define a corrente (torque), não
 * a velocidade. Com comando fixo a roda acelera até o limite do ESCON
 * (Max. Permissible Speed = 12000 rpm), então todo comando volta sozinho
 * para zero depois de TEMPO_MAX_MS.
 */
const uint8_t  CMD_MAX_PCT  = 50;        // teto do comando: 50 % = 1,0 A = 5,9 mN·m
const uint32_t TEMPO_MAX_MS = 3000;      // comando != 0 volta a zero depois disso

const uint32_t DEBUG_BAUD = 115200;      // USB
