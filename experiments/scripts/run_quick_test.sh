#!/bin/bash
#==============================================================================
# QUBO Hybrid R-Flip - Quick Test Runner
#==============================================================================
# Script de teste rápido para validar a pipeline completa.
# Usa o conjunto de instâncias "quick_test" definido em instances.json:
#   - bqp50.1, bqp100.1, G1
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
GENTABLES_SCRIPT="${PROJECT_DIR}/experiments/scripts/gentables.py"

# Timeout para teste (em segundos)
TEST_TIMEOUT=120

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

# Mostrar instâncias do quick_test
log_info "Instâncias usadas (de instances.json -> quick_test):"
python3 -c "
import json
with open('$PROJECT_DIR/experiments/config/instances.json') as f:
    data = json.load(f)
    instances = data['instance_sets']['quick_test']['instances']
    print('  ' + ', '.join(instances))
"

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
# Executar quick_test
#------------------------------------------------------------------------------
log_section "Executando quick_test"

log_info "Este experimento usa instâncias do conjunto 'quick_test' e executa:"
log_info "  - eval (avaliação de tempo)"
log_info "  - ls_all (local search com todas as estratégias)"
log_info "  - ls_count_all (contagem de avaliações)"
log_info "  - vns_tables_all (VNS com todas as estratégias de LS)"

START_TIME=$(date +%s)

timeout $TEST_TIMEOUT "$EXECUTABLE" "$RESULTS_DIR/test_quick.json" "quick_test" 2>&1 || {
    log_warning "Experimento interrompido por timeout (${TEST_TIMEOUT}s)"
}

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

if [ -f "$RESULTS_DIR/test_quick.json" ]; then
    LINES=$(wc -l < "$RESULTS_DIR/test_quick.json")
    RESULTS=$(grep -c '"result"' "$RESULTS_DIR/test_quick.json" 2>/dev/null || echo 0)
    log_success "quick_test: ${RESULTS} resultados gerados em ${ELAPSED}s"
    
    # Mostrar estatísticas por tipo de experimento
    echo ""
    log_info "Resultados por experimento:"
    python3 -c "
import json
with open('$RESULTS_DIR/test_quick.json') as f:
    data = json.load(f)
    
# Count by experiment type
by_exp = {}
for item in data:
    exp = item.get('params', {}).get('exp', 'unknown')
    by_exp[exp] = by_exp.get(exp, 0) + 1

for exp, count in sorted(by_exp.items()):
    print(f'  {exp}: {count} resultados')
" 2>/dev/null || log_warning "Não foi possível analisar resultados"
else
    log_warning "Arquivo de resultados não gerado"
fi

#------------------------------------------------------------------------------
# Gerar figuras (se houver dados)
#------------------------------------------------------------------------------
log_section "Gerando figuras"

if [ -d "$PROJECT_DIR/.venv" ]; then
    source "$PROJECT_DIR/.venv/bin/activate"
fi

cd "$PROJECT_DIR"
if [ -f "$RESULTS_DIR/test_quick.json" ] && [ -f "$GENFIGURES_SCRIPT" ]; then
    python3 "$GENFIGURES_SCRIPT" "$RESULTS_DIR/test_quick.json" "$FIGURES_DIR/" 2>&1 || {
        log_warning "Alguns gráficos podem não ter sido gerados (dados insuficientes)"
    }
    
    # Limpar arquivos temporários
    rm -f "$FIGURES_DIR"/*.aux "$FIGURES_DIR"/*.dvi "$FIGURES_DIR"/*.log 2>/dev/null || true
else
    log_warning "Script de figuras ou dados não disponíveis"
fi

#------------------------------------------------------------------------------
# Gerar tabelas
#------------------------------------------------------------------------------
log_section "Gerando tabelas"

if [ -f "$GENTABLES_SCRIPT" ] && [ -f "$RESULTS_DIR/test_quick.json" ]; then
    python3 "$GENTABLES_SCRIPT" "$RESULTS_DIR/test_quick.json" > "$FIGURES_DIR/tables_output.txt" 2>&1 || true
    
    if [ -s "$FIGURES_DIR/tables_output.txt" ]; then
        TABLE_LINES=$(wc -l < "$FIGURES_DIR/tables_output.txt")
        log_success "Tabelas geradas: ${TABLE_LINES} linhas"
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
ls -lh "$RESULTS_DIR"/*.json 2>/dev/null || echo "  (nenhum)"

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
echo "  Quick Test concluído!"
echo "=============================================="
echo -e "${NC}"
echo "Tempo total: $SECONDS segundos"
echo ""
echo "Instâncias testadas: bqp50.1, bqp100.1, G1"
echo "Definido em: experiments/config/instances.json -> quick_test"
