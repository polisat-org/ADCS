/* sun_sensor.h — Azimute da fonte de luz com 4 LDRs (FRONTAL, DIREITO,
 * TRASEIRO, ESQUERDO)
 *
 * HARDWARE (PCB_LASC_v1.0)
 *   Q1 (base em A2) alimenta FRONTAL e DIREITO; Q2 (base em A3) alimenta
 *   TRASEIRO e ESQUERDO. A0 lê FRONTAL/TRASEIRO e A1 lê DIREITO/ESQUERDO,
 *   cada um sobre 100 Ω para o GND. Um banco é ligado por vez.
 *
 * ALGORITMO
 *   v_i  = tensão no nó analógico (média de LDR_OVERSAMPLE leituras)
 *   n_i  = (v_i / LDR_CAL[i])^(1/LDR_GAMMA)     sensibilidades igualadas
 *   x    = n_FRONTAL − n_TRASEIRO   (∝ cos az)
 *   y    = n_DIREITO − n_ESQUERDO   (∝ sin az)
 *   az   = atan2(y, x)  em [0, 360)°, 0° = FRONTAL, 90° = DIREITO
 *   A diferença entre faces opostas tira a parte da luz que é comum às duas
 *   (ambiente difuso). Para a telemetria, (x, y) são somados entre envios e
 *   o azimute sai do vetor médio (média circular, sem problema em 0/360°).
 *
 * LIMITAÇÕES (HLR-ADCS-03)
 *   - Supõe resposta ~cosseno de cada face; LDR real tem campo de visão e
 *     não linearidade próprios. O erro residual tem que ser medido na
 *     bancada (varredura de 0 a 360°) e, se preciso, corrigido por tabela.
 *   - Com o banco A ligado há um caminho parasita A0 -> TRASEIRO -> ESQUERDO
 *     -> A1 (dois LDRs em série entre nós de ~0.5 V): ~1 % de diafonia.
 *   - Resolução: ~4.9 mV por contagem (Vref 5 V); com leituras de ~0.5 V a
 *     média de 8 amostras dá bem menos de 1° de quantização.
 */
#pragma once
#include <Arduino.h>

class SunSensor {
public:
  void begin();

  // Lê os 4 LDRs (~4.5 ms, bloqueante). Se acumular=true e houver luz
  // suficiente, acumula o vetor para a telemetria.
  void sample(bool acumular = true);

  // Azimute do vetor médio desde a última chamada; zera o acumulador.
  // Retorna false (e az = ATT_INVALID) se não havia luz suficiente.
  bool takeAverage(float &azDeg);

  // Última amostra individual (para depuração/calibração).
  float volts(uint8_t face) const { return v[face]; }
  float norm(uint8_t face)  const { return n[face]; }
  float x() const { return vx; }
  float y() const { return vy; }
  float azimuthDeg() const { return az; }
  bool  valid() const { return ok; }

private:
  float readVolts(uint8_t pino);
  static float toDeg360(float yy, float xx);

  float v[4] = {0, 0, 0, 0};
  float n[4] = {0, 0, 0, 0};
  float vx = 0, vy = 0, az = 0;
  bool ok = false;

  float somaX = 0, somaY = 0;
  uint16_t nSomas = 0;
};
