# Métricas do classificador

- Amostras: 6000 (treino 4500 / teste 1500)
- Profundidade da árvore: 5 | folhas: 10
- **Acurácia no teste: 99.9%**

## Distribuição das classes

| Estado | Amostras |
|---|---|
| NORMAL | 3082 |
| LAMPADA_QUEIMADA | 651 |
| ACESA_DE_DIA | 643 |
| QUEDA_ENERGIA | 775 |
| POSTE_INCLINADO | 463 |
| POSTE_CAIDO | 386 |

## Relatório por classe

```
                  precision    recall  f1-score   support

          NORMAL      1.000     0.999     0.999       770
LAMPADA_QUEIMADA      1.000     1.000     1.000       163
    ACESA_DE_DIA      0.994     1.000     0.997       161
   QUEDA_ENERGIA      1.000     1.000     1.000       194
 POSTE_INCLINADO      1.000     0.991     0.996       116
     POSTE_CAIDO      0.990     1.000     0.995        96

        accuracy                          0.999      1500
       macro avg      0.997     0.998     0.998      1500
    weighted avg      0.999     0.999     0.999      1500
```

## Matriz de confusão (linha = real, coluna = previsto)

```
                  NORMAL  LAMPADA_QUEIMADA  ACESA_DE_DIA  QUEDA_ENERGIA  POSTE_INCLINADO  POSTE_CAIDO
NORMAL               769                 0             1              0                0            0
LAMPADA_QUEIMADA       0               163             0              0                0            0
ACESA_DE_DIA           0                 0           161              0                0            0
QUEDA_ENERGIA          0                 0             0            194                0            0
POSTE_INCLINADO        0                 0             0              0              115            1
POSTE_CAIDO            0                 0             0              0                0           96
```

## Regras aprendidas pela árvore

```
|--- tensao_rede <= 187.5
|   |--- inclinacao <= 13.3
|   |   |--- class: 3
|   |--- inclinacao >  13.3
|   |   |--- inclinacao <= 44.9
|   |   |   |--- class: 4
|   |   |--- inclinacao >  44.9
|   |   |   |--- class: 5
|--- tensao_rede >  187.5
|   |--- inclinacao <= 14.9
|   |   |--- lux_ambiente <= 41.6
|   |   |   |--- lux_lampada <= 46.8
|   |   |   |   |--- class: 1
|   |   |   |--- lux_lampada >  46.8
|   |   |   |   |--- class: 0
|   |   |--- lux_ambiente >  41.6
|   |   |   |--- lux_lampada <= 45.2
|   |   |   |   |--- class: 0
|   |   |   |--- lux_lampada >  45.2
|   |   |   |   |--- lux_ambiente <= 65.5
|   |   |   |   |   |--- class: 2
|   |   |   |   |--- lux_ambiente >  65.5
|   |   |   |   |   |--- class: 2
|   |--- inclinacao >  14.9
|   |   |--- inclinacao <= 44.7
|   |   |   |--- class: 4
|   |   |--- inclinacao >  44.7
|   |   |   |--- class: 5
```