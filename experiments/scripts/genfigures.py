#!/usr/bin/env python3
"""
QUBO Hybrid R-Flip Experiments - Figure Generator

Este script gera figuras a partir dos resultados dos experimentos.
Suporta dados completos e parciais, tratando graciosamente dados faltantes.
"""

import os
import json
import sys
from typing import List, Callable, Optional, Any
from pylatex import Document, Command, TikZ, Axis, Plot, NoEscape


#==============================================================================
# FUNÇÕES AUXILIARES
#==============================================================================

def percentage_change(baseline: float, value: float) -> float:
    """Calculate speedup ratio (baseline/value)."""
    return baseline / value if value != 0 else 0


def percentage_usage(basics: float, deltas: float) -> float:
    """Calculate percentage of delta usage."""
    total = basics + deltas
    return (deltas * 100) / total if total != 0 else 0


def groupby(func: Callable[[Any], Any], data: List[Any]):
    """Group data by a key function."""
    keys = sorted(set(map(func, data)))
    return ((key, [x for x in data if func(x) == key]) for key in keys)


def safe_get(entries: List[dict], eval_num: int, field: str) -> Optional[float]:
    """Safely get a value from entries, returning None if not found."""
    for x in entries:
        if x["params"]["eval"] == eval_num:
            return x.get("result", {}).get(field)
    return None


#==============================================================================
# GERAÇÃO DE FIGURAS
#==============================================================================

STRATEGY_NAMES = ["B", "RV", "S", "A", "C", "M", "AC", "AM", "CM", "ACM"]


def create_figure(filename: str, options: List[str], plots: List[Plot]):
    """Create a LaTeX figure with TikZ/PGFPlots and generate PDF/EPS."""
    if not plots:
        print(f"  Skipping {filename}: no valid plots")
        return
        
    doc = Document(documentclass="standalone", document_options="tikz,border=5pt")
    doc.append(Command("usepgfplotslibrary", "colorbrewer"))
    doc.append(Command("pgfplotsset", NoEscape("colormap/Paired-9")))
    
    with doc.create(TikZ()) as fig:
        fig_options = [
            "width=9cm",
            "height=5.5cm",
            "cycle list name=Paired-9",
            "legend style={font=\\footnotesize}",
            *options,
        ]
        with fig.create(Axis(NoEscape(",".join(fig_options)))) as axis:
            for plot in plots:
                if plot is not None:
                    axis.append(plot)
    
    doc.generate_tex(filename)
    
    # Gerar PDF
    ret = os.system(f"pdflatex -interaction=nonstopmode {filename}.tex > /dev/null 2>&1")
    if ret == 0:
        print(f"  ✓ {filename}.pdf")
    
    # Gerar EPS
    os.system(f"latex {filename}.tex > /dev/null 2>&1 && "
              f"dvips {filename}.dvi -o {filename}.eps > /dev/null 2>&1")


