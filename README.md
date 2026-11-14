# 💡 Poste Sentinela: iluminação pública que se monitora sozinha

> **FIAP · Global Solution 2026.2 · CitizenTech · Project-Based Maker Lab**

| Integrante | RM |
|---|---|
| Ryan Perez Pacheco| RM98782 |
| João Lucas Yudi Redi Handa | RM98458 |
| Guilheme Faustino Vargas | RM98278 |
| Guilherme Dal Posolo Matheus | RM98694 |
| Lucas Laia Manentti | RM97709|

- 🔗 **Simulação no Wokwi:** https://wokwi.com/projects/480680533941189633
- 🎥 **Vídeo demonstrativo (até 6 min):** _(link do YouTube em breve)_

---

## 1. O problema (CitizenTech)

Um poste apagado transforma uma rua em um ponto de insegurança para quem volta a pé do trabalho, da escola ou do ponto de ônibus. Hoje, a falha só chega à prefeitura quando **um cidadão percebe e liga para reclamar**, e o conserto pode levar dias. Ao mesmo tempo, postes que ficam **acesos de dia** desperdiçam energia paga com dinheiro público, e um **poste inclinado ou caído** (batida de carro, vento forte) representa risco de choque e acidente.

**Cidadão beneficiado:** quem circula à noite em vias públicas, sobretudo em periferias e cidades pequenas, onde o problema demora mais a ser notado.

**ODS conectados:** 7 (energia limpa e acessível), 9 (indústria, inovação e infraestrutura) e 11 (cidades e comunidades sustentáveis).

## 2. A solução

O **Poste Sentinela** é um dispositivo com ESP32 instalado no poste. Ele **percebe** o ambiente com 5 sensores, **interpreta** as leituras com uma **Árvore de Decisão (IA) embarcada** e **age sozinho**, sem comando manual:

| Situação detectada pela IA | Ação autônoma do dispositivo |
|---|---|
| `NORMAL` | Acende à noite. Brilho de 100% com pessoas por perto e ~27% com a rua vazia (economia de energia) |
| `LAMPADA_QUEIMADA` | Registra o alerta para a manutenção |
| `ACESA_DE_DIA` | Registra o desperdício de energia |
| `QUEDA_ENERGIA` | Apaga a lâmpada, dá um bipe curto e registra a falta de energia na região |
| `POSTE_INCLINADO` | Alarme sonoro médio e registro de risco |
| `POSTE_CAIDO` | **Desliga a lâmpada por segurança**, dispara o alarme de emergência e registra |

Cada mudança de estado gera um **evento em JSON** (no Serial), no formato que seria enviado a uma central da prefeitura ou ao app de registro de quedas de energia.

## 3. Componentes (Wokwi)

| Componente | Pino ESP32 | Função no projeto |
|---|---|---|
| ESP32 DevKit C v4 | n/a | Processamento e IA embarcada |
| Módulo LDR (`ldrAmbiente`) | GPIO 34 | Luz do céu: identifica se é dia ou noite |
| Módulo LDR (`ldrLampada`) | GPIO 35 | Fica dentro da luminária e mede **só** a luz da lâmpada |
| Potenciômetro (`tensaoRede`) | GPIO 32 | Simula o sensor de tensão da rede (0–250 V) |
| MPU6050 (acelerômetro) | SDA 21 / SCL 22 | **Sensor adicional:** inclinação ou queda do poste |
| Sensor PIR | GPIO 27 | **Sensor adicional:** presença de pessoas na rua |
| LED amarelo + resistor 220 Ω | GPIO 25 (PWM) | A lâmpada do poste, com brilho controlado |
| Buzzer | GPIO 26 | Alarme local |
| Display OLED SSD1306 | SDA 21 / SCL 22 | Painel de status do poste |

## 4. A Inteligência Artificial

### Por que uma Árvore de Decisão embarcada?
- **Roda offline no próprio ESP32**, sem depender de internet. Isso importa porque postes com problema muitas vezes estão em áreas de conectividade ruim.
- **É explicável:** dá para ler cada decisão como regra (ver `ia/metricas.md`). Isso é essencial para um sistema que dispara manutenção paga com dinheiro público.
- É leve: são 10 folhas e profundidade 5, ou seja, poucos `if`s em C.

