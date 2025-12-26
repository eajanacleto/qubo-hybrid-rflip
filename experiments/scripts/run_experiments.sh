#!/bin/bash
#==============================================================================
# QUBO Hybrid R-Flip Experiments Runner
#==============================================================================
# Este script executa todos os experimentos e gera os gráficos automaticamente.
# Configure os parâmetros abaixo conforme necessário.
#==============================================================================

set -e  # Parar em caso de erro

#==============================================================================
# CONFIGURAÇÕES - Altere conforme necessário
#==============================================================================

# Diretórios
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${PROJECT_DIR}/output"
RESULTS_DIR="${OUTPUT_DIR}/results"
FIGURES_DIR="${OUTPUT_DIR}/figures"
TABLES_DIR="${OUTPUT_DIR}/tables"

# Executável
EXECUTABLE="${PROJECT_DIR}/main.out"

# Scripts Python
GENFIGURES_SCRIPT="${PROJECT_DIR}/experiments/scripts/genfigures.py"
GENTABLES_SCRIPT="${PROJECT_DIR}/experiments/scripts/gentables.py"

# Arquivos de resultados
RESULTS_EVAL="${RESULTS_DIR}/results_eval.json"
RESULTS_LS="${RESULTS_DIR}/results_ls.json"
RESULTS_LS_COUNT="${RESULTS_DIR}/results_ls_count.json"
RESULTS_VNS="${RESULTS_DIR}/results_vns.json"
RESULTS_VNS_COUNT="${RESULTS_DIR}/results_vns_count.json"
RESULTS_TABLES="${RESULTS_DIR}/results_tables.json"
RESULTS_COMPLETE="${RESULTS_DIR}/results_complete.json"

# Timeouts (em segundos) - 0 = sem timeout
TIMEOUT_EVAL=0          # Experimento de avaliação
TIMEOUT_LS=0            # Local Search
TIMEOUT_LS_COUNT=0      # Local Search com contagem
TIMEOUT_VNS=0           # VNS para figuras
TIMEOUT_VNS_COUNT=0     # VNS com contagem
TIMEOUT_TABLES=0        # VNS para tabelas (mais longo)

# Experimentos a executar (1=sim, 0=não)
RUN_EVAL=1
RUN_LS=1
RUN_LS_COUNT=1
RUN_VNS=1
RUN_VNS_COUNT=1
RUN_TABLES=0            # Desativado por padrão (muito demorado)

# Geração de figuras e tabelas
GENERATE_FIGURES=1
GENERATE_TABLES=1

# Compilar antes de executar
COMPILE_FIRST=1

# Modo verbose
VERBOSE=1

#==============================================================================
# FUNÇÕES AUXILIARES
#==============================================================================

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERRO]${NC} $1"
}

log_section() {
    echo ""
    echo -e "${CYAN}=============================================================================="
    echo -e " $1"
    echo -e "==============================================================================${NC}"
}

# Função para executar experimento com timeout opcional
run_experiment() {
    local name="$1"
    local output_file="$2"
    local exp_type="$3"
    local timeout_val="$4"
    
    log_info "Executando experimento: ${name}"
    log_info "  Arquivo de saída: ${output_file}"
    
    local start_time=$(date +%s)
    
    if [ "$timeout_val" -gt 0 ]; then
        log_info "  Timeout: ${timeout_val}s"
        timeout "$timeout_val" "$EXECUTABLE" "$output_file" "$exp_type" || {
            if [ $? -eq 124 ]; then
                log_warning "Experimento interrompido por timeout"
            else
                log_error "Experimento falhou"
                return 1
            fi
        }
    else
        "$EXECUTABLE" "$output_file" "$exp_type"
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    log_success "Experimento ${name} concluído em ${duration}s"
}

# Função para mesclar resultados JSON
merge_results() {
    log_info "Mesclando resultados em ${RESULTS_COMPLETE}..."
    
    python3 - "$RESULTS_COMPLETE" "$@" << 'EOF'
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
            else:
                all_results.append(data)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"Aviso: Não foi possível ler {f}: {e}", file=sys.stderr)

with open(output_file, 'w') as fp:
    json.dump(all_results, fp, indent=1)

print(f"Total de resultados mesclados: {len(all_results)}")
EOF
}

#==============================================================================
# INÍCIO DO SCRIPT
#==============================================================================

log_section "QUBO Hybrid R-Flip Experiments"
echo "Projeto: ${PROJECT_DIR}"
echo "Data: $(date)"

#------------------------------------------------------------------------------
# Criar diretórios
#------------------------------------------------------------------------------
log_section "Preparando diretórios"

mkdir -p "$RESULTS_DIR"
mkdir -p "$FIGURES_DIR"
mkdir -p "$TABLES_DIR"

log_success "Diretórios criados"

#------------------------------------------------------------------------------
# Compilar projeto
#------------------------------------------------------------------------------
if [ "$COMPILE_FIRST" -eq 1 ]; then
    log_section "Compilando projeto"
    
    cd "$PROJECT_DIR"
    
    if [ -f "Makefile" ]; then
        make clean && make
        log_success "Compilação concluída"
    else
        log_error "Makefile não encontrado"
        exit 1
    fi
fi

#------------------------------------------------------------------------------
# Verificar executável
#------------------------------------------------------------------------------
if [ ! -x "$EXECUTABLE" ]; then
    log_error "Executável não encontrado: ${EXECUTABLE}"
    exit 1
