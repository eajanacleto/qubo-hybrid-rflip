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
│   ├── main.cpp          # Main entry point
│   ├── json.hpp          # nlohmann/json (auto-downloaded)
│   │
│   ├── core/             # Core data structures
│   │   ├── ubqp.hpp      # UBQP problem definition
│   │   ├── evaluation.hpp # Evaluation strategy enum
│   │   ├── solution.hpp  # Solution data structures
│   │   └── utils.hpp     # Utility functions
│   │
│   ├── algorithms/       # Algorithm implementations
│   │   ├── evaluate.hpp  # Main include for evaluation
│   │   ├── evaluate/     # Evaluation algorithms
│   │   │   ├── basic.hpp     # O(n²) basic evaluation
│   │   │   ├── rfliprv.hpp   # O(r²) r-flip-rv evaluation
│   │   │   ├── hybrid.hpp    # Hybrid strategy selection
│   │   │   └── verify.hpp    # Debug verification functions
│   │   ├── rv/           # Reevaluation vector algorithms
│   │   │   ├── build.hpp     # Build RV from scratch O(n²)
│   │   │   ├── update.hpp    # Incremental RV update O(nr)
│   │   │   └── hybrid.hpp    # Hybrid RV update strategy
│   │   ├── neighbor/     # Neighbor solution operations
│   │   │   ├── generate.hpp  # Random neighbor generation
│   │   │   └── replace.hpp   # Incumbent replacement
│   │   ├── local_search.hpp  # Local Search algorithm
│   │   └── vns.hpp           # Variable Neighborhood Search
│   │
│   └── experiments/      # Experiment definitions
│       └── experiments.hpp # Experiment functions
│
├── scripts/              # Python scripts
│   ├── genfigures.py     # Generate LaTeX figures
│   ├── gentables.py      # Generate LaTeX tables
│   ├── run_experiments.sh # Run all experiments + figures
│   ├── run_quick_test.sh # Quick validation test
│   └── convert_eps_to_pdf.sh
│
├── data/                 # Input data
│   └── instances/        # Problem instances (auto-downloaded)
│       ├── bqp*.txt      # OR-Library BQP instances
│       └── G*            # Gset MaxCut instances
│
├── output/               # Generated output
│   ├── results/          # Experiment results (JSON)
│   └── figures/          # LaTeX figures (.tex, .eps, .pdf)
│
└── docs/                 # Documentation
    └── analysis/         # Analysis reports
```

## Module Overview

### Core (`src/core/`)
- **ubqp.hpp**: UBQP problem structure with instance loaders for BQP and MaxCut formats
- **evaluation.hpp**: Enumeration of 10 evaluation strategies (basic, rflip_rv, s, a, c, m, ac, am, cm, acm)
- **solution.hpp**: Template-based incumbent and neighbor solution structures
- **utils.hpp**: Timing utilities for performance measurement

### Algorithms (`src/algorithms/`)

#### Evaluation (`src/algorithms/evaluate/`)
- **basic.hpp**: O(n²) basic evaluation - computes objective from scratch
- **rfliprv.hpp**: O(r²) r-flip-rv evaluation - uses reevaluation vector
- **hybrid.hpp**: Hybrid strategy selection between basic and r-flip-rv
- **verify.hpp**: Debug verification functions for correctness checking

#### Reevaluation Vector (`src/algorithms/rv/`)
- **build.hpp**: Build reevaluation vector from scratch O(n²)
- **update.hpp**: Incremental reevaluation vector update O(nr)
- **hybrid.hpp**: Hybrid RV update strategy selection

#### Neighbor Operations (`src/algorithms/neighbor/`)
- **generate.hpp**: Random r-flip neighbor generation
- **replace.hpp**: Replace incumbent with neighbor solution

#### Metaheuristics
- **local_search.hpp**: Stochastic local search with r-flip neighborhood
- **vns.hpp**: Variable Neighborhood Search metaheuristic

### Experiments (`src/experiments/`)
- **experiments.hpp**: Experiment functions for benchmarking evaluation strategies

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

# 3. Run all experiments and generate figures (recommended)
./scripts/run_experiments.sh

# Or run quick test (30s timeout per experiment)
./scripts/run_quick_test.sh

# Or run individual experiments
make run EXP=ls              # Local Search
make run EXP=vns_figures     # VNS for figures
make run EXP=eval            # Evaluation

# 4. Generate figures from results
make figures RESULTS_FILE=output/results/results_complete.json
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
| 0 | B (Basic) | Baseline O(n²) evaluation |
| 1 | RV | r-flip with reevaluation vector O(r²) |
| 2 | S | Hybrid strategy S (size-based) |
| 3 | A | Hybrid strategy A (additions-based) |
| 4 | C | Hybrid strategy C (cache-based) |
| 5 | M | Hybrid strategy M (multiplications-based) |
| 6 | AC | Combined A+C |
| 7 | AM | Combined A+M |
| 8 | CM | Combined C+M |
| 9 | ACM | Combined A+C+M |

## Manual Commands

```bash
# Run experiment directly
./main.out output/results/results.json ls

# Generate figures manually
source .venv/bin/activate
python scripts/genfigures.py output/results/results.json output/figures/

# Convert EPS to PDF
cd output/figures
../../scripts/convert_eps_to_pdf.sh
```

## Experiment Runner Scripts

### run_experiments.sh
Full experiment runner with configurable parameters. Edit the configuration section at the top of the script to customize:

```bash
# Key configuration options in scripts/run_experiments.sh:
TIMEOUT_EVAL=0          # Timeout for eval experiment (0=no timeout)
TIMEOUT_LS=0            # Timeout for ls experiment
TIMEOUT_VNS=0           # Timeout for vns experiment
RUN_EVAL=1              # Enable/disable experiments (1=yes, 0=no)
RUN_LS=1
RUN_VNS=1
RUN_TABLES=0            # vns_tables (disabled by default - very long)
GENERATE_FIGURES=1      # Generate figures after experiments
```

### run_quick_test.sh
Quick validation script with 30-second timeouts. Use this to verify the pipeline works:

```bash
./scripts/run_quick_test.sh
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
