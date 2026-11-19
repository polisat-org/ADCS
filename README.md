# Firmware ADCS: Arduino Nano (PCB_LASC_v1.0)

Firmware do microcontrolador da placa ADCS do POLISAT (Cubedesign 2026). Ele faz duas coisas:

1. **Detumbling** no eixo Z, com a roda de reação (maxon EC 20 flat + ESCON Module 24/2) e o giroscópio do MPU-9250. A lei de controle e as constantes são as do simulador (`params.m`); os valores estão copiados em `config.h`.
2. **Determinação de atitude (azimute)** a partir de uma fonte de luz, com 4 LDRs (faces FRONTAL, DIREITO, TRASEIRO e ESQUERDO).

O OBC liga e desliga cada função pela UART (comandos 1–4) e recebe o azimute de volta.

## Arquivos (`adcs_firmware/`)

| Arquivo | Conteúdo |
|---|---|
| `config.h` | **Todas** as constantes: pinos, planta, ganho, escala do ESCON, calibração dos LDRs, UART |
| `adcs_firmware.ino` | `setup()`, loop de 100 Hz, comandos, telemetria de depuração |
| `gyro_mpu9250.*` | Driver mínimo do giroscópio (registradores via `Wire`, sem biblioteca externa) |
| `escon.*` | Torque → PWM 10–90 %, Enable, Direction, Ready |
| `detumbling.*` | Lei τ = Kp·ω, filtro, faixa-alvo e proteções |
| `sun_sensor.*` | Leitura dos LDRs em dois bancos e cálculo do azimute |
| `obc_link.*` | UART com o OBC (SoftwareSerial): comandos 1–4 e azimute em 5 caracteres |

Só usa bibliotecas que vêm com o Arduino (`Wire`, `SoftwareSerial`).

Documentação de apoio: [`docs/escon_studio_config.md`](docs/escon_studio_config.md), com todos os parâmetros do ESCON Studio e a ligação ESCON ↔ Nano.

## Gravar no Arduino

1. Abra `adcs_firmware/adcs_firmware.ino` no Arduino IDE. Os outros arquivos da pasta abrem como abas e são compilados juntos; mantenha a pasta inteira.
2. **Tools → Board:** Arduino Nano. **Processor:** ATmega328P. Se o upload falhar, use *ATmega328P (Old Bootloader)*, que é o caso da maioria dos clones.
3. **Tools → Port:** a porta COM do Nano. Clique em **Upload**.

Não precisa instalar biblioteca. Tamanho atual: 15,5 kB de flash (47 %) e 719 B de RAM (35 %).

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

**Ao ligar:** ligue a placa com o satélite **PARADO**. Logo depois de ligar, o firmware mede o bias do giroscópio por ~1 s. Se o satélite estiver girando, a calibração é recusada e o bias fica zero (funciona, mas com o erro do sensor, tipicamente < 2 °/s). O firmware está pronto quando aparece a linha `# comandos: ...` no monitor serial (de 1,5 a 3 s após ligar, contando o bootloader). Até receber um comando, ele não faz nada e não envia nada.

**Detumbling:**

1. A terra pede o detumbling; o OBC manda **1**. A roda começa a absorver a rotação e o corpo cai para menos de 5 rpm em ~8 s.
2. O ADCS **não envia nada** ao OBC durante nem ao fim do detumbling. O controle continua ativo, segurando o corpo abaixo de 5 rpm, até chegar o comando de parada. Quem decide o fim é a prova.
3. A terra manda parar; o OBC manda **2**. O motor é desabilitado e a roda gira livre. O atrito dela devolve aos poucos a rotação ao corpo; mande **1** de novo se precisar segurar outra vez.

**Azimute:**

1. A terra pede o azimute; o OBC manda **3**. O ADCS começa a ler os LDRs a 10 Hz.
2. A cada 1 s, se houver luz suficiente, o ADCS envia o ângulo médio desse segundo como 5 caracteres (`270.5`). Sem luz suficiente ele não envia nada e continua procurando. Não há timeout.
3. O OBC guarda o último valor recebido no struct de telemetria e o repassa à terra.
4. O OBC manda **4**: o ADCS para de procurar e não envia mais nada.

