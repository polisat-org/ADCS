/* obc_link.cpp — ver obc_link.h */
#include "obc_link.h"
#include "config.h"
#include <SoftwareSerial.h>

static SoftwareSerial uart(PIN_UART_RX, PIN_UART_TX);

void ObcLink::begin() {
  uart.begin(UART_BAUD);
  ini = fim = 0;
}

int ObcLink::readCommand() {
  return uart.available() > 0 ? uart.read() : -1;
}

size_t ObcLink::write(uint8_t b) {
  if (livre() == 0) return 0;
  buf[fim] = b;
  fim = (fim + 1) % FILA;
  return 1;
}

bool ObcLink::sendAttitude(float azDeg) {
  // "-1.00\r\n" ou "359.99\r\n": no máximo 8 bytes. Só escreve a linha se
  // couber inteira, para o OBC nunca receber um número cortado.
  if (livre() < 10) {
    descartadas++;
    return false;
  }
  print(azDeg, ATT_TX_DECIMALS);
  print(F("\r\n"));
  return true;
}

void ObcLink::service() {
  if (ini == fim) return;
  uart.write(buf[ini]);
  ini = (ini + 1) % FILA;
}
