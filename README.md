# QUBO Hybrid r-Flip Experiments

Computational experiments for UBQP (Unconstrained Binary Quadratic Programming) optimization using hybrid evaluation strategies with r-flip moves.

## Project Structure

```
qubo-hybrid-rflip-experiments/
├── Makefile              # Build and automation
├── README.md             # This file
├── .gitignore
│
├── src/                  # Source code
│   └── main.cpp          # Main C++ implementation
│
├── include/              # Header files
│   └── json.hpp          # nlohmann/json (auto-downloaded)
│
├── scripts/              # Python scripts
│   ├── genfigures.py     # Generate LaTeX figures
│   ├── gentables.py      # Generate LaTeX tables
│   └── convert_eps_to_pdf.sh
│
├── data/                 # Input data
│   └── instances/        # Problem instances (auto-downloaded)
│       ├── bqp*.txt      # OR-Library BQP instances
│       └── G*            # Gset MaxCut instances
│
├── results/              # Experiment results (JSON)
│
├── output/               # Generated output
│   └── figures/          # LaTeX figures (.tex, .eps, .pdf)
│
└── docs/                 # Documentation
    └── analysis/         # Analysis reports
```

## Prerequisites

- **Build:** `g++` (C++17), `make`, `curl`
- **Figures:** Python 3, LaTeX (`texlive-latex-base`, `texlive-pictures`, `texlive-latex-extra`, `texlive-science`)

```bash
# Ubuntu/Debian
sudo apt install g++ make curl python3-venv texlive-latex-base texlive-pictures texlive-latex-extra texlive-science
```

## Quick Start

```bash
# 1. Setup (download dependencies + create Python venv)
make setup

# 2. Build
make

# 3. Run experiments
make run EXP=ls              # Local Search
make run EXP=vns_figures     # VNS for figures
make run EXP=eval            # Evaluation

# 4. Generate figures
make figures RESULTS_FILE=results.json
```

## Available Experiments

| Experiment | Command | Description |
|------------|---------|-------------|
| `eval` | `make run EXP=eval` | Evaluation algorithm comparison |
| `ls` | `make run EXP=ls` | Local Search experiments |
| `ls_count` | `make run EXP=ls_count` | LS with operation counting |
| `vns_figures` | `make run EXP=vns_figures` | VNS experiments for figures |
| `vns_figures_count` | `make run EXP=vns_figures_count` | VNS with counting |
| `vns_tables` | `make run EXP=vns_tables` | VNS experiments for tables |

## Evaluation Strategies

| ID | Name | Description |
|----|------|-------------|
| 0 | B (Basic) | Baseline evaluation |
| 1 | RV | r-flip with reevaluation vector |
| 2 | S | Strategy S |
| 3 | A | Strategy A |
| 4 | C | Strategy C |
| 5 | M | Strategy M |
| 6 | AC | Combined A+C |
| 7 | AM | Combined A+M |
| 8 | CM | Combined C+M |
| 9 | ACM | Combined A+C+M |

## Manual Commands

```bash
# Run experiment directly
./main.out results/results.json ls

# Generate figures manually
source .venv/bin/activate
cd output/figures
python ../../scripts/genfigures.py ../../results/results.json

# Convert EPS to PDF
cd output/figures
../../scripts/convert_eps_to_pdf.sh
```

## Makefile Targets

```bash
make              # Build project
make setup        # Setup dependencies + Python venv
make run EXP=...  # Run experiment
make figures ...  # Generate figures
make clean        # Remove build artifacts
make distclean    # Remove all generated files
make help         # Show help
```

## License

See project license for details.
