/* config.h — Configuração central do firmware ADCS (Arduino Nano)
 *
 * Equipe: POLISAT · Subsistema: ADCS · Cubedesign 2026
 *
 * TODAS as constantes do firmware ficam aqui: pinos (da PCB), parâmetros da
 * planta e do controlador (de Simulador/params.m), escala do ESCON (de
 * Simulador/escon_studio_config.md), calibração dos LDRs e protocolo UART.
 * Os outros arquivos só consomem estes valores.
 */
#pragma once
#include <Arduino.h>

/* ══════════════════════════════════════════════════════════════════════════
 * 1. PINOS — netlist da PCB_LASC_v1.0 (PCB/Projeto ADCS/PCB_LASC_v1.0.kicad_pcb)
 * ══════════════════════════════════════════════════════════════════════════
 *
 *  Pino Nano | Rede na PCB          | Liga em
 *  ----------+----------------------+------------------------------------------
 *  D3  (PWM) | /D1IN                | ESCON pino 17  DigIN1   (PWM Set Value)
 *  D4        | /D2IN                | ESCON pino 16  DigIN2   (Enable)
 *  D5        | /D3IN/D1OUT          | ESCON pino 15  DigIN/OUT3 (Direction)
 *  D6        | /D4IN/D2OUT          | ESCON pino 14  DigIN/OUT4 (Ready, saída do ESCON)
 *  A7        | /AOUT1               | ESCON pino 20  AnOUT1   (não usado)
 *  A6        | /AOUT2               | ESCON pino 19  AnOUT2   (não usado)
 *  A4 / A5   | /A4 /A5              | MPU-9250 SDA / SCL (I2C)
 *  D11       | Tx  (global label)   | conector de pilha J1 pinos 13-14 (UART -> OBC)
 *  D12       | Rx  (global label)   | conector de pilha J1 pinos 7-8   (UART <- OBC)
 *  A2        | /D7                  | base de Q1 (2N2219): alimenta DIREITO e FRONTAL
 *  A3        | /D8                  | base de Q2 (2N2219): alimenta ESQUERDO e TRASEIRO
 *  A0        | /A0                  | FRONTAL pino 1 + TRASEIRO pino 1 + R1 100 Ω -> GND
 *  A1        | /A1                  | DIREITO pino 1 + ESQUERDO pino 1 + R2 100 Ω -> GND
 *  D7 / D8   | /HC-05 RX, /HC-05 TX | sem conexão na placa (sobras do HC-05)
 *  D10       | /DISABLE             | sem conexão na placa
 *  D0 / D1   | —                    | livres na placa: ficam para o USB (depuração)
 *
 * Os LDRs formam uma matriz 2x2: cada transistor (seguidor de emissor)
 * alimenta dois LDRs, e cada entrada analógica lê um LDR de cada par sobre o
 * resistor de 100 Ω. Ligando só Q1 lê-se FRONTAL (A0) e DIREITO (A1); ligando
 * só Q2 lê-se TRASEIRO (A0) e ESQUERDO (A1). Os dois transistores NUNCA podem
 * ficar ligados juntos, senão os LDRs do mesmo nó analógico se somam.
 */
const uint8_t PIN_ESCON_PWM    = 3;    // Timer2, ~490 Hz (ESCON aceita 10 Hz–5 kHz)
const uint8_t PIN_ESCON_ENABLE = 4;
const uint8_t PIN_ESCON_DIR    = 5;
const uint8_t PIN_ESCON_READY  = 6;

const uint8_t PIN_UART_TX = 11;        // Nano -> OBC
const uint8_t PIN_UART_RX = 12;        // OBC  -> Nano

const uint8_t PIN_LDR_BANK_A  = A2;    // Q1: FRONTAL + DIREITO
const uint8_t PIN_LDR_BANK_B  = A3;    // Q2: TRASEIRO + ESQUERDO
const uint8_t PIN_LDR_SENSE_0 = A0;    // FRONTAL (banco A) / TRASEIRO (banco B)
const uint8_t PIN_LDR_SENSE_1 = A1;    // DIREITO (banco A) / ESQUERDO (banco B)

/* ══════════════════════════════════════════════════════════════════════════
 * 2. PLANTA E REQUISITOS DA MISSÃO   (params.m, seções 1–3)
 * ══════════════════════════════════════════════════════════════════════════ */
const float I_B             = 3.067e-3f; // [kg·m²] inércia do corpo no eixo da roda (rev. 19/10/2026)
const float OMEGA_0         = 6.2832f;   // [rad/s] 60 rpm, rotação inicial
const float OMEGA_THRESHOLD = 0.5236f;   // [rad/s]  5 rpm, faixa-alvo do detumbling
const float T_REQUIRE       = 10.0f;     // [s] requisito de tempo

/* ══════════════════════════════════════════════════════════════════════════
 * 3. GANHO DO CONTROLADOR   (params.m, seção 5c)
 * ══════════════════════════════════════════════════════════════════════════
 * Kp sai do requisito de tempo, não de ajuste empírico:
 *   T_design = 0.80 · T_REQUIRE = 8 s
 *   Kp = I_B · ln(OMEGA_0 / OMEGA_THRESHOLD) / T_design = 9.53e-4 N·m·s/rad
 * Torque de pico (t = 0): Kp · OMEGA_0 = 5.99 mN·m (1.02 A), abaixo dos
 * 11.76 mN·m de fundo de escala, então a lei não satura.
 */
const float KP = 9.53e-4f;               // [N·m·s/rad]

