# Configuração do ESCON Module 24/2 no ESCON Studio

**Equipe:** POLISAT · **Subsistema:** ADCS · **Projeto:** Cubedesign 2026 — Detumbling
**Revisão:** 19/10/2026 (após revisão estrutural: I_b = 3,067×10⁻³ kg·m², I_r = 2,94×10⁻⁵ kg·m²)

Conjunto: **maxon EC 20 flat 3 W, 6 V, com Hall (351098)** + **ESCON Module 24/2 (466023)**,
comandado por Arduino Nano via PWM (`adcs_firmware/`, pinagem da PCB_LASC_v1.0).

> Cópia de `Simulador/escon_studio_config.md` com a pinagem corrigida para a PCB
> (D3/D4/D5/D6) e os comandos do firmware atual (`2`/`1`). Os valores do ESCON Studio não mudaram.

> Os nomes das opções seguem a interface em inglês do ESCON Studio (Startup Wizard).
> Podem variar um pouco entre versões do software.

---

## 0. Resumo — todos os valores

| # | Tela do Startup Wizard | Parâmetro | Valor |
|---|---|---|---|
| 1 | Motor Type | Tipo de motor | **maxon EC motor** |
| 2 | Motor Data | Speed Constant | **1620 rpm/V** |
| 3 | Motor Data | Thermal Time Constant Winding | **3.69 s** |
| 4 | Motor Data | Number of Pole Pairs | **4** |
| 5 | System Data | Max. Permissible Speed | **12000 rpm** |
| 6 | System Data | Nominal Current | **0.560 A** |
| 7 | System Data | Max. Output Current Limit | **2.000 A** |
| 8 | Detection of Rotor Position | Sensor | **Digital Hall Sensors** |
| 9 | Detection of Rotor Position | Hall Sensor Polarity | **maxon (padrão)** |
| 10 | Speed Sensor | Sensor | **Available Hall Sensors** |
| 11 | Mode of Operation | Modo | **Current Controller** |
| 12 | Enable | Funcionalidade | **Enable & Direction** |
| 13 | Enable | Enable → entrada | **Digital Input 2**, **High active** |
| 14 | Enable | Direction → entrada | **Digital Input/Output 3**, **High active = CW** (confirmar no teste da seção 5) |
| 15 | Set Value | Tipo | **PWM Set Value** |
| 16 | Set Value | Entrada | **Digital Input 1** |
| 17 | Set Value | Current at 10.0 % | **0.000 A** |
| 18 | Set Value | Current at 90.0 % | **2.000 A** |
| 19 | Current Limit | Tipo | **Fixed Current Limit** |
| 20 | Current Limit | Valor | **2.000 A** |
| 21 | Offset | Tipo / valor | **Fixed Offset = 0.000 A** |
| 22 | Digital Inputs & Outputs | DigIN/DigOUT4 | **Ready, High active** (lido em D6; só desliga o motor com `ESCON_READY_CHECK = true`) |
| 23 | Analog Outputs | AnOUT1 | **Actual Current Averaged**: 0.000 V = 0.000 A, 4.000 V = 2.000 A (opcional) |
| 24 | Analog Outputs | AnOUT2 | **Actual Speed Averaged**: 0.000 V = 0 rpm, 4.000 V = 12000 rpm (opcional) |
| 25 | Regulation Tuning | Current controller | **Auto Tuning** (com o volante montado) |

As seções abaixo explicam de onde vem cada número.

---

## 1. Antes de abrir o Wizard

| Item | Valor / ação |
|---|---|
| Alimentação +VCC (pino 4) | **13.6 V** (faixa nominal 10–24 V; desliga por subtensão em 7.2 V e por sobretensão em 29 V) |
| Conexão | USB do ESCON → PC com ESCON Studio |
| Firmware do ESCON | Atualizar pela ferramenta *Firmware Update* do ESCON Studio antes de configurar |
| Motor | Conectado e **com o volante montado** (o Auto Tuning precisa da carga real) |
| Satélite | **Preso na bancada** durante a configuração e o Auto Tuning. O motor gira, e o satélite não pode estar no mancal a ar. |
| Arduino | Pode ficar desconectado durante o Wizard |

---

## 2. Dados do motor (telas *Motor Type*, *Motor Data*, *System Data*)

