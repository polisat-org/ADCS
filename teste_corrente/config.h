/* config.h — Teste de corrente EPS + ADCS (+0,2 A por 5 s, -0,2 A por 5 s)
 *
 * Pinos, motor e escala do ESCON: os MESMOS de adcs_firmware/config.h (main), para o teste usar o
 * ESCON exatamente como o detumbling: Current Controller, PWM 10 % = 0 A, 90 % = 2,0 A.
 * escon.h/escon.cpp são cópias sem mudança de adcs_firmware/. Se a config do firmware mudar, copie de novo.
 */
#pragma once
#include <Arduino.h>

/* ── Pinos (netlist da PCB_LASC_v1.0) ─────────────────────────────────────
 *  D3 | ESCON pino 17 DigIN1     (PWM Set Value)
 *  D4 | ESCON pino 16 DigIN2     (Enable)
 *  D5 | ESCON pino 15 DigIN/OUT3 (Direction)
 *  D6 | ESCON pino 14 DigIN/OUT4 (Ready, saída do ESCON)
 */
const uint8_t PIN_ESCON_PWM    = 3;
const uint8_t PIN_ESCON_ENABLE = 4;
const uint8_t PIN_ESCON_DIR    = 5;
const uint8_t PIN_ESCON_READY  = 6;

/* ── Motor + ESCON (iguais a adcs_firmware/config.h) ──────────────────── */
const float MOTOR_KT    = 5.88e-3f;      // [N·m/A]
const float DRIVER_I_FS = 2.0f;          // [A] = "Current at 90 %"
const float TAU_FS      = MOTOR_KT * DRIVER_I_FS;   // 11,76 mN·m
const uint8_t DUTY_MIN_PCT = 10;
const uint8_t DUTY_MAX_PCT = 90;
const bool    INVERT_DIR   = true;       // igual ao firmware: + = sentido positivo do detumbling
const bool     ESCON_READY_CHECK    = false;
const uint16_t ESCON_READY_GRACE_MS = 200;

/* ── Perfil do teste ─────────────────────────────────────────────────── */
const float    I_TESTE_A     = 0.2f;     // [A] corrente do motor nas duas fases
const uint32_t T_FASE_MS     = 5000;     // [ms] duração de cada fase
const float    TAU_CMD_MAX   = 0.5f * TAU_FS;   // trava: nada passa de 1,0 A

/* ── Início ───────────────────────────────────────────────────────────────
 * false: só começa com o comando "s" pelo serial (seguro para gravar com a roda montada).
 * true: começa sozinho AUTO_ATRASO_MS depois de ligar (útil alimentado só pela EPS, sem PC).
 */
const bool     AUTO_INICIO    = false;
const uint32_t AUTO_ATRASO_MS = 5000;

const uint16_t TELEM_MS   = 100;         // telemetria (10 Hz)
const uint32_t DEBUG_BAUD = 115200;
