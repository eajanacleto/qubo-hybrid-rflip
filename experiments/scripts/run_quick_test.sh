#!/bin/bash
#==============================================================================
# QUBO Hybrid R-Flip - Quick Test Runner
#==============================================================================
# Script de teste rápido para validar a pipeline completa.
# Executa versões reduzidas dos experimentos com timeout.
#==============================================================================

set -e

#==============================================================================
# CONFIGURAÇÕES DE TESTE
#==============================================================================

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${PROJECT_DIR}/output"
RESULTS_DIR="${OUTPUT_DIR}/test_results"
FIGURES_DIR="${OUTPUT_DIR}/test_figures"

EXECUTABLE="${PROJECT_DIR}/main.out"
GENFIGURES_SCRIPT="${PROJECT_DIR}/experiments/scripts/genfigures.py"

# Timeouts curtos para teste (em segundos)
TEST_TIMEOUT=30
TABLES_TIMEOUT=20

#==============================================================================
# CORES E FUNÇÕES
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[OK]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error()   { echo -e "${RED}[ERRO]${NC} $1"; }
log_section() { echo -e "\n${CYAN}=== $1 ===${NC}"; }

#==============================================================================
# INÍCIO
#==============================================================================

echo -e "${CYAN}"
echo "=============================================="
echo "  QUBO Hybrid R-Flip - Quick Test"
echo "=============================================="
echo -e "${NC}"

#------------------------------------------------------------------------------
# Preparação
#------------------------------------------------------------------------------
log_section "Preparação"

mkdir -p "$RESULTS_DIR"
mkdir -p "$FIGURES_DIR"

