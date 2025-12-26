# QUBO Hybrid r-Flip Experiments

Computational experiments for UBQP (Unconstrained Binary Quadratic Programming) optimization using hybrid evaluation strategies with r-flip moves.

## Project Structure

```
qubo-hybrid-rflip-experiments/
├── Makefile              # Build and automation
├── README.md             # This file
│
├── src/                  # Source code (modular C++ headers)
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
│   │   │   └── verify.hpp    # Debug verification
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
│   └── experiments/      # Experiment modules
│       ├── experiments.hpp    # Main include wrapper
│       ├── common.hpp         # ExperimentRunner class
│       ├── eval_experiment.hpp    # Evaluation experiments
│       ├── ls_experiment.hpp      # Local Search experiments
│       └── vns_experiment.hpp     # VNS experiments
│
├── experiments/          # Experiment configuration and scripts
│   ├── config/           # JSON configuration files
│   │   ├── instances.json    # Instance set definitions
│   │   └── experiments.json  # Experiment configurations
│   ├── scripts/          # Experiment scripts
│   │   ├── run_experiments.sh   # Full experiment runner
│   │   ├── run_quick_test.sh    # Quick validation test
│   │   ├── genfigures.py        # Generate LaTeX figures
│   │   ├── gentables.py         # Generate LaTeX tables
│   │   └── convert_eps_to_pdf.sh
│   ├── results/          # Experiment results (generated)
│   └── figures/          # Generated figures (generated)
│
├── data/                 # Input data
│   └── instances/        # Problem instances (auto-downloaded)
│       ├── bqp*.txt      # OR-Library BQP instances
│       └── G*            # Gset MaxCut instances
│
├── output/               # Generated output
│   ├── results/          # Experiment results (JSON)
│   ├── figures/          # LaTeX figures (.tex, .eps, .pdf)
│   ├── test_results/     # Quick test results
│   └── test_figures/     # Quick test figures
│
└── docs/                 # Documentation
    └── analysis/         # Analysis reports
```

## Module Overview

### Core (`src/core/`)
- **ubqp.hpp**: UBQP problem structure with instance loaders
- **evaluation.hpp**: Enumeration of 10 evaluation strategies
- **solution.hpp**: Template-based incumbent and neighbor solutions
- **utils.hpp**: Timing utilities for performance measurement

### Algorithms (`src/algorithms/`)

#### Evaluation (`src/algorithms/evaluate/`)
- **basic.hpp**: O(n²) basic evaluation
- **rfliprv.hpp**: O(r²) r-flip-rv evaluation
- **hybrid.hpp**: Hybrid strategy selection

#### Reevaluation Vector (`src/algorithms/rv/`)
- **build.hpp**: Build RV from scratch O(n²)
- **update.hpp**: Incremental RV update O(nr)
- **hybrid.hpp**: Hybrid RV update strategy

#### Metaheuristics
- **local_search.hpp**: Stochastic local search with r-flip neighborhood
- **vns.hpp**: Variable Neighborhood Search metaheuristic

### Experiments (`src/experiments/`)
- **common.hpp**: `ExperimentRunner` class with caching and I/O
- **eval_experiment.hpp**: Evaluation time experiments
- **ls_experiment.hpp**: Local Search experiments
- **vns_experiment.hpp**: VNS experiments

## Prerequisites

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

# 3. Run quick validation test (~2 min)
make test-quick

# 4. Run all experiments + generate figures (may take hours)
make test-full
```

## Running Experiments

### Option 1: Using Makefile (recommended)

```bash
# Quick test with short timeouts
make test-quick

# Full experiments (all experiments + figures)
make test-full

# Run individual experiment
make run EXP=ls
make run EXP=vns_figures
make run EXP=eval
```

### Option 2: Using Scripts Directly

```bash
# Quick validation test
./experiments/scripts/run_quick_test.sh

# Full experiment runner (configurable)
./experiments/scripts/run_experiments.sh
```

### Option 3: Direct Execution

```bash
./main.out output/results/results.json ls
./main.out output/results/results.json vns_figures
./main.out output/results/results.json eval
```

## Available Experiments

| Experiment | Description |
|------------|-------------|
| `eval` | Evaluation algorithm comparison |
| `ls` | Local Search time measurement |
| `ls_count` | Local Search with operation counting |
| `vns_figures` | VNS experiments for figures |
| `vns_figures_count` | VNS with operation counting |
| `vns_tables` | VNS benchmark experiments |

## Evaluation Strategies

| ID | Name | Description |
|----|------|-------------|
| 0 | B | Baseline O(n²) evaluation |
| 1 | RV | r-flip with reevaluation vector O(r²) |
| 2 | S | Hybrid strategy S (size-based) |
| 3 | A | Hybrid strategy A (additions-based) |
| 4 | C | Hybrid strategy C (cache-based) |
| 5 | M | Hybrid strategy M (multiplications-based) |
| 6 | AC | Combined A+C |
| 7 | AM | Combined A+M |
| 8 | CM | Combined C+M |
| 9 | ACM | Combined A+C+M |

## Configuration

Experiment configurations are stored in `experiments/config/`:

- **instances.json**: Define instance sets for different experiments
- **experiments.json**: Configure experiment parameters

Edit `experiments/scripts/run_experiments.sh` for timeouts and experiment selection.

## Makefile Targets

```bash
make              # Build project
make setup        # Setup dependencies + Python venv
make test-quick   # Run quick validation (~2 min)
make test-full    # Run all experiments
make run EXP=...  # Run specific experiment
make figures      # Generate figures from results
make clean        # Remove build artifacts
make distclean    # Remove all generated files
make help         # Show help
```

## License

See project license for details.