fi

log_success "Executável encontrado: ${EXECUTABLE}"

#------------------------------------------------------------------------------
# Executar experimentos
#------------------------------------------------------------------------------
log_section "Executando experimentos"

if [ "$RUN_EVAL" -eq 1 ]; then
    run_experiment "eval" "$RESULTS_EVAL" "eval" "$TIMEOUT_EVAL"
fi

if [ "$RUN_LS" -eq 1 ]; then
    run_experiment "ls" "$RESULTS_LS" "ls" "$TIMEOUT_LS"
fi

if [ "$RUN_LS_COUNT" -eq 1 ]; then
    run_experiment "ls_count" "$RESULTS_LS_COUNT" "ls_count" "$TIMEOUT_LS_COUNT"
fi

if [ "$RUN_VNS" -eq 1 ]; then
    run_experiment "vns_figures" "$RESULTS_VNS" "vns_figures" "$TIMEOUT_VNS"
fi

if [ "$RUN_VNS_COUNT" -eq 1 ]; then
    run_experiment "vns_figures_count" "$RESULTS_VNS_COUNT" "vns_figures_count" "$TIMEOUT_VNS_COUNT"
fi

if [ "$RUN_TABLES" -eq 1 ]; then
    run_experiment "vns_tables" "$RESULTS_TABLES" "vns_tables" "$TIMEOUT_TABLES"
fi

#------------------------------------------------------------------------------
# Mesclar resultados
#------------------------------------------------------------------------------
log_section "Mesclando resultados"

RESULT_FILES=()
[ -f "$RESULTS_EVAL" ] && RESULT_FILES+=("$RESULTS_EVAL")
[ -f "$RESULTS_LS" ] && RESULT_FILES+=("$RESULTS_LS")
[ -f "$RESULTS_LS_COUNT" ] && RESULT_FILES+=("$RESULTS_LS_COUNT")
[ -f "$RESULTS_VNS" ] && RESULT_FILES+=("$RESULTS_VNS")
[ -f "$RESULTS_VNS_COUNT" ] && RESULT_FILES+=("$RESULTS_VNS_COUNT")
[ -f "$RESULTS_TABLES" ] && RESULT_FILES+=("$RESULTS_TABLES")

if [ ${#RESULT_FILES[@]} -gt 0 ]; then
    merge_results "${RESULT_FILES[@]}"
    log_success "Resultados mesclados em ${RESULTS_COMPLETE}"
else
    log_warning "Nenhum arquivo de resultados encontrado para mesclar"
fi

#------------------------------------------------------------------------------
# Gerar figuras
#------------------------------------------------------------------------------
if [ "$GENERATE_FIGURES" -eq 1 ] && [ -f "$RESULTS_COMPLETE" ]; then
    log_section "Gerando figuras"
    
    if [ -f "$GENFIGURES_SCRIPT" ]; then
        cd "$PROJECT_DIR"
        
        # Ativar ambiente virtual se existir
        if [ -d ".venv" ]; then
            source .venv/bin/activate
            log_info "Ambiente virtual ativado"
        fi
        
        python3 "$GENFIGURES_SCRIPT" "$RESULTS_COMPLETE" "$FIGURES_DIR/"
        
        # Contar figuras geradas
        PDF_COUNT=$(find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | wc -l)
        EPS_COUNT=$(find "$FIGURES_DIR" -name "*.eps" 2>/dev/null | wc -l)
        
        log_success "Figuras geradas: ${PDF_COUNT} PDFs, ${EPS_COUNT} EPS"
        
        # Limpar arquivos temporários
        rm -f "$FIGURES_DIR"/*.aux "$FIGURES_DIR"/*.dvi "$FIGURES_DIR"/*.log
    else
        log_warning "Script de figuras não encontrado: ${GENFIGURES_SCRIPT}"
    fi
fi

#------------------------------------------------------------------------------
# Gerar tabelas
#------------------------------------------------------------------------------
if [ "$GENERATE_TABLES" -eq 1 ] && [ -f "$RESULTS_COMPLETE" ]; then
    log_section "Gerando tabelas"
    
    if [ -f "$GENTABLES_SCRIPT" ]; then
        cd "$TABLES_DIR"
        python3 "$GENTABLES_SCRIPT" "$RESULTS_COMPLETE" > tables_output.txt 2>&1 || true
        
        if [ -s "tables_output.txt" ]; then
            log_success "Tabelas geradas em ${TABLES_DIR}/tables_output.txt"
        fi
    else
        log_warning "Script de tabelas não encontrado: ${GENTABLES_SCRIPT}"
    fi
fi

#------------------------------------------------------------------------------
# Resumo final
#------------------------------------------------------------------------------
log_section "Resumo Final"

echo "Diretório de resultados: ${RESULTS_DIR}"
echo ""
echo "Arquivos de resultados:"
ls -lh "$RESULTS_DIR"/*.json 2>/dev/null || echo "  (nenhum arquivo encontrado)"

echo ""
echo "Figuras geradas:"
ls -lh "$FIGURES_DIR"/*.pdf 2>/dev/null | head -10 || echo "  (nenhuma figura encontrada)"

if [ $(find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | wc -l) -gt 10 ]; then
    echo "  ... e mais $(( $(find "$FIGURES_DIR" -name "*.pdf" | wc -l) - 10 )) figuras"
fi

log_section "Script concluído com sucesso!"
echo "Tempo total: $SECONDS segundos"
