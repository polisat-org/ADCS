# Firmware ADCS: Arduino Nano (PCB_LASC_v1.0)

Firmware do microcontrolador da placa ADCS do POLISAT (Cubedesign 2026). Ele faz duas coisas:

1. **Detumbling** no eixo Z, com a roda de reação (maxon EC 20 flat + ESCON Module 24/2) e o giroscópio do MPU-9250. A lei de controle e as constantes são as do simulador (`Simulador/params.m`).
2. **Determinação de atitude (azimute)** a partir de uma fonte de luz, com 4 LDRs (faces FRONTAL, DIREITO, TRASEIRO e ESQUERDO).

O OBC comanda o modo pela UART e recebe o azimute de volta.

## Arquivos (`adcs_firmware/`)

| Arquivo | Conteúdo |
|---|---|
| `config.h` | **Todas** as constantes: pinos, planta, ganho, escala do ESCON, calibração dos LDRs, UART |
| `adcs_firmware.ino` | `setup()`, loop de 100 Hz, comandos, telemetria de depuração |
| `gyro_mpu9250.*` | Driver mínimo do giroscópio (registradores via `Wire`, sem biblioteca externa) |
| `escon.*` | Torque → PWM 10–90 %, Enable, Direction, Ready |
| `detumbling.*` | Lei τ = Kp·ω, filtro, faixa-alvo e proteções |
| `sun_sensor.*` | Leitura dos LDRs em dois bancos e cálculo do azimute |
| `obc_link.*` | UART com o OBC (SoftwareSerial) com fila de transmissão não bloqueante |

Só usa bibliotecas que vêm com o Arduino (`Wire`, `SoftwareSerial`).

**Compilar/gravar:** abra `adcs_firmware/adcs_firmware.ino` no Arduino IDE, placa **Arduino Nano**, processador **ATmega328P** (ou *Old Bootloader*, se o upload falhar). Tamanho atual: 13,9 kB de flash (42 %) e 756 B de RAM (37 %).

## Pinagem (extraída da netlist do `PCB_LASC_v1.0.kicad_pcb`)

| Nano | Rede | Destino |
|---|---|---|
| D3 | /D1IN | ESCON pino 17 DigIN1: PWM Set Value |
| D4 | /D2IN | ESCON pino 16 DigIN2: Enable |
| D5 | /D3IN/D1OUT | ESCON pino 15 DigIN/OUT3: Direction |
| D6 | /D4IN/D2OUT | ESCON pino 14 DigIN/OUT4: Ready (entrada no Nano) |
| A6 / A7 | /AOUT2 / /AOUT1 | ESCON AnOUT2 / AnOUT1 (não usados: podem ficar negativos) |
| A4 / A5 | SDA / SCL | MPU-9250 (endereço 0x68) |
| D12 | Rx | J1 pinos 7-8: UART **do** OBC |
| D11 | Tx | J1 pinos 13-14: UART **para** o OBC |
| A2 | /D7 | base de Q1: alimenta FRONTAL + DIREITO |
| A3 | /D8 | base de Q2: alimenta TRASEIRO + ESQUERDO |
| A0 | /A0 | FRONTAL e TRASEIRO sobre R1 = 100 Ω |
| A1 | /A1 | DIREITO e ESQUERDO sobre R2 = 100 Ω |
| D7, D8, D10 | HC-05 RX/TX, /DISABLE | sem conexão na placa |
| D0 / D1 | — | livres: ficam para o USB (depuração) |

> ⚠️ A tabela de ligações em `Simulador/escon_studio_config.md` (seções 4.2, 7 e 9) ainda cita **D7/D8/D9**. A PCB usa **D3/D4/D5**, que é o que este firmware usa. A configuração do ESCON Studio em si não muda.

## Protocolo com o OBC

UART em **9600 8N1** (`UART_BAUD`).

| Sentido | Conteúdo |
|---|---|
| OBC → ADCS | `2` (ASCII 0x32): entra no **modo detumbling** |
| OBC → ADCS | `1` (ASCII 0x31): sai do modo detumbling (motor desabilitado) |
| ADCS → OBC | azimute em graus, `float` com 2 casas, uma linha a cada 200 ms: `123.45\r\n` |
| ADCS → OBC | `-1.00` = luz insuficiente para medir |

