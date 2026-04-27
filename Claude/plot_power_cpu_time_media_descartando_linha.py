"""
plot_power_cpu_time.py
======================
Gera gráficos de potência (PSys / (User Time + System Time)) por benchmark,
linguagem e limite de potência para dois experimentos: single core e multi core.

Uso:
    python plot_power_cpu_time.py \
        --single measurements-single-elite-perf-01042026.csv \
        --mult   measurements-mult-elite-perf-27-03-2026.csv

Dependências:
    pip install pandas matplotlib
"""

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import pandas as pd
import numpy as np


# ---------------------------------------------------------------------------
# Configuração
# ---------------------------------------------------------------------------

LANG_COLORS = {
    "C":      "#185FA5",
    "C++":    "#0F6E56",
    "Java":   "#BA7517",
    "Python": "#993C1D",
}
LANGUAGES = ["C", "C++", "Java", "Python"]

SINGLE_POWERS = [-1, 2, 10, 15, 25]
MULT_POWERS   = [-1, 1, 2, 4, 8, 10, 15, 25]

def power_label(p):
    return "unlimited" if p == -1 else f"{p}W"


# ---------------------------------------------------------------------------
# Leitura e pré-processamento
# ---------------------------------------------------------------------------

def load(path: str, experiment: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    df.columns = df.columns.str.strip()
    df["Program"] = df["Program"].str.strip()
    df["experiment"] = experiment
    df["cpu_time_ms"] = df["User Time"] + df["System Time"]
    df["power_W"] = df["PSys"] / df["cpu_time_ms"] * 1000
    return df


def compute_summary(df: pd.DataFrame) -> pd.DataFrame:
    def _trim_group_by_power(grp: pd.DataFrame) -> pd.DataFrame:
        vals = grp["power_W"].dropna().values
        n = vals.size
        if n == 0:
            return grp
        k = int(n * 0.15)
        if k == 0 or (n - 2 * k) <= 0:
            return grp
        grp_sorted = grp.sort_values("power_W")
        return grp_sorted.iloc[k : n - k]

    group_cols = ["experiment", "Language", "Program", "PowerLimit"]

    def _agg_after_trim(grp: pd.DataFrame) -> pd.Series:
        t = _trim_group_by_power(grp)
        def mean_or_nan(s: pd.Series) -> float:
            arr = s.dropna()
            return float(arr.mean()) if not arr.empty else float("nan")

        return pd.Series(
            {
                "mean_power_W": mean_or_nan(t["power_W"]),
                "mean_user_time": mean_or_nan(t["User Time"]),
                "mean_system_time": mean_or_nan(t["System Time"]),
            }
        )

    summary = df.groupby(group_cols).apply(_agg_after_trim).reset_index()

    summary = summary.rename(
        columns={"Language": "language", "Program": "benchmark", "PowerLimit": "power_limit"}
    )

    cols = ["experiment", "language", "benchmark", "power_limit", "mean_power_W", "mean_user_time", "mean_system_time"]
    return summary[cols]


# ---------------------------------------------------------------------------
# Plot
# ---------------------------------------------------------------------------

def plot_benchmark(summary: pd.DataFrame, benchmark: str, out_dir: Path,
                   scale: str = "linear") -> None:
    """Gera um único arquivo PNG com dois subplots (single | multi) para um benchmark."""

    fig, axes = plt.subplots(1, 2, figsize=(14, 5), sharey=False)
    fig.suptitle(
        f"Potência média (descontando 15% extremos) — PSys / (User Time + System Time)\nBenchmark: {benchmark}",
        fontsize=13, fontweight="bold", y=1.02,
    )

    configs = [
        ("single", SINGLE_POWERS, axes[0], "Single core"),
        ("mult",   MULT_POWERS,   axes[1], "Multi core"),
    ]

    n_langs = len(LANGUAGES)

    for exp, powers, ax, title in configs:
        sub = summary[(summary["experiment"] == exp) & (summary["benchmark"] == benchmark)]

        x = range(len(powers))
        bar_width = 0.8 / n_langs

        for i, lang in enumerate(LANGUAGES):
            lang_data = sub[sub["language"] == lang].set_index("power_limit")["mean_power_W"]
            values = [lang_data.get(p, float("nan")) for p in powers]
            offsets = [xi + (i - n_langs / 2 + 0.5) * bar_width for xi in x]
            ax.bar(
                offsets, values,
                width=bar_width * 0.9,
                color=LANG_COLORS[lang],
                alpha=0.85,
                label=lang,
            )

        ax.set_title(title, fontsize=11)
        ax.set_xticks(list(x))
        ax.set_xticklabels([power_label(p) for p in powers], rotation=30, ha="right", fontsize=9)
        ax.set_xlabel("Limite de potência", fontsize=9)
        ax.set_ylabel("W (média)", fontsize=9)
        ax.set_yscale(scale)
        ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f W"))
        ax.grid(axis="y", color="gray", alpha=0.2, linestyle="--")
        ax.spines[["top", "right"]].set_visible(False)
        ax.legend(fontsize=8, loc="upper left")

    fig.tight_layout()
    safe_name = benchmark.replace("-", "_")
    out_path = out_dir / f"power_cpu_{safe_name}_{scale}.png"
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"  Salvo: {out_path}")


def plot_all(summary: pd.DataFrame, out_dir: Path, scale: str = "linear") -> None:
    benchmarks = sorted(summary["benchmark"].unique())
    print(f"\nGerando {len(benchmarks)} gráfico(s) — escala {scale}...")
    for bench in benchmarks:
        plot_benchmark(summary, bench, out_dir, scale)


# ---------------------------------------------------------------------------
# Entrypoint
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(description="Plota potência por CPU time (single vs mult)")
    parser.add_argument("--single", required=True, help="CSV do experimento single core")
    parser.add_argument("--mult",   required=True, help="CSV do experimento multi core")
    parser.add_argument("--out",    default="plots", help="Diretório de saída (default: plots/)")
    parser.add_argument("--scale",  default="linear", choices=["linear", "log"],
                        help="Escala do eixo Y: linear ou log (default: linear)")
    parser.add_argument("--benchmark", default=None,
                        help="Gerar apenas um benchmark específico (opcional)")
    args = parser.parse_args()

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    print("Carregando dados...")
    df_single = load(args.single, "single")
    df_mult   = load(args.mult,   "mult")
    df = pd.concat([df_single, df_mult], ignore_index=True)

    summary = compute_summary(df)

    mpl_scale = "log" if args.scale == "log" else "linear"

    if args.benchmark:
        plot_benchmark(summary, args.benchmark, out_dir, mpl_scale)
    else:
        plot_all(summary, out_dir, mpl_scale)

    print("\nConcluído.")


if __name__ == "__main__":
    main()

