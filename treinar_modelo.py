"""
Poste Sentinela - treino do classificador de estado do poste.

O que este script faz:
  1. Gera um dataset SIMULADO de leituras de sensores (justificativa no README).
  2. Treina uma Árvore de Decisão (scikit-learn).
  3. Mede o desempenho (acurácia, relatório por classe, matriz de confusão).
  4. Exporta a árvore como função C (modelo_arvore.h) para rodar no ESP32,
     sem internet e sem biblioteca de ML (TinyML "na unha").

Como rodar:
  pip install scikit-learn pandas numpy
  python treinar_modelo.py
"""
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from sklearn.model_selection import train_test_split
from sklearn.tree import DecisionTreeClassifier, export_text

PASTA = Path(__file__).parent
SEMENTE = 42
N_AMOSTRAS = 6000

FEATURES = ["lux_ambiente", "lux_lampada", "tensao_rede", "inclinacao"]
CLASSES = [
    "NORMAL",            # 0 - tudo certo (dia com lâmpada apagada ou noite com lâmpada acesa)
    "LAMPADA_QUEIMADA",  # 1 - é noite, tem energia, mas a lâmpada não acende
    "ACESA_DE_DIA",      # 2 - desperdício: lâmpada acesa com sol
    "QUEDA_ENERGIA",     # 3 - tensão da rede abaixo do mínimo
    "POSTE_INCLINADO",   # 4 - inclinação entre 15° e 45° (risco)
    "POSTE_CAIDO",       # 5 - inclinação acima de 45° (emergência)
]

rng = np.random.default_rng(SEMENTE)


def rotular(lux_amb, lux_lamp, tensao, incl):
    """Regra física usada para rotular os dados simulados (ordem = prioridade)."""
    if incl > 45:
        return 5
    if incl > 15:
        return 4
    if tensao < 180:
        return 3
    noite = lux_amb < 50
    lampada_acesa = lux_lamp > 40
    if noite and not lampada_acesa:
        return 1
    if not noite and lampada_acesa:
        return 2
    return 0


def gerar_amostra():
    # Cenário base: dia ou noite
    noite = rng.random() < 0.5
    lux_amb = 10 ** rng.uniform(-1, 1.6) if noite else 10 ** rng.uniform(1.8, 5)

    # Rede elétrica: 85% normal (~220V), 15% queda/subtensão
    tensao = rng.normal(220, 6) if rng.random() < 0.85 else rng.uniform(0, 175)

    # Lâmpada: segue o esperado em 70% dos casos, falha nos outros 30%
    deveria_acender = noite and tensao >= 180
    acesa = deveria_acender if rng.random() < 0.7 else not deveria_acender
    if tensao < 180:
        acesa = False  # sem energia não acende
    lux_lamp = rng.uniform(80, 1000) if acesa else rng.uniform(0.1, 15)

    # Poste: 85% em pé (ruído do acelerômetro), 15% inclinado ou caído
    r = rng.random()
    if r < 0.85:
        incl = abs(rng.normal(0, 3))
    elif r < 0.93:
        incl = rng.uniform(16, 44)
    else:
        incl = rng.uniform(46, 90)

    # Ruído de medição dos sensores
    lux_amb *= rng.normal(1, 0.05)
    lux_lamp *= rng.normal(1, 0.05)
    tensao += rng.normal(0, 2)
    incl += rng.normal(0, 1)

    lux_amb, lux_lamp = max(lux_amb, 0.1), max(lux_lamp, 0.1)
    tensao, incl = max(tensao, 0), min(max(incl, 0), 90)
    return [lux_amb, lux_lamp, tensao, incl, rotular(lux_amb, lux_lamp, tensao, incl)]


def exportar_para_c(arvore, caminho):
    """Converte a árvore treinada em if/else C para o ESP32."""
    t = arvore.tree_

    def no(i, nivel):
        pad = "  " * nivel
        if t.children_left[i] == -1:  # folha
            classe = int(np.argmax(t.value[i][0]))
            return f"{pad}return {classe};  // {CLASSES[classe]}\n"
        f = FEATURES[t.feature[i]]
        lim = t.threshold[i]
        return (
            f"{pad}if ({f} <= {lim:.4f}f) {{\n"
            + no(t.children_left[i], nivel + 1)
            + f"{pad}}} else {{\n"
            + no(t.children_right[i], nivel + 1)
            + f"{pad}}}\n"
        )

    nomes = ",\n  ".join(f'"{c}"' for c in CLASSES)
    codigo = (
        "// ARQUIVO GERADO AUTOMATICAMENTE por ia/treinar_modelo.py - não editar à mão.\n"
        "// Árvore de Decisão treinada em Python e convertida para C (roda 100% no ESP32).\n"
        "#pragma once\n\n"
        f"const char* NOMES_ESTADO[] = {{\n  {nomes}\n}};\n\n"
        "int classificarEstado(float lux_ambiente, float lux_lampada,\n"
        "                      float tensao_rede, float inclinacao) {\n"
        + no(0, 1)
        + "}\n"
    )
    caminho.write_text(codigo, encoding="utf-8")


def main():
    dados = pd.DataFrame(
        [gerar_amostra() for _ in range(N_AMOSTRAS)], columns=FEATURES + ["estado"]
    )
    dados.round(3).to_csv(PASTA / "dataset_simulado.csv", index=False)

    X, y = dados[FEATURES], dados["estado"]
    X_tr, X_te, y_tr, y_te = train_test_split(
        X, y, test_size=0.25, random_state=SEMENTE, stratify=y
    )

    modelo = DecisionTreeClassifier(max_depth=6, min_samples_leaf=5, random_state=SEMENTE)
    modelo.fit(X_tr, y_tr)
    pred = modelo.predict(X_te)

    nomes_presentes = [CLASSES[i] for i in sorted(y.unique())]
    acc = accuracy_score(y_te, pred)
    relatorio = classification_report(y_te, pred, target_names=nomes_presentes, digits=3)
    matriz = pd.DataFrame(
        confusion_matrix(y_te, pred), index=nomes_presentes, columns=nomes_presentes
    )

    md = [
        "# Métricas do classificador\n",
        f"- Amostras: {len(dados)} (treino {len(X_tr)} / teste {len(X_te)})",
        f"- Profundidade da árvore: {modelo.get_depth()} | folhas: {modelo.get_n_leaves()}",
        f"- **Acurácia no teste: {acc:.1%}**\n",
        "## Distribuição das classes\n",
        "| Estado | Amostras |\n|---|---|",
        *[f"| {CLASSES[k]} | {v} |" for k, v in y.value_counts().sort_index().items()],
        "\n## Relatório por classe\n",
        "```\n" + relatorio + "```\n",
        "## Matriz de confusão (linha = real, coluna = previsto)\n",
        "```\n" + matriz.to_string() + "\n```\n",
        "## Regras aprendidas pela árvore\n",
        "```\n" + export_text(modelo, feature_names=FEATURES, decimals=1) + "```",
    ]
    (PASTA / "metricas.md").write_text("\n".join(md), encoding="utf-8")

    exportar_para_c(modelo, PASTA.parent / "wokwi" / "modelo_arvore.h")
    print(f"Acurácia no teste: {acc:.1%}")
    print(relatorio)
    print("Gerados: ia/dataset_simulado.csv, ia/metricas.md, wokwi/modelo_arvore.h")


if __name__ == "__main__":
    main()