Os dois modos são independentes e podem ficar ativos juntos. Mas, com o corpo girando, o azimute não significa nada: meça com o satélite parado ou quase parado.

O OBC transmite e recebe na task de housekeeping, de minuto em minuto. Então um comando pode chegar até ~1 min depois do pedido da terra, e o valor que vai para a terra é o último que o ADCS mandou antes da leitura do OBC.

### Saída esperada no monitor serial (115200)

Valores da simulação, partindo de 60 rpm (o bias muda de sensor para sensor):

```
# ADCS POLISAT: pinos em estado seguro
# MPU WHO_AM_I = 0x71
# calibrando bias do giroscopio (mantenha parado)...
# bias Z = 0.812 deg/s
# comandos: 1/2=inicia/encerra detumbling  3/4=inicia/encerra azimute  C ? L
# OBC: comando 1
# detumbling ATIVO
0.10,58.871,5.875
0.20,57.099,5.698
...
# faixa de 5 rpm atingida em 8.44 s (controle segue ativo ate o comando 2)
...
# OBC: comando 2
# detumbling PARADO (comando 2)
# OBC: comando 3
# busca de azimute ATIVA
# buscando azimute: luz insuficiente
# azimute enviado: 270.5
# azimute enviado: 270.4
# OBC: comando 4
# busca de azimute PARADA (comando 4)
```

As linhas CSV do detumbling são `t[s],omega[rpm],torque[mN·m]`. A "faixa atingida" é declarada depois de 0,5 s seguidos abaixo de 5 rpm, por isso aparece ~0,5 s após o cruzamento real.

### Mensagens de erro

| Mensagem | Causa provável | O que fazer |
|---|---|---|
| `ERRO: MPU-9250 nao respondeu no I2C; detumbling bloqueado` | MPU sem alimentação, SDA/SCL (A4/A5) com mau contato, endereço diferente de 0x68 | Conferir solda e 5 V do MPU; reiniciar a placa |
| `WHO_AM_I` diferente de 0x71/0x73/0x70 | Clone com outro chip | Normalmente funciona; confira se ω responde ao girar |
| `ERRO: calibracao recusada (em movimento ou falha I2C); bias mantido` | Satélite se mexeu durante a calibração | Deixe parado e mande `C` pelo USB (com o detumbling desligado) |
| `ERRO: giroscopio indisponivel, detumbling bloqueado` | Recebeu `1` sem o MPU ter iniciado | Resolver o erro do MPU e reiniciar |
| `PARADO (SOBREVELOCIDADE: ...)` | \|ω\| passou de 120 rpm por 0,5 s: o controle está acelerando o corpo | Sentido da roda invertido (`INVERT_DIR`) ou eixo do MPU invertido (`GYRO_Z_SIGN`); mude só um deles |
| `PARADO (falha de leitura do giroscopio)` | 5 leituras I2C seguidas falharam | Mau contato ou ruído no I2C; verificar cabos e o aterramento do motor |
| `PARADO (ESCON sinalizou erro no Ready)` | ESCON em erro (só com `ESCON_READY_CHECK = true`) | Ver o erro no ESCON Studio (LED vermelho) |
| `ERRO: encerre o detumbling (2) antes de calibrar` | `C` enviado com o detumbling ativo | Mande `2` antes |
| `# buscando azimute: luz insuficiente` sem parar | Luz fraca demais ou LDRs desconectados | Mande `L` e confira as tensões |
| `# detumbling ATIVO` mas a roda não gira | ESCON sem alimentação, Enable/PWM mal ligados ou ESCON em erro | Ver os LEDs do ESCON e o checklist de `docs/escon_studio_config.md` |
| Nenhum `# OBC: comando ...` aparece | Baud diferente do huart1, Tx/Rx trocados ou GND não comum | Ver a seção *Protocolo com o OBC* |

### Ajustes mais comuns (`config.h`)

