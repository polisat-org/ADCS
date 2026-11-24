/* obc_link.h — UART com o OBC (SoftwareSerial: RX = D12, TX = D11)
 *
 * PROTOCOLO (definido pelo OBC)
 *   OBC -> ADCS : comando como inteiro de 1 byte (valor binário, não ASCII):
 *                   1 = inicia detumbling      2 = encerra detumbling
 *                   3 = inicia busca de azimute 4 = encerra busca
 *                 Outros valores (ex.: bytes 0 extras do vetor comandoADCS)
 *                 são ignorados.
 *   ADCS -> OBC : azimute em graus como string de EXATAMENTE 5 caracteres,
 *                 sem terminador: 270.5° -> "270.5", 45.3° -> "045.3".
 *
 * O OBC recebe com HAL_UART_Receive_IT(&huart1, angulo, sizeof(angulo) - 1),
 * ou seja, em blocos fixos de 5 bytes. Por isso nada além dessas 5 letras
 * pode sair por este TX: um byte a mais desalinharia todas as leituras
 * seguintes. Mensagens de depuração vão só para o USB.
 *
 * TEMPO REAL
 *   O SoftwareSerial transmite com as interrupções desligadas (~1.04 ms por
 *   byte a 9600 baud). As 5 letras saem juntas (~5.2 ms), num ciclo em que os
 *   LDRs não são lidos, para caber nos 10 ms do loop. Enviar em rajada também
 *   diminui a chance de o OBC começar a ouvir no meio de uma mensagem.
 *
 * LIMITAÇÃO
 *   O SoftwareSerial é half-duplex: um comando que chega durante esses 5 ms
 *   pode se perder (~0.5 % do tempo com um envio por segundo).
 */
#pragma once
#include <Arduino.h>

class ObcLink {
public:
  void begin();

  // Próximo byte recebido do OBC, ou -1 se não houver.
  int readCommand();

  // Envia o azimute nos 5 caracteres do protocolo (bloqueia ~5 ms a 9600).
  void sendAzimuth(float azDeg);

  // Envia o estado "SDBF!" (bloqueia ~5 ms a 9600). Não se confunde com o azimute "ddd.d".
  void sendStatus(bool detumbling, bool busca, uint8_t falha);

  // "270.5", "045.3", "000.0": 0.0–359.9 arredondado a 0.1°. 'out' precisa
  // de AZ_MSG_LEN + 1 bytes (o '\0' não é transmitido).
  static void formatAzimuth(float azDeg, char *out);
};