# Limpar resultados anteriores
rm -f "$RESULTS_DIR"/*.json
rm -f "$FIGURES_DIR"/*

log_success "Diretórios preparados"

#------------------------------------------------------------------------------
# Compilação
#------------------------------------------------------------------------------
log_section "Compilação"

cd "$PROJECT_DIR"
make clean && make
log_success "Compilado com sucesso"

#------------------------------------------------------------------------------
# Teste: eval
#------------------------------------------------------------------------------
log_section "Teste: eval"

timeout $TEST_TIMEOUT "$EXECUTABLE" "$RESULTS_DIR/test_eval.json" "eval" 2>&1 | tail -5 || true

if [ -f "$RESULTS_DIR/test_eval.json" ]; then
    LINES=$(wc -l < "$RESULTS_DIR/test_eval.json")
    log_success "eval: ${LINES} linhas geradas"
else
    log_warning "eval: arquivo não gerado (timeout?)"
fi

#------------------------------------------------------------------------------
# Teste: ls
#------------------------------------------------------------------------------
log_section "Teste: ls"

timeout $TEST_TIMEOUT "$EXECUTABLE" "$RESULTS_DIR/test_ls.json" "ls" 2>&1 | tail -5 || true

if [ -f "$RESULTS_DIR/test_ls.json" ]; then
    LINES=$(wc -l < "$RESULTS_DIR/test_ls.json")
    log_success "ls: ${LINES} linhas geradas"
else
    log_warning "ls: arquivo não gerado (timeout?)"
fi

#------------------------------------------------------------------------------
# Teste: ls_count
#------------------------------------------------------------------------------
log_section "Teste: ls_count"

timeout $TEST_TIMEOUT "$EXECUTABLE" "$RESULTS_DIR/test_ls_count.json" "ls_count" 2>&1 | tail -5 || true

if [ -f "$RESULTS_DIR/test_ls_count.json" ]; then
    LINES=$(wc -l < "$RESULTS_DIR/test_ls_count.json")
    log_success "ls_count: ${LINES} linhas geradas"
else
    log_warning "ls_count: arquivo não gerado (timeout?)"
fi

#------------------------------------------------------------------------------
# Teste: vns_figures
#------------------------------------------------------------------------------
log_section "Teste: vns_figures"

timeout $TEST_TIMEOUT "$EXECUTABLE" "$RESULTS_DIR/test_vns.json" "vns_figures" 2>&1 | tail -5 || true

if [ -f "$RESULTS_DIR/test_vns.json" ]; then
    LINES=$(wc -l < "$RESULTS_DIR/test_vns.json")
    log_success "vns_figures: ${LINES} linhas geradas"
else
    log_warning "vns_figures: arquivo não gerado (timeout?)"
fi

#------------------------------------------------------------------------------
# Teste: vns_figures_count
#------------------------------------------------------------------------------
log_section "Teste: vns_figures_count"

timeout $TEST_TIMEOUT "$EXECUTABLE" "$RESULTS_DIR/test_vns_count.json" "vns_figures_count" 2>&1 | tail -5 || true

if [ -f "$RESULTS_DIR/test_vns_count.json" ]; then
    LINES=$(wc -l < "$RESULTS_DIR/test_vns_count.json")
    log_success "vns_figures_count: ${LINES} linhas geradas"
else
    log_warning "vns_figures_count: arquivo não gerado (timeout?)"
fi

#------------------------------------------------------------------------------
# Teste: vns_tables
#------------------------------------------------------------------------------
log_section "Teste: vns_tables"

timeout $TABLES_TIMEOUT "$EXECUTABLE" "$RESULTS_DIR/test_tables.json" "vns_tables" 2>&1 | tail -5 || true

if [ -f "$RESULTS_DIR/test_tables.json" ]; then
    LINES=$(wc -l < "$RESULTS_DIR/test_tables.json")
    log_success "vns_tables: ${LINES} linhas geradas"
else
    log_warning "vns_tables: arquivo não gerado (timeout?)"
fi

#------------------------------------------------------------------------------
# Mesclar resultados
#------------------------------------------------------------------------------
log_section "Mesclando resultados"

python3 - "$RESULTS_DIR/test_complete.json" "$RESULTS_DIR"/test_*.json << 'EOF'
import json
import sys

output_file = sys.argv[1]
input_files = sys.argv[2:]

all_results = []
for f in input_files:
    try:
        with open(f) as fp:
            data = json.load(fp)
            if isinstance(data, list):
                all_results.extend(data)
    except Exception as e:
        pass

with open(output_file, 'w') as fp:
    json.dump(all_results, fp, indent=1)

print(f"Total: {len(all_results)} resultados mesclados")
EOF

#------------------------------------------------------------------------------
# Gerar figuras
#------------------------------------------------------------------------------
log_section "Gerando figuras"

if [ -d "$PROJECT_DIR/.venv" ]; then
    source "$PROJECT_DIR/.venv/bin/activate"
fi

cd "$PROJECT_DIR"
python3 "$GENFIGURES_SCRIPT" "$RESULTS_DIR/test_complete.json" "$FIGURES_DIR/" 2>&1 || {
    log_warning "Alguns gráficos podem não ter sido gerados (dados insuficientes)"
}

# Limpar arquivos temporários
rm -f "$FIGURES_DIR"/*.aux "$FIGURES_DIR"/*.dvi "$FIGURES_DIR"/*.log 2>/dev/null || true

#------------------------------------------------------------------------------
# Gerar tabelas
#------------------------------------------------------------------------------
log_section "Gerando tabelas"

GENTABLES_SCRIPT="${PROJECT_DIR}/experiments/scripts/gentables.py"
TABLES_DIR="${FIGURES_DIR}"

if [ -f "$GENTABLES_SCRIPT" ] && [ -f "$RESULTS_DIR/test_complete.json" ]; then
    python3 "$GENTABLES_SCRIPT" "$RESULTS_DIR/test_complete.json" > "$TABLES_DIR/tables_output.txt" 2>&1 || true
    
    if [ -s "$TABLES_DIR/tables_output.txt" ]; then
        TABLE_LINES=$(wc -l < "$TABLES_DIR/tables_output.txt")
        log_success "Tabelas geradas: ${TABLE_LINES} linhas em tables_output.txt"
        echo "Preview das tabelas:"
        head -5 "$TABLES_DIR/tables_output.txt" || true
    else
        log_warning "Nenhuma tabela gerada (dados insuficientes para vns_tables)"
    fi
else
    log_warning "Script de tabelas ou dados não disponíveis"
fi

#------------------------------------------------------------------------------
# Resumo
#------------------------------------------------------------------------------
log_section "Resumo"

echo ""
echo "Arquivos de resultados:"
ls -lh "$RESULTS_DIR"/*.json 2>/dev/null | head -10

echo ""
echo "Figuras geradas:"
PDF_COUNT=$(find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | wc -l)
EPS_COUNT=$(find "$FIGURES_DIR" -name "*.eps" 2>/dev/null | wc -l)
echo "  PDFs: $PDF_COUNT"
echo "  EPS:  $EPS_COUNT"

if [ $PDF_COUNT -gt 0 ]; then
    echo ""
    ls -lh "$FIGURES_DIR"/*.pdf 2>/dev/null | head -5
fi

echo ""
echo -e "${GREEN}=============================================="
echo "  Teste concluído!"
echo "=============================================="
echo -e "${NC}"
echo "Tempo total: $SECONDS segundos"