Fonte: datasheet maxon *EC 20 flat Ø20 mm, brushless, 3 W* (edição 2023), coluna **351098** (6 V, com Hall).

| Parâmetro | Valor | Origem |
|---|---|---|
| Motor Type | maxon EC motor | Motor brushless (EC) |
| Speed Constant | 1620 rpm/V | Datasheet, linha 13 (= `mot.speed_cnst`) |
| Thermal Time Constant Winding | 3.69 s | Datasheet, linha 19 |
| Number of Pole Pairs | 4 | Datasheet, linha 29 |
| Nominal Current | 0.560 A | Datasheet, linha 6 (= `mot.I_nom` = 0.559 A) |
| Max. Permissible Speed | 12000 rpm | 80 % do limite mecânico de 15000 rpm (`margin_speed` × `mot.n_max`) |
| Max. Output Current Limit | 2.000 A | Fundo de escala do firmware (`DRIVER_I_FS`) |

**Velocidade máxima (12000 rpm).** O enrolamento de 6 V alimentado com 13.6 V tem rotação a vazio
elétrica de cerca de 21600 rpm, acima do limite mecânico de 15000 rpm. Por isso o ESCON **precisa**
desse limite como proteção de sobrevelocidade. 12000 rpm é o teto operacional do `params.m`. A roda
termina o detumbling em **~5740 rpm**, então sobra mais que o dobro de margem.
Não coloque um valor menor que ~7000 rpm: abaixo disso a roda pode atingir o limite antes de o
detumbling terminar. No pior caso, ela absorve toda a rotação inicial e chega a ~6260 rpm.

**Corrente máxima (2.000 A).** Esse número precisa ser **igual** ao `DRIVER_I_FS` do firmware, porque
o firmware calcula o duty do PWM assumindo que 90 % corresponde a 2.0 A. Fisicamente, em 13.6 V
a corrente com a roda parada já é limitada a 0.98·13.6/6.67 ≈ 2.0 A, então esse limite não restringe
nada além do que a tensão já restringe.
O pico real de corrente é **1.02 A** com o KP novo e **1.66 A** com o KP antigo do firmware.
Se a sua versão do ESCON Studio não aceitar 2.000 A nesse campo, use o maior valor que ela aceitar
e ajuste o `DRIVER_I_FS` do firmware e a linha 18 (Current at 90 %) para o mesmo número.

### Proteção térmica I²t (por que Nominal Current e Thermal Time Constant importam)

O ESCON estima a temperatura do enrolamento a partir de *Nominal Current* e *Thermal Time Constant Winding*.
Quando o nível I²t chega a 100 %, ele **limita a corrente ao valor nominal (0.56 A)**. O limite só é
liberado quando o nível volta para baixo de 90 %. O simulador (`reaction_wheel_design.m`) **não** modela
esse limite, então ele foi simulado à parte com o modelo de 1ª ordem do I²t:

| Ganho no firmware | τ enrolamento | Limitação I²t? | Detumbling |
|---|---|---|---|
| KP novo (9.53×10⁻⁴) | 3.69 s | Não (pico do nível ≈ 76 %) | 8.00 s ✅ |
| KP novo (9.53×10⁻⁴) | 2.67 s | Não (pico do nível ≈ 93 %) | 8.00 s ✅ |
| KP antigo (1.553×10⁻³) | 3.69 s | Sim, em t = 0.62 s | 5.68 s ✅ |
| KP antigo (1.553×10⁻³) | 2.67 s | Sim, em t = 0.40 s | 5.97 s ✅ |

Os quatro casos cumprem o requisito de 10 s com os valores de datasheet. **Não aumente a Nominal Current
para "fugir" do I²t.** Ela é a única proteção térmica do enrolamento, e o projeto não precisa disso.

> **Divergência com `params.m`:** o `params.m` usa constantes térmicas de outra edição do datasheet
> (`tau_th_wdg` = 2.67 s, `Rth_wh` = 8.68 K/W, `Rth_ha` = 20.6 K/W, `tau_th_mot` = 34.1 s,
> `I_noload` = 81.3 mA, `n_noload` = 8720 rpm, `tau_nom` = 3.04 mN·m). O datasheet 2023 do 351098 traz
> 3.69 s, 8.41 K/W, 19.2 K/W, 31.8 s, 53.6 mA, 9070 rpm e 3.22 mN·m. Use no ESCON o valor do
> datasheet que acompanha o motor que vocês têm. A tabela acima mostra que os dois funcionam.