def make_speedup_plot(name: str, data: List[dict], n: int, 
                      strategy_num: int, field: str, 
                      r_param: str = "r", options: str = None) -> Optional[Plot]:
    """Create a speedup plot comparing strategy vs basic."""
    coords = []
    for r, entries in groupby(lambda x: x["params"][r_param], data):
        entries = list(entries)
        basic = safe_get(entries, 0, field)
        value = safe_get(entries, strategy_num, field)
        if basic is not None and value is not None:
            coords.append((r * 100 // n, percentage_change(basic, value)))
    
    if not coords:
        return None
    
    if options:
        return Plot(name=Command("texttt", name), options=options, coordinates=coords)
    return Plot(name=Command("texttt", name), coordinates=coords)


def make_usage_plot(name: str, data: List[dict], n: int,
                    strategy_num: int, r_param: str = "r") -> Optional[Plot]:
    """Create a usage percentage plot."""
    coords = []
    for r, entries in groupby(lambda x: x["params"][r_param], data):
        entries = list(entries)
        basics = safe_get(entries, strategy_num, "basics")
        deltas = safe_get(entries, strategy_num, "deltas")
        if basics is not None and deltas is not None:
            coords.append((r * 100 // n, percentage_usage(basics, deltas)))
    
    if not coords:
        return None
    
    return Plot(name=Command("texttt", name), coordinates=coords)


#==============================================================================
# GERADORES DE FIGURAS POR TIPO DE EXPERIMENTO
#==============================================================================

def generate_eval_figures(data: List[dict]):
    """Generate evaluation experiment figures."""
    eval_data = [x for x in data if x["params"]["exp"] == "eval"]
    if not eval_data:
        print("  No eval data found")
        return
    
    for (instance, n), inst_data in groupby(
        lambda x: (x["params"]["instance"], x["params"]["n"]), eval_data
    ):
        inst_data = list(inst_data)
        for n1, n1_data in groupby(lambda x: x["params"]["n1"], inst_data):
            n1_data = list(n1_data)
            
            plots = []
            # Strategies 2-9 (S, A, C, M, AC, AM, CM, ACM)
            for i, name in enumerate(STRATEGY_NAMES[2:], start=2):
                plot = make_speedup_plot(name, n1_data, n, i, "avg")
                if plot:
                    plots.append(plot)
            
            # RV reference line
            rv_plot = make_speedup_plot("RV", n1_data, n, 1, "avg",
                                        options="mark=none,dash dot,thick,color=gray")
            if rv_plot:
                plots.append(rv_plot)
            
            # Baseline reference
            plots.append(Plot(options="mark=none,dashed,thick,color=red",
                            coordinates=[(0, 1), (100, 1)]))
            
            if plots:
                create_figure(
                    f"eval-{instance}-{n1}".replace(".", "-"),
                    [
                        "xlabel={$r$ (\\% of $n$)}",
                        "ylabel={speedup}",
                        "xmin=1", "xmax=99",
                        "legend cell align={left}",
                        "legend columns=2",
                        "legend pos={north east}"
                    ],
                    plots
                )


def generate_ls_figures(data: List[dict]):
    """Generate local search figures."""
    ls_data = [x for x in data if x["params"]["exp"] == "ls"]
    if not ls_data:
        print("  No ls data found")
        return
    
    for (instance, n), inst_data in groupby(
        lambda x: (x["params"]["instance"], x["params"]["n"]), ls_data
    ):
        inst_data = list(inst_data)
        
        plots = []
        for i, name in enumerate(STRATEGY_NAMES[2:], start=2):
            plot = make_speedup_plot(name, inst_data, n, i, "dt")
            if plot:
                plots.append(plot)
        
        rv_plot = make_speedup_plot("RV", inst_data, n, 1, "dt",
                                    options="mark=none,dash dot,thick,color=gray")
        if rv_plot:
            plots.append(rv_plot)
        
        plots.append(Plot(options="mark=none,dashed,thick,color=red",
                         coordinates=[(0, 1), (100, 1)]))
        
        if plots:
            create_figure(
                f"ls-{instance}".replace(".", "-"),
                [
                    "xlabel={$r$ (\\% of $n$)}",
                    "ylabel={speedup}",
                    "xmin=1", "xmax=99",
                    "legend cell align={left}",
                    "legend columns=3",
                    "legend pos={north east}",
                ],
                plots
            )


def generate_ls_count_figures(data: List[dict]):
    """Generate local search count figures."""
    ls_count_data = [x for x in data if x["params"]["exp"] == "ls_count"]
    if not ls_count_data:
        print("  No ls_count data found")
        return
    
    for (instance, n), inst_data in groupby(
        lambda x: (x["params"]["instance"], x["params"]["n"]), ls_count_data
    ):
        inst_data = list(inst_data)
        
        plots = []
        for i, name in enumerate(STRATEGY_NAMES[2:], start=2):
            plot = make_usage_plot(name, inst_data, n, i)
            if plot:
                plots.append(plot)
        
        if plots:
            create_figure(
                f"ls-count-{instance}".replace(".", "-"),
                [
                    "xlabel={$r$ (\\% of $n$)}",
                    "ylabel={\\% \\textit{r-flip-rv} usage}",
                    "xmin=1", "xmax=99",
                    "ytick={0, 25, 50, 75, 100}",
                    "legend cell align={left}",
                    "legend columns=2",
                    "legend pos={north east}",
                ],
                plots
            )


def generate_vns_figures(data: List[dict]):
    """Generate VNS figures."""
    vns_data = [x for x in data if x["params"]["exp"] == "vns_figures"]
    if not vns_data:
        print("  No vns_figures data found")
        return
    
    for (instance, n), inst_data in groupby(
        lambda x: (x["params"]["instance"], x["params"]["n"]), vns_data
    ):
        inst_data = list(inst_data)
        
        plots = []
        for i, name in enumerate(STRATEGY_NAMES[2:], start=2):
            plot = make_speedup_plot(name, inst_data, n, i, "dt", r_param="r_max")
            if plot:
                plots.append(plot)
        
        rv_plot = make_speedup_plot("RV", inst_data, n, 1, "dt", r_param="r_max",
                                    options="mark=none,dash dot,thick,color=gray")
        if rv_plot:
            plots.append(rv_plot)
        
        plots.append(Plot(options="mark=none,dashed,thick,color=red",
                         coordinates=[(0, 1), (100, 1)]))
        
        if plots:
            create_figure(
                f"vns-{instance}".replace(".", "-"),
                [
                    "xlabel={$r$ (\\% of $n$)}",
                    "ylabel={speedup}",
                    "xmin=1", "xmax=99",
                    "legend cell align={left}",
                    "legend columns=3",
                    "legend pos={north east}",
                ],
                plots
            )


def generate_vns_count_figures(data: List[dict]):
    """Generate VNS count figures."""
    vns_count_data = [x for x in data if x["params"]["exp"] == "vns_figures_count"]
    if not vns_count_data:
        print("  No vns_figures_count data found")
        return
    
    for (instance, n), inst_data in groupby(
        lambda x: (x["params"]["instance"], x["params"]["n"]), vns_count_data
    ):
        inst_data = list(inst_data)
        
        plots = []
        for i, name in enumerate(STRATEGY_NAMES[2:], start=2):
            plot = make_usage_plot(name, inst_data, n, i, r_param="r_max")
            if plot:
                plots.append(plot)
        
        if plots:
            create_figure(
                f"vns-count-{instance}".replace(".", "-"),
                [
                    "xlabel={$r$ (\\% of $n$)}",
                    "ylabel={\\% \\textit{r-flip-rv} usage}",
                    "xmin=1", "xmax=99",
                    "ymin=45", "ymax=105",
                    "ytick={0, 25, 50, 75, 100}",
                    "legend cell align={left}",
                    "legend columns=2",
                    "legend pos={south west}",
                ],
                plots
            )


#==============================================================================
# MAIN
#==============================================================================

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <results.json> [output_dir]")
        sys.exit(1)
    
    # Carregar dados
    with open(sys.argv[1]) as fp:
        data = json.load(fp)
    
    print(f"Loaded {len(data)} results from {sys.argv[1]}")
    
    # Mudar para diretório de output
    if len(sys.argv) > 2:
        output_dir = sys.argv[2]
        os.makedirs(output_dir, exist_ok=True)
        os.chdir(output_dir)
        print(f"Output directory: {output_dir}")
    
    # Gerar figuras para cada tipo de experimento
    print("\nGenerating eval figures...")
    generate_eval_figures(data)
    
    print("\nGenerating ls figures...")
    generate_ls_figures(data)
    
    print("\nGenerating ls_count figures...")
    generate_ls_count_figures(data)
    
    print("\nGenerating vns figures...")
    generate_vns_figures(data)
    
    print("\nGenerating vns_count figures...")
    generate_vns_count_figures(data)
    
    # Limpar arquivos temporários
    os.system("rm -f *.aux *.dvi *.log 2>/dev/null")
    
    print("\nDone!")


if __name__ == "__main__":
    main()