Outros bytes (CR, LF etc.) são ignorados, então mandar `2\n` também funciona.

**Modo 2:** o firmware lê o giroscópio a 100 Hz e comanda a roda para levar |ω| abaixo de 5 rpm e **manter** lá até chegar o `1`. Ele não desliga sozinho ao atingir a faixa, porque o atrito da roda devolveria o momento ao corpo. O motor é desligado e o modo volta a 1 automaticamente em três casos: a faixa não foi atingida em 15 s; o giroscópio falhou 5 vezes seguidas; o ESCON sinalizou erro no Ready (só com `ESCON_READY_CHECK = true`).

**Limitação do SoftwareSerial:** ele é half-duplex. Um comando que chega exatamente enquanto um byte de telemetria está saindo pode se perder (~4 % do tempo). Se o modo não mudar, o OBC deve reenviar.

## Depuração pelo USB (115200)

| Comando | Ação |
|---|---|
| `2` / `1` | Mesmos comandos do OBC |
| `C` | Recalibra o bias do giroscópio (satélite **parado**, só no modo 1) |
| `?` | Estado: modo, ω, se está na faixa, tempo até a faixa, Ready, azimute |
| `L` | LDRs: tensão, valor normalizado, x, y e azimute |

No modo 2 sai uma linha CSV a 10 Hz: `t[s],omega[rpm],torque[mN·m]`.

## Determinação de atitude

```
n_i = (V_i / LDR_CAL[i]) ^ (1/LDR_GAMMA)
az  = atan2(n_DIREITO − n_ESQUERDO, n_FRONTAL − n_TRASEIRO)    → [0, 360)°
```

0° = luz de frente para a face FRONTAL, 90° = face DIREITA. A diferença entre faces opostas cancela a luz ambiente comum às duas. Para a telemetria, as amostras de 10 Hz entram numa média vetorial, que não salta em 0/360°.

**Premissas a confirmar** (em `config.h`, seção 7):

1. **Numeração:** LDR 1 = FRONTAL, 2 = DIREITO, 3 = TRASEIRO, 4 = ESQUERDO. Se for outra, reordene `LDR_CAL`.
2. **Significado da calibração (0,434 / 0,637 / 0,502 / 0,570):** é a leitura de cada LDR sob a **mesma** luz, e o firmware **divide** por ela. A unidade não importa, porque só a razão entre os valores entra no `atan2`. Se esses números forem ganhos que deveriam **multiplicar** a leitura, troque cada valor por 1/valor.
3. **`LDR_GAMMA` = 1:** num modelo cosseno ideal o erro é zero. Mas se o LDR responder com γ = 0,7 e o firmware ficar em 1, o erro chega a **6,7°**, acima da faixa de 5° do regulamento. Meça γ na bancada.

## Roteiro de testes na bancada

1. **Sem motor:** grave, abra o monitor serial e confira `WHO_AM_I` (0x71, 0x73 ou 0x70) e o bias.
2. **LDRs:** aponte a lanterna para cada face e mande `L`. A face iluminada deve ter o maior `n`, e o `az` deve dar ~0/90/180/270°.
3. **Varredura de azimute:** gire a fonte de 0° a 360° em passos de 15° e registre o erro. Ele vai para o relatório (HLR-ADCS-03) e, se preciso, para o ajuste de `LDR_GAMMA` ou uma tabela de correção.
4. **ESCON:** siga o checklist da seção 8 de `escon_studio_config.md`, com o satélite **preso**. Gire o corpo à mão no sentido +Z do giroscópio e mande `2`: a roda tem que acelerar **no mesmo sentido** do corpo. Se acelerar ao contrário, mude `INVERT_DIR` (ou `GYRO_Z_SIGN`, se o eixo do MPU estiver invertido), nunca os dois.
5. **Detumbling no mancal:** a 60 rpm, mande `2`. A simulação da lei discreta (com quantização do PWM) prevê cruzar 5 rpm em **~7,9 s**, com pico de 1,02 A.
6. **UART:** ligue o OBC (ou um conversor USB-serial em J1) e confira as linhas de azimute e os comandos `1`/`2`.
