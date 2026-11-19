# Firmware ADCS: Arduino Nano (PCB_LASC_v1.0)

Firmware do microcontrolador da placa ADCS do POLISAT (Cubedesign 2026). Ele faz duas coisas:

1. **Detumbling** no eixo Z, com a roda de reação (maxon EC 20 flat + ESCON Module 24/2) e o giroscópio do MPU-9250. A lei de controle e as constantes são as do simulador (`params.m`); os valores estão copiados em `config.h`.
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

Documentação de apoio: [`docs/escon_studio_config.md`](docs/escon_studio_config.md), com todos os parâmetros do ESCON Studio e a ligação ESCON ↔ Nano.

## Gravar no Arduino

1. Abra `adcs_firmware/adcs_firmware.ino` no Arduino IDE. Os outros arquivos da pasta abrem como abas e são compilados juntos; mantenha a pasta inteira.
2. **Tools → Board:** Arduino Nano. **Processor:** ATmega328P. Se o upload falhar, use *ATmega328P (Old Bootloader)*, que é o caso da maioria dos clones.
3. **Tools → Port:** a porta COM do Nano. Clique em **Upload**.

Não precisa instalar biblioteca. Tamanho atual: 13,9 kB de flash (42 %) e 756 B de RAM (37 %).

## Operação (com tudo montado)

### Alimentação

| Parte | Vem de |
|---|---|
| Nano (pino +5V), MPU-9250, LDRs | trilho de **+5 V** do conector de pilha (J1 pinos 29-30, J2 pino 26). O VIN do Nano não está ligado |
| ESCON (+VCC, pino 4) | conector **14.8V1**, passando pelo fusível F1 e pelo TVS D1 (faixa do ESCON: 10–24 V) |

- Pode ligar o USB do Nano junto com a placa para acompanhar a depuração.
- ⚠️ **Verificar:** na PCB, a **saída** auxiliar de +5 V do ESCON (pino 6) está ligada no mesmo trilho de +5 V da pilha. São duas fontes em paralelo. Meça o trilho com o ESCON ligado e desligado antes do teste longo.
- O `escon_studio_config.md` foi calculado para 13,6 V. Com 14,8 V a corrente continua limitada a 2 A pela configuração do ESCON, e o limite de 12000 rpm continua protegendo contra sobrevelocidade.

### Antes da primeira vez

1. Configure o ESCON no ESCON Studio conforme [`docs/escon_studio_config.md`](docs/escon_studio_config.md) e faça o checklist da seção 8 com o satélite **preso**.
2. Confirme o sentido da roda (item 4 do roteiro de testes abaixo) e ajuste `INVERT_DIR` em `config.h`, se precisar.
3. Confirme a numeração e a calibração dos LDRs (seção *Determinação de atitude*).

### Sequência de operação

1. **Ligue a placa com o satélite PARADO.** Logo depois de ligar, o firmware mede o bias do giroscópio por ~1 s. Se o satélite estiver girando, a calibração é recusada e o bias fica zero (funciona, mas com o erro do sensor, tipicamente < 2 °/s).
2. Espere a linha `# comandos: ...` no monitor serial (de 1,5 a 3 s após ligar, contando o bootloader). A partir daí o azimute já sai pelo TX a cada 200 ms.
3. Coloque o satélite em rotação (60 rpm).
4. O OBC manda **`2`**. A roda começa a absorver a rotação e o corpo cai para menos de 5 rpm em ~8 s.
5. O controle continua ativo, segurando o corpo abaixo de 5 rpm, até o OBC mandar **`1`**.
6. Depois do `1` o motor é desabilitado e a roda gira livre. O atrito dela devolve aos poucos a rotação ao corpo; mande `2` de novo se precisar segurar outra vez.

Durante o detumbling o corpo gira rápido demais para o azimute significar algo. Use o valor de atitude com o satélite parado ou quase parado.

### Saída esperada no monitor serial (115200)

Valores da simulação, partindo de 60 rpm (o bias muda de sensor para sensor):

