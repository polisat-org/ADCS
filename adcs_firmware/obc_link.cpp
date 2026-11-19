/* obc_link.cpp — ver obc_link.h */
#include "obc_link.h"
#include "config.h"
#include <SoftwareSerial.h>

static SoftwareSerial uart(PIN_UART_RX, PIN_UART_TX);

void ObcLink::begin() {
  uart.begin(UART_BAUD);
}

int ObcLink::readCommand() {
  return uart.available() > 0 ? uart.read() : -1;
}

void ObcLink::formatAzimuth(float azDeg, char *out) {
  // Décimos de grau; 359.95 arredonda para 3600 = 360.0 e volta para 000.0.
  long d = (long)(azDeg * 10.0f + 0.5f);
  d %= 3600;
  if (d < 0) d += 3600;
  snprintf(out, AZ_MSG_LEN + 1, "%03u.%u", (unsigned)(d / 10), (unsigned)(d % 10));
}

void ObcLink::sendAzimuth(float azDeg) {
  char msg[AZ_MSG_LEN + 1];
  formatAzimuth(azDeg, msg);
  uart.write((const uint8_t *)msg, AZ_MSG_LEN);
}