| Constante | Quando mudar |
|---|---|
| `INVERT_DIR` | A roda acelera no sentido contrário ao do corpo |
| `GYRO_Z_SIGN` | O eixo Z do MPU está montado ao contrário do eixo da roda |
| `LDR_CAL` | Recalibração ou numeração diferente dos LDRs |
| `LDR_GAMMA` | Depois de medir a não linearidade dos LDRs |
| `UART_BAUD` | Tem que ser igual ao `BaudRate` do huart1 do OBC (máx. recomendado no SoftwareSerial: 38400) |
| `AZ_TX_PERIOD_MS` | Intervalo entre envios do azimute durante a busca (padrão 1 s) |
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

UART em **9600 8N1** (`UART_BAUD`), pelo SoftwareSerial em D12 (RX) / D11 (TX).

| Sentido | Conteúdo |
|---|---|
| OBC → ADCS | inteiro **1**: inicia o detumbling |
| OBC → ADCS | inteiro **2**: encerra o detumbling (motor desabilitado) |
| OBC → ADCS | inteiro **3**: inicia a busca de azimute |
| OBC → ADCS | inteiro **4**: encerra a busca de azimute |
| ADCS → OBC | azimute como string de **exatamente 5 caracteres**, sem terminador: 270,5° → `270.5`, 45,3° → `045.3` |

- **Comandos:** são o **valor binário** do byte (o OBC faz `atoi()` e envia `comandoADCS`), não o caractere ASCII. Qualquer outro valor é ignorado, inclusive os bytes 0 que sobram se `comandoADCS` tiver mais de uma posição.
- **Ângulo:** vai de `000.0` a `359.9`, arredondado a 0,1°, sempre com zeros à esquerda para manter os 5 caracteres. Só sai durante a busca (comando 3), uma vez por segundo, e só quando há luz suficiente. Fora isso, **nada** sai por esse TX; as mensagens de depuração vão só pelo USB.
- **Detumbling:** o ADCS não envia nada, nem ao atingir a faixa.

**O que precisa estar certo no OBC:**

1. `huart1` com **BaudRate = 9600** (ou mude `UART_BAUD` para o mesmo valor). O padrão do CubeMX é 115200, e o SoftwareSerial não recebe de forma confiável nessa velocidade.
2. `char angulo[6]`: 5 caracteres mais o `'\0'`, para o `Receive_IT(..., sizeof(angulo) - 1)` pegar exatamente 5 bytes e o `printf("%s")` ter o terminador.
3. **Alinhamento:** como a recepção é por contagem fixa de 5 bytes, se o OBC começar a ouvir no meio de uma mensagem (por exemplo, ao reiniciar) ou perder um byte, **todas** as leituras seguintes ficam deslocadas (`5270.`, `4270.`...). O ADCS envia as 5 letras de uma vez (~5 ms) com 1 s de intervalo, o que torna isso raro, mas não impossível. A solução robusta é no OBC: `HAL_UARTEx_ReceiveToIdle_IT`, que separa as mensagens pelo silêncio entre elas.
4. **Erro de overrun:** se o huart1 tiver um erro (ORE, ruído), o HAL aborta a recepção e o `HAL_UART_RxCpltCallback` não é mais chamado. Rearme o `HAL_UART_Receive_IT` também em `HAL_UART_ErrorCallback`, senão o OBC para de receber o ângulo até reiniciar.
5. GND comum entre OBC e ADCS (pelo conector de pilha).

**Limitação do SoftwareSerial:** ele é half-duplex. Um comando que chega exatamente durante os ~5 ms de envio do ângulo pode se perder (~0,5 % do tempo durante a busca). Se o efeito não aparecer, o OBC deve reenviar.

### Proteções do detumbling

O detumbling não tem timeout: só termina pelo comando 2. O motor é desligado automaticamente em três casos:

- **Sobrevelocidade:** |ω| > 120 rpm por 0,5 s. Com o sentido certo, o controle só tira rotação do corpo, então isso só acontece com a roda ou o giroscópio invertidos. Simulado: partindo de 60 rpm com o sentido invertido, desliga em 2,8 s.
- **Giroscópio:** 5 leituras seguidas com falha.
- **ESCON:** Ready baixo (só com `ESCON_READY_CHECK = true`).

Nesses casos o ADCS também não avisa o OBC (o protocolo não prevê); a causa aparece só no USB.

## Depuração pelo USB (115200)

| Comando | Ação |
|---|---|
| `1` `2` `3` `4` | Mesmos comandos do OBC, digitados como caracteres |
| `C` | Recalibra o bias do giroscópio (satélite **parado**, detumbling desligado) |
| `?` | Estado: detumbling e busca ligados ou não, ω, faixa, tempo até a faixa, Ready, último azimute enviado |
| `L` | Lê os LDRs agora e mostra tensão, valor normalizado, x, y e azimute |

Com o detumbling ativo sai uma linha CSV a 10 Hz: `t[s],omega[rpm],torque[mN·m]`.

## Determinação de atitude

```
n_i = (V_i / LDR_CAL[i]) ^ (1/LDR_GAMMA)
az  = atan2(n_DIREITO − n_ESQUERDO, n_FRONTAL − n_TRASEIRO)    → [0, 360)°
```

0° = luz de frente para a face FRONTAL, 90° = face DIREITA. A diferença entre faces opostas cancela a luz ambiente comum às duas. O valor enviado ao OBC é a média vetorial das amostras de 10 Hz do último segundo, que não salta em 0/360°.

**Premissas a confirmar** (em `config.h`, seção 7):

1. **Numeração:** LDR 1 = FRONTAL, 2 = DIREITO, 3 = TRASEIRO, 4 = ESQUERDO. Se for outra, reordene `LDR_CAL`.
2. **Significado da calibração (0,434 / 0,637 / 0,502 / 0,570):** é a leitura de cada LDR sob a **mesma** luz, e o firmware **divide** por ela. A unidade não importa, porque só a razão entre os valores entra no `atan2`. Se esses números forem ganhos que deveriam **multiplicar** a leitura, troque cada valor por 1/valor.
3. **`LDR_GAMMA` = 1:** num modelo cosseno ideal o erro é zero. Mas se o LDR responder com γ = 0,7 e o firmware ficar em 1, o erro chega a **6,7°**, acima da faixa de 5° do regulamento. Meça γ na bancada.

## Roteiro de testes na bancada

1. **Sem motor:** grave, abra o monitor serial e confira `WHO_AM_I` (0x71, 0x73 ou 0x70) e o bias.
2. **LDRs:** aponte a lanterna para cada face e mande `L`. A face iluminada deve ter o maior `n`, e o `az` deve dar ~0/90/180/270°.
3. **Varredura de azimute:** gire a fonte de 0° a 360° em passos de 15° e registre o erro. Ele vai para o relatório (HLR-ADCS-03) e, se preciso, para o ajuste de `LDR_GAMMA` ou uma tabela de correção.
4. **ESCON:** siga o checklist da seção 8 de [`docs/escon_studio_config.md`](docs/escon_studio_config.md), com o satélite **preso**. Gire o corpo à mão no sentido +Z do giroscópio e mande `1`: a roda tem que acelerar **no mesmo sentido** do corpo. Se acelerar ao contrário, mude `INVERT_DIR` (ou `GYRO_Z_SIGN`, se o eixo do MPU estiver invertido), nunca os dois.
5. **Detumbling no mancal:** a 60 rpm, mande `1` (e `2` para encerrar). A simulação da lei discreta (com quantização do PWM) prevê cruzar 5 rpm em **~7,9 s**, com pico de 1,02 A.
6. **UART:** ligue o OBC e confira no USB do Nano as linhas `# OBC: comando N` e, com o comando 3, o ângulo chegando no OBC. Com um conversor USB-serial em J1 é preciso um terminal que envie bytes binários (ex.: RealTerm), porque digitar `1` no PuTTY envia o ASCII 0x31, que o ADCS ignora.