---

## 3. Sensores (telas *Detection of Rotor Position* e *Speed Sensor*)

| Parâmetro | Valor |
|---|---|
| Detection of Rotor Position | Digital Hall Sensors |
| Hall Sensor Polarity | maxon (padrão) |
| Speed Sensor | Available Hall Sensors (não há encoder) |

O Hall é alimentado pelo ESCON (+5 V, pino 6, máx. 30 mA). O motor aceita 4.5–24 V nos Halls.

---

## 4. Modo, Enable e Set Value

### 4.1 Mode of Operation: **Current Controller**

O firmware comanda **torque** (τ = Kt·I). Só em controle de corrente o mapeamento duty → torque é
linear. Em *Speed Controller* o comando viraria velocidade e a lei de controle deixaria de valer.
As telas de *Speed Ramp* não aparecem nesse modo.

### 4.2 Enable: **Enable & Direction**

| Função | Entrada ESCON | Polaridade | Pino Arduino |
|---|---|---|---|
| Enable | Digital Input 2 (pino 16) | High active | D4 (`PIN_ESCON_ENABLE`) |
| Direction | Digital Input/Output 3 (pino 15) | High active = CW | D5 (`PIN_ESCON_DIR`) |

As entradas reconhecem nível lógico 1 acima de 2.4 V e nível 0 abaixo de 1.0 V, então os 5 V do Arduino funcionam direto.

### 4.3 Set Value: **PWM Set Value** em **Digital Input 1**

| Parâmetro | Valor |
|---|---|
| Entrada | Digital Input 1 (pino 17), a única que aceita PWM |
| Current at 10.0 % | 0.000 A |
| Current at 90.0 % | 2.000 A |

- Precisa bater com `DUTY_MIN_PCT = 10` e `DUTY_MAX_PCT = 90` do firmware.
- O PWM do pino D3 do Nano (Timer2) roda a **~490 Hz**, dentro da faixa do ESCON (10 Hz–5 kHz, duty 10–90 %).
- A resolução é de 8 bits: 204 passos entre 10 % e 90 %, ou **~9.8 mA (0.058 mN·m) por passo**.
- O ESCON dá erro *PWM Set Value Input out of Range* se a frequência cair abaixo de ~5 Hz, o que inclui
  duty de 0 % ou 100 %. O firmware já mantém 10 % (comando zero) com o motor parado e aplica o PWM
  **antes** do Enable, como a maxon recomenda.

### 4.4 Current Limit: **Fixed Current Limit = 2.000 A**

### 4.5 Offset: **Fixed Offset = 0.000 A**

---

## 5. Entradas/saídas extras (opcionais)

AnOUT1/AnOUT2 chegam em A7/A6 do Nano pela PCB, mas o firmware não as lê. Ready chega em D6.

| Saída | Função | Escala |
|---|---|---|
| DigIN/DigOUT4 (pino 14) | Ready, High active | Nível alto = ESCON sem erro |
| AnOUT1 (pino 20) | Actual Current Averaged | 0.000 V = 0.000 A, 4.000 V = 2.000 A |
| AnOUT2 (pino 19) | Actual Speed Averaged | 0.000 V = 0 rpm, 4.000 V = 12000 rpm |

As saídas analógicas vão de −4 a +4 V. **Não ligue numa entrada analógica do Arduino** sem um
divisor ou deslocamento de nível, porque a corrente negativa gera tensão negativa.

---

## 6. Regulation Tuning

| Parâmetro | Valor |
|---|---|
| Método | **Auto Tuning** (Regulation Tuning → Auto Tuning) |
| Condição | Volante montado, satélite preso na bancada, +VCC = 13.6 V |

O Auto Tuning identifica a carga real (motor + volante) e ajusta o controlador de corrente.
Refaça sempre que trocar o volante.

---

## 7. Ligação física Arduino ↔ ESCON Module 24/2