/* ══════════════════════════════════════════════════════════════════════════
 * 4. MOTOR + ESCON 24/2   (params.m seção 4, escon_studio_config.md)
 * ══════════════════════════════════════════════════════════════════════════
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
// Com ESCON_READY_CHECK = true, Ready baixo durante o detumbling desliga o
// motor. Só ative depois de configurar DigOUT4 = "Ready, High active".
const bool     ESCON_READY_CHECK    = false;
const uint16_t ESCON_READY_GRACE_MS = 200;  // tolerância após o Enable

/* ══════════════════════════════════════════════════════════════════════════
 * 5. GIROSCÓPIO MPU-9250
 * ══════════════════════════════════════════════════════════════════════════ */
const uint8_t MPU_ADDR      = 0x68;      // AD0 sem ligação na PCB -> 0x68
const float   GYRO_Z_SIGN   = 1.0f;      // -1 se o eixo Z do MPU apontar contra o da roda
// Calibração de bias: o satélite tem que estar parado. Se a leitura variar
// mais que isso durante a média, a calibração é recusada.
const uint16_t GYRO_CAL_SAMPLES  = 100;      // 1 s a 100 Hz
const float    GYRO_CAL_MAX_SPAN = 0.035f;   // [rad/s] ~2 °/s

/* ══════════════════════════════════════════════════════════════════════════
 * 6. LOOP DE CONTROLE, FILTRO E SEGURANÇA DO DETUMBLING
 * ══════════════════════════════════════════════════════════════════════════ */
const uint32_t CONTROL_PERIOD_US = 10000;                       // 100 Hz = f_s de params.m
const float    DT                = CONTROL_PERIOD_US * 1.0e-6f; // [s]

// Passa-baixa de 1ª ordem em ω (corte bem acima da dinâmica de 3.22 s).
const float LPF_FC    = 5.0f;                                   // [Hz]
const float LPF_ALPHA = (2.0f*PI*LPF_FC*DT) / (1.0f + 2.0f*PI*LPF_FC*DT);

// "Na faixa" = |ω| <= OMEGA_THRESHOLD por SETTLE_SAMPLES amostras seguidas.
const uint16_t SETTLE_SAMPLES = 50;                             // 0.5 s

// Se a faixa não for atingida neste tempo após o comando 2, algo está errado
// (sensor, sentido invertido, ESCON desabilitado): o motor é desligado.
const uint32_t MANEUVER_TIMEOUT_MS = (uint32_t)(1500.0f * T_REQUIRE);  // 1.5x = 15 s

// Falhas seguidas de leitura do giroscópio antes de abortar.
const uint8_t GYRO_MAX_FAILS = 5;                               // 50 ms

/* ══════════════════════════════════════════════════════════════════════════
 * 7. LDRs — DETERMINAÇÃO DE ATITUDE (AZIMUTE)
 * ══════════════════════════════════════════════════════════════════════════
 * Numeração dos LDRs da calibração -> face (sentido horário visto de cima,
 * azimute 0° = FRONTAL, 90° = DIREITO, 180° = TRASEIRO, 270° = ESQUERDO).
 * CONFIRMAR: se a numeração física for outra, troque só LDR_CAL.
 *
 * LDR_CAL[i] = leitura (em volts no nó analógico) do LDR i sob a MESMA luz
 * de referência. A leitura é dividida por esse valor, o que iguala a
 * sensibilidade dos quatro. Como o azimute vem de atan2, só a RAZÃO entre os
 * valores importa: a unidade da calibração não muda o ângulo.
 */
enum LdrFace : uint8_t { LDR_FRONTAL = 0, LDR_DIREITO = 1, LDR_TRASEIRO = 2, LDR_ESQUERDO = 3 };

const float LDR_CAL[4] = {
  0.434f,   // LDR 1 -> FRONTAL
  0.637f,   // LDR 2 -> DIREITO
  0.502f,   // LDR 3 -> TRASEIRO
  0.570f,   // LDR 4 -> ESQUERDO
};

const float    ADC_VREF          = 5.0f;   // [V] referência DEFAULT (5 V do Nano)
const uint8_t  LDR_OVERSAMPLE    = 8;      // leituras médias por canal
const uint16_t LDR_SETTLE_US     = 300;    // espera após ligar um banco
// Linearização opcional: o LDR responde a E^gamma. Com gamma ≠ 1 a leitura
// normalizada é elevada a 1/gamma antes do atan2. 1.0 = sem correção.
const float    LDR_GAMMA         = 1.0f;
// Abaixo deste módulo (em unidades de leitura de calibração) não há luz
// suficiente para um azimute confiável.
const float    LDR_MIN_SIGNAL    = 0.05f;
const uint32_t ATT_SAMPLE_PERIOD_MS = 100;  // amostragem dos LDRs: 10 Hz

/* ══════════════════════════════════════════════════════════════════════════
 * 8. UART COM O OBC (SoftwareSerial em D11/D12) E DEPURAÇÃO (USB)
 * ══════════════════════════════════════════════════════════════════════════
 * Comandos (1 byte ASCII):  '2' = entra no detumbling,  '1' = sai.
 * Telemetria no TX: azimute em graus, float com 2 casas, uma linha por valor.
 */
const uint32_t UART_BAUD          = 9600;
const uint32_t ATT_TX_PERIOD_MS   = 200;     // telemetria de atitude: 5 Hz
const float    ATT_INVALID        = -1.0f;   // enviado quando não há luz suficiente
const uint8_t  ATT_TX_DECIMALS    = 2;

const uint32_t DEBUG_BAUD         = 115200;  // USB (D0/D1 livres na PCB)
const uint32_t DEBUG_PERIOD_MS    = 100;     // telemetria de depuração: 10 Hz
