# =============================================================================
# QUBO Hybrid r-Flip Experiments
# =============================================================================

# Directories
SRC_DIR     := src
DATA_DIR    := data
OUTPUT_DIR  := output
EXP_DIR     := experiments
FIGURES_DIR := $(OUTPUT_DIR)/figures
RESULTS_DIR := $(OUTPUT_DIR)/results
SCRIPTS_DIR := $(EXP_DIR)/scripts
CONFIG_DIR  := $(EXP_DIR)/config

# Compiler settings
CXX      := g++
CXXFLAGS := -DNDEBUG -std=c++17 -Wall -Wextra -Ofast -march=native -pthread
LDFLAGS  := -s

# Files
TARGET   := main.out
SRC      := $(SRC_DIR)/main.cpp

# Header files (for dependency tracking)
CORE_HEADERS := $(wildcard $(SRC_DIR)/core/*.hpp)
ALGO_HEADERS := $(wildcard $(SRC_DIR)/algorithms/*.hpp) \
                $(wildcard $(SRC_DIR)/algorithms/*/*.hpp)
EXP_HEADERS  := $(wildcard $(SRC_DIR)/experiments/*.hpp)
ALL_HEADERS  := $(CORE_HEADERS) $(ALGO_HEADERS) $(EXP_HEADERS) $(SRC_DIR)/json.hpp

# =============================================================================
# Main targets
# =============================================================================

.PHONY: all clean run setup help figures

all: $(TARGET)

$(TARGET): $(SRC) $(ALL_HEADERS) Makefile | $(DATA_DIR)/instances
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(SRC) $(LDFLAGS) -o $(TARGET)

# =============================================================================
# Dependencies
# =============================================================================

$(SRC_DIR)/json.hpp:
	@echo "Downloading json.hpp..."
	curl -fLo $(SRC_DIR)/json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

$(DATA_DIR)/instances:
	@mkdir -p $(DATA_DIR)/instances
	@echo "Downloading BQP instances..."
	curl -fL 'http://people.brunel.ac.uk/~mastjjb/jeb/orlib/files/bqp{50,100,250,500,1000}.txt' -o '$(DATA_DIR)/instances/bqp#1.txt'
	curl -fL 'http://people.brunel.ac.uk/~mastjjb/jeb/orlib/files/bqp2500.gz' | gzip -c -d > $(DATA_DIR)/instances/bqp2500.txt
	@echo "Downloading Gset instances..."
	curl -fL 'https://web.stanford.edu/~yyye/yyye/Gset/G[1-54]' -o '$(DATA_DIR)/instances/G#1'

# =============================================================================
# Setup & Run
# =============================================================================

setup: $(SRC_DIR)/json.hpp $(DATA_DIR)/instances
	@echo "Setting up Python virtual environment..."
	@test -d .venv || python3 -m venv .venv
	@. .venv/bin/activate && pip install -q pylatex
	@echo "Setup complete!"

run: $(TARGET)
	@mkdir -p $(RESULTS_DIR)
	./$(TARGET) $(RESULTS_DIR)/results.json $(EXP)

# =============================================================================
# Figures generation
# =============================================================================

figures:
	@mkdir -p $(FIGURES_DIR)
	@. .venv/bin/activate && cd $(FIGURES_DIR) && python ../../$(SCRIPTS_DIR)/genfigures.py ../results/$(RESULTS_FILE)

# =============================================================================
# Experiment scripts (convenience targets)
# =============================================================================

.PHONY: test-quick test-full experiments

test-quick: $(TARGET)
	@bash $(SCRIPTS_DIR)/run_quick_test.sh

test-full: $(TARGET)
	@bash $(SCRIPTS_DIR)/run_experiments.sh

experiments: test-full

# =============================================================================
# Utility targets
# =============================================================================

clean:
	rm -f $(TARGET)
	rm -f $(FIGURES_DIR)/*.aux $(FIGURES_DIR)/*.dvi $(FIGURES_DIR)/*.log

distclean: clean
	rm -rf $(SRC_DIR)/json.hpp
	rm -rf $(DATA_DIR)/instances
	rm -rf $(OUTPUT_DIR)
	rm -rf .venv

help:
	@echo "QUBO Hybrid r-Flip Experiments"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the project"
	@echo "  make setup        - Download dependencies and setup Python environment"
	@echo "  make run EXP=type - Run experiment directly"
	@echo "                      Types: eval, ls, ls_count, vns_figures, vns_tables"
	@echo "  make test-quick   - Run quick validation test (~2 min)"
	@echo "  make test-full    - Run all experiments (may take hours)"
	@echo "  make figures      - Generate figures from results"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Remove all generated files"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Project structure:"
	@echo "  src/              - Source code (modular C++ headers)"
	@echo "  experiments/      - Experiment configuration and scripts"
	@echo "    config/         - JSON configuration files"
	@echo "    scripts/        - Python and Bash scripts"
	@echo "  output/           - Generated results and figures"
	@echo "  data/             - Benchmark instances"
	@echo ""
	@echo "Examples:"
	@echo "  make setup && make && make test-quick"
