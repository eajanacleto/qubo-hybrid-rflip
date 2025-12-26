# =============================================================================
# QUBO Hybrid r-Flip Experiments
# =============================================================================

# Directories
SRC_DIR     := src
INCLUDE_DIR := include
DATA_DIR    := data
OUTPUT_DIR  := output
FIGURES_DIR := $(OUTPUT_DIR)/figures
RESULTS_DIR := $(OUTPUT_DIR)/results
SCRIPTS_DIR := scripts

# Compiler settings
CXX      := g++
CXXFLAGS := -DNDEBUG -std=c++17 -Wall -Wextra -Ofast -march=native -pthread
LDFLAGS  := -s

# Files
TARGET   := main.out
SRC      := $(SRC_DIR)/main.cpp
JSON_HPP := $(INCLUDE_DIR)/json.hpp

# =============================================================================
# Main targets
# =============================================================================

.PHONY: all clean run setup help figures

all: $(TARGET)

$(TARGET): $(SRC) $(JSON_HPP) Makefile | $(DATA_DIR)/instances
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(SRC) $(LDFLAGS) -o $(TARGET)

# =============================================================================
# Dependencies
# =============================================================================

$(JSON_HPP):
	@mkdir -p $(INCLUDE_DIR)
	@echo "Downloading json.hpp..."
	curl -fLo $(JSON_HPP) https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

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

setup: $(JSON_HPP) $(DATA_DIR)/instances
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
# Utility targets
# =============================================================================

clean:
	rm -f $(TARGET)
	rm -f $(FIGURES_DIR)/*.aux $(FIGURES_DIR)/*.dvi $(FIGURES_DIR)/*.log

distclean: clean
	rm -rf $(INCLUDE_DIR)/json.hpp
	rm -rf $(DATA_DIR)/instances
	rm -rf $(OUTPUT_DIR)
	rm -rf .venv

help:
	@echo "QUBO Hybrid r-Flip Experiments"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the project"
	@echo "  make setup        - Download dependencies and setup Python environment"
	@echo "  make run EXP=ls   - Run experiment (ls, ls_count, vns_figures, vns_tables, eval)"
	@echo "  make figures RESULTS_FILE=results.json - Generate figures from results"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Remove all generated files (including dependencies)"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make setup"
	@echo "  make"
	@echo "  make run EXP=ls"
	@echo "  make figures RESULTS_FILE=results.json"
