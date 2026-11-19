/* obc_link.h — UART com o OBC (SoftwareSerial: RX = D12, TX = D11)
 *
 * PROTOCOLO
 *   OBC -> ADCS : 1 byte ASCII.  '2' = entra no modo detumbling
 *                                '1' = sai do modo detumbling
 *                 Outros bytes (CR, LF...) são ignorados.
 *   ADCS -> OBC : azimute em graus como float ASCII, uma linha por valor
 *                 ("123.45\r\n"), a cada ATT_TX_PERIOD_MS. ATT_INVALID
 *                 (-1.00) = luz insuficiente para medir.
 *
 * TEMPO REAL
 *   O SoftwareSerial transmite com as interrupções desligadas (~1.04 ms por
 *   byte a 9600 baud). Para não atrasar o loop de 100 Hz, a linha vai para
 *   uma fila e service() envia UM byte por ciclo de controle (100 B/s, contra
 *   ~40 B/s de telemetria).
 *
 * LIMITAÇÃO
 *   O SoftwareSerial é half-duplex: um byte que chega do OBC enquanto um byte
 *   está saindo pode ser perdido (~4 % do tempo com a telemetria a 5 Hz). O
 *   OBC deve reenviar o comando se o efeito não aparecer.
 */
#pragma once
#include <Arduino.h>

class ObcLink : public Print {
public:
  void begin();

  // Próximo byte recebido do OBC, ou -1 se não houver.
  int readCommand();

  // Enfileira o azimute. Retorna false (linha descartada) se a fila estiver cheia.
  bool sendAttitude(float azDeg);

  // Envia no máximo um byte da fila. Chamar uma vez por ciclo de controle.
  void service();

  uint16_t droppedLines() const { return descartadas; }

  // Print: escreve na fila (não bloqueia).
  size_t write(uint8_t b) override;
  using Print::write;

private:
  static const uint8_t FILA = 32;
  uint8_t buf[FILA];
  uint8_t ini = 0, fim = 0;
  uint16_t descartadas = 0;
  uint8_t livre() const { return (uint8_t)(FILA - 1 - (uint8_t)(fim - ini + FILA) % FILA); }
};