| ESCON (pino) | Sinal | Ligar em |
|---|---|---|
| 1 | Motor winding 1 | Motor pino 8 (winding 1) |
| 2 | Motor winding 2 | Motor pino 7 (winding 2) |
| 3 | Motor winding 3 | Motor pino 6 (winding 3) |
| 4 | +VCC | EPS 13.6 V |
| 5 | Power_GND / GND | EPS GND **e** motor pino 5 (GND dos Halls) |
| 6 | +5 VDC | Motor pino 1 (VHall) |
| 7 | Hall sensor 1 | Motor pino 3 (Hall 1) |
| 8 | Hall sensor 2 | Motor pino 4 (Hall 2) |
| 9 | Hall sensor 3 | Motor pino 2 (Hall 3) |
| 14 | DigIN/DigOUT4 | Arduino **D6** (Ready) |
| 15 | DigIN/DigOUT3 | Arduino **D5** (Direction) |
| 16 | DigIN2 | Arduino **D4** (Enable) |
| 17 | DigIN1 | Arduino **D3** (PWM) |
| 18 | GND | Arduino **GND**, obrigatório: referência comum dos sinais |

**Indutor de motor:** o ESCON Module 24/2 não tem choke interno. Pela fórmula do Hardware Reference (§5.1.5):

L_ext ≥ ½ · ( V_CC / (6 · f_PWM · I_N) − 0.3 · L_motor )
     = ½ · ( 13.6 / (6 · 53600 · 0.56) − 0.3 · 0.639 mH )
     = ½ · ( 75.5 µH − 191.7 µH ) < 0 → **não precisa de indutor externo**.

O resultado só fica positivo acima de V_CC ≈ 34 V, que já passa do máximo do módulo (24 V). Então vale para qualquer alimentação permitida.

---

## 8. Checklist de validação (depois de clicar *Finish*)

Faça com o satélite **preso** e o Arduino conectado.

1. **Sem erro:** LED verde aceso, sem LED vermelho. Em *Diagnostics*, nenhum erro ativo.
2. **Zero em repouso:** com o firmware ocioso (duty 10 %), *Controller Monitor* mostra corrente ≈ 0 A
   e a roda parada.
3. **Escala:** gere um comando conhecido (ex.: torque de 2.94 mN·m = 0.5 A, duty 30 %). O *Controller Monitor*
   deve mostrar **0.50 A** (±0.02 A).
4. **Sentido:** gire o satélite à mão no sentido +Z do giroscópio (ω_z > 0) e mande `2`.
   **A roda tem que acelerar no MESMO sentido da rotação do corpo**, que recebe a reação contrária.
   Se acelerar ao contrário, mude `INVERT_DIR = true` no firmware, ou troque a polaridade da Direction no
   ESCON. Não faça as duas coisas.
5. **Parada:** mande `1`. A corrente deve ir a 0 e o Enable a LOW, e a roda perde velocidade só por atrito.
6. **Gravar:** os parâmetros do Wizard ficam salvos no ESCON. Exporte também um backup
   (*File → Export Parameters*) e versione junto com este arquivo.

---

## 9. Dependências com o firmware e o `params.m`

| Valor no ESCON | Tem que bater com |
|---|---|
| Max. Output Current Limit = 2.000 A, Current at 90 % = 2.000 A | `DRIVER_I_FS = 2.0f` (`config.h`) |
| Current at 10 % / 90 % | `DUTY_MIN_PCT = 10`, `DUTY_MAX_PCT = 90` (`config.h`) |
| Enable em DigIN2 / Direction em DigIN3 / PWM em DigIN1 | `PIN_ESCON_ENABLE = 4`, `PIN_ESCON_DIR = 5`, `PIN_ESCON_PWM = 3` (`config.h`) |
| Max. Permissible Speed = 12000 rpm | `margin_speed · mot.n_max` (`params.m`) |
| Nominal Current = 0.560 A | `mot.I_nom` (`params.m`) |

> O firmware atual já usa `I_B = 3.067e-3` e **KP = 9.53×10⁻⁴ N·m·s/rad** (`config.h`), os
> valores do `params.m` revisado.

---

## Fontes

- maxon, *EC 20 flat Ø20 mm, brushless, 3 W* — Product Datasheet 2023 (351098)
- maxon, *ESCON Module 24/2 Hardware Reference*, rel8422, Nov/2018: dados técnicos (§2.1), pinagem (§3.3),
  entradas digitais e PWM (§3.3.4), choke (§5.1.5)
- maxon Support, *maxon controllers: I2t Limitation & Motor winding protection*