### Pipeline
```
ia/treinar_modelo.py
  ├─ gera dataset simulado (6.000 leituras) ──► ia/dataset_simulado.csv
  ├─ treina DecisionTreeClassifier (scikit-learn)
  ├─ avalia (acurácia, matriz de confusão)    ──► ia/metricas.md
  └─ exporta a árvore para C                  ──► wokwi/modelo_arvore.h
                                                      │
wokwi/sketch.ino  ── chama classificarEstado() ◄──────┘
```

### Por que dados simulados?
Não existe base pública com leituras de LDR, tensão e acelerômetro em postes. O dataset foi gerado a partir de **faixas físicas reais**:
- Noite: 0,1 a 40 lux
- Dia: 60 a 100.000 lux
- Rede: 220 V ± 6 V, com 15% de casos de subtensão
- Poste em pé: inclinação de ~0° com ruído de 3°

Foi adicionado **ruído de medição** de 5% nos sensores, e 30% dos casos simulam a lâmpada falhando.

### Resultado
- **Acurácia no conjunto de teste: 99,9%** (1.500 amostras nunca vistas no treino)
- Detalhes por classe e matriz de confusão em [`ia/metricas.md`](ia/metricas.md)

### Confiabilidade contra leitura falsa
O estado só muda após **3 leituras seguidas iguais** (1,5 s). Isso evita alarme por um carro passando com farol alto ou por uma vibração momentânea.

## 5. Como executar

### Simulação (Wokwi)
1. Acesse [wokwi.com/projects/new/esp32](https://wokwi.com/projects/new/esp32).
2. Substitua `sketch.ino` e `diagram.json` pelos arquivos da pasta `wokwi/`.
3. Crie as abas `modelo_arvore.h` e `libraries.txt` (botão ▾ ao lado das abas → *New file*) e cole o conteúdo.
4. Clique em ▶ e abra o **Serial Monitor**.

### Retreinar a IA (opcional)
```bash
pip install scikit-learn pandas numpy
python ia/treinar_modelo.py   # regenera dataset, métricas e wokwi/modelo_arvore.h
```

## 6. Roteiro de testes (clique nos sensores durante a simulação)

| # | Cenário | Como simular | Resultado esperado |
|---|---|---|---|
| 1 | Dia normal | `ldrAmbiente` = 20000 lux, `ldrLampada` = 1 lux | `NORMAL`, lâmpada apagada |
| 2 | Anoitece | `ldrAmbiente` = 5 lux, `ldrLampada` = 300 lux | `NORMAL`, lâmpada a ~27% |
| 3 | Pedestre passa | Clique no PIR | Brilho sobe para 100% |
| 4 | Lâmpada queimada | Noite + `ldrLampada` = 1 lux | `LAMPADA_QUEIMADA` |
| 5 | Desperdício | `ldrAmbiente` = 20000, `ldrLampada` = 300 | `ACESA_DE_DIA` |
| 6 | Queda de energia | Gire o potenciômetro para baixo (< 180 V) | `QUEDA_ENERGIA`, bipe curto, lâmpada apaga |
| 7 | Poste inclinado | MPU6050: aceleração X ≈ 0,5 g, Z ≈ 0,87 g (~30°) | `POSTE_INCLINADO`, alarme |
| 8 | Poste caído | MPU6050: X = 1 g, Z = 0 (90°) | `POSTE_CAIDO`, alarme de emergência, lâmpada desligada |
| 9 | Falha de sensor | Remova o MPU6050 do diagrama | Log de erro e o sistema segue funcionando com os outros sensores |

Exemplo de evento gerado:
```json
{"evento":"POSTE_CAIDO","t_ms":48210,"lux_ambiente":5.0,"lux_lampada":1.0,"tensao_rede":220,"inclinacao":89.6,"presenca":false,"alertas":3}
```

## 7. Estrutura do repositório
```
├── README.md
├── wokwi/
│   ├── sketch.ino          # firmware do ESP32
│   ├── modelo_arvore.h     # IA exportada (gerado automaticamente)
│   ├── diagram.json        # circuito do Wokwi
│   └── libraries.txt       # bibliotecas Arduino
├── ia/
│   ├── treinar_modelo.py   # dataset + treino + avaliação + exportação
│   ├── dataset_simulado.csv
│   └── metricas.md
└── docs/                   # prints do circuito e da simulação
```

## 8. Próximos passos
- Enviar os eventos por MQTT para um painel da prefeitura, com mapa das zonas apagadas.
- Usar uma LLM para gerar a ordem de serviço em linguagem natural a partir do evento.
- Coletar leituras reais em campo para retreinar o modelo.
- Usar bateria e energia solar para o dispositivo continuar reportando durante quedas de energia.