```
# ADCS POLISAT: pinos em estado seguro
# MPU WHO_AM_I = 0x71
# calibrando bias do giroscopio (mantenha parado)...
# bias Z = 0.812 deg/s
# comandos: 2=detumbling  1=sai  C=calibra  ?=estado  L=LDRs
# MODO 2: detumbling ATIVO
0.10,58.871,5.875
0.20,57.099,5.698
...
# faixa de 5 rpm atingida em 8.44 s (controle segue ativo ate o comando 1)
...
# MODO 1: detumbling PARADO (comando 1)
```

As linhas CSV do modo 2 são `t[s],omega[rpm],torque[mN·m]`. A "faixa atingida" é declarada depois de 0,5 s seguidos abaixo de 5 rpm, por isso aparece ~0,5 s após o cruzamento real.

### Mensagens de erro

| Mensagem | Causa provável | O que fazer |
|---|---|---|
| `ERRO: MPU-9250 nao respondeu no I2C; detumbling bloqueado` | MPU sem alimentação, SDA/SCL (A4/A5) com mau contato, endereço diferente de 0x68 | Conferir solda e 5 V do MPU; reiniciar a placa |
| `WHO_AM_I` diferente de 0x71/0x73/0x70 | Clone com outro chip | Normalmente funciona; confira se ω responde ao girar |
| `ERRO: calibracao recusada (em movimento ou falha I2C); bias mantido` | Satélite se mexeu durante a calibração | Deixe parado e mande `C` (no modo 1) |
| `ERRO: giroscopio indisponivel, detumbling bloqueado` | Recebeu `2` sem o MPU ter iniciado | Resolver o erro do MPU e reiniciar |
| `PARADO (TIMEOUT: ...)` | Não chegou a 5 rpm em 15 s | Quase sempre é o sentido da roda invertido (`INVERT_DIR`) ou o Enable do ESCON. Veja também `GYRO_Z_SIGN` |
| `PARADO (falha de leitura do giroscopio)` | 5 leituras I2C seguidas falharam | Mau contato ou ruído no I2C; verificar cabos e o aterramento do motor |
| `PARADO (ESCON sinalizou erro no Ready)` | ESCON em erro (só com `ESCON_READY_CHECK = true`) | Ver o erro no ESCON Studio (LED vermelho) |
| `ERRO: saia do modo 2 antes de calibrar` | `C` enviado com o detumbling ativo | Mande `1` antes |
| Azimute sempre `-1.00` | Luz fraca demais ou LDRs desconectados | Mande `L` e confira as tensões |

### Ajustes mais comuns (`config.h`)

| Constante | Quando mudar |
|---|---|
| `INVERT_DIR` | A roda acelera no sentido contrário ao do corpo |
| `GYRO_Z_SIGN` | O eixo Z do MPU está montado ao contrário do eixo da roda |
| `LDR_CAL` | Recalibração ou numeração diferente dos LDRs |
| `LDR_GAMMA` | Depois de medir a não linearidade dos LDRs |
| `UART_BAUD` | O OBC usa outra velocidade (máx. recomendado no SoftwareSerial: 38400) |
| `ESCON_READY_CHECK` | Depois de configurar DigOUT4 = Ready no ESCON |
| `PIN_UART_TX` / `PIN_UART_RX` | O OBC não recebe nada: os rótulos Tx/Rx da PCB podem estar do ponto de vista do OBC |

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

> A versão antiga de `escon_studio_config.md` na pasta do simulador cita **D7/D8/D9**. A PCB usa **D3/D4/D5**; a cópia em `docs/` já está corrigida.

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
4. **ESCON:** siga o checklist da seção 8 de [`docs/escon_studio_config.md`](docs/escon_studio_config.md), com o satélite **preso**. Gire o corpo à mão no sentido +Z do giroscópio e mande `2`: a roda tem que acelerar **no mesmo sentido** do corpo. Se acelerar ao contrário, mude `INVERT_DIR` (ou `GYRO_Z_SIGN`, se o eixo do MPU estiver invertido), nunca os dois.
5. **Detumbling no mancal:** a 60 rpm, mande `2`. A simulação da lei discreta (com quantização do PWM) prevê cruzar 5 rpm em **~7,9 s**, com pico de 1,02 A.
6. **UART:** ligue o OBC (ou um conversor USB-serial em J1) e confira as linhas de azimute e os comandos `1`/`2`.
