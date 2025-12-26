#!/bin/bash
#==============================================================================
# QUBO Hybrid R-Flip Experiments Runner
#==============================================================================
# Este script executa todos os experimentos e gera os gráficos automaticamente.
# As configurações são lidas do arquivo experiments/config/experiments.json
#
# Uso:
#   ./run_experiments.sh [opções]
#
# Opções:
#   --threads N    Limitar número de threads (sobrescreve config)
#   --timeout N    Timeout padrão em segundos (sobrescreve config)
#   --no-compile   Não compilar antes de executar
#   --no-figures   Não gerar figuras
#   --no-tables    Não gerar tabelas
#   --help         Mostrar ajuda
#==============================================================================

set -e  # Parar em caso de erro

#==============================================================================
# DIRETÓRIOS E CAMINHOS
#==============================================================================

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${PROJECT_DIR}/output"
RESULTS_DIR="${OUTPUT_DIR}/results"
FIGURES_DIR="${OUTPUT_DIR}/figures"
TABLES_DIR="${OUTPUT_DIR}/tables"

EXECUTABLE="${PROJECT_DIR}/main.out"
CONFIG_FILE="${PROJECT_DIR}/experiments/config/experiments.json"
CONFIG_READER="${PROJECT_DIR}/experiments/scripts/config_reader.py"
GENFIGURES_SCRIPT="${PROJECT_DIR}/experiments/scripts/genfigures.py"
GENTABLES_SCRIPT="${PROJECT_DIR}/experiments/scripts/gentables.py"
SPLIT_SCRIPT="${PROJECT_DIR}/experiments/scripts/split_by_strategy.py"

# Arquivos de resultados
RESULTS_EVAL="${RESULTS_DIR}/results_eval.json"
RESULTS_LS="${RESULTS_DIR}/results_ls.json"
RESULTS_LS_COUNT="${RESULTS_DIR}/results_ls_count.json"
RESULTS_LS_ALL="${RESULTS_DIR}/results_ls_all.json"
RESULTS_LS_COUNT_ALL="${RESULTS_DIR}/results_ls_count_all.json"
RESULTS_VNS="${RESULTS_DIR}/results_vns.json"
RESULTS_VNS_COUNT="${RESULTS_DIR}/results_vns_count.json"
RESULTS_TABLES="${RESULTS_DIR}/results_tables.json"
RESULTS_COMPLETE="${RESULTS_DIR}/results_complete.json"

#==============================================================================
# OPÇÕES DE LINHA DE COMANDO
#==============================================================================

OVERRIDE_THREADS=""
OVERRIDE_TIMEOUT=""
COMPILE_FIRST=1
GENERATE_FIGURES=1
GENERATE_TABLES=1
SPLIT_BY_STRATEGY=1

while [[ $# -gt 0 ]]; do
    case $1 in
        --threads)
            OVERRIDE_THREADS="$2"
            shift 2
            ;;
        --timeout)
            OVERRIDE_TIMEOUT="$2"
            shift 2
            ;;
        --no-compile)
            COMPILE_FIRST=0
            shift
            ;;
        --no-figures)
            GENERATE_FIGURES=0
            shift
            ;;
        --no-tables)
            GENERATE_TABLES=0
            shift
            ;;
        --help)
            echo "QUBO Hybrid R-Flip Experiments Runner"
            echo ""
            echo "Uso: $0 [opções]"
            echo ""
            echo "Opções:"
            echo "  --threads N    Limitar número de threads (0 = ilimitado)"
            echo "  --timeout N    Timeout padrão em segundos (0 = sem timeout)"
            echo "  --no-compile   Não compilar antes de executar"
            echo "  --no-figures   Não gerar figuras"
            echo "  --no-tables    Não gerar tabelas"
            echo "  --help         Mostrar esta ajuda"
            echo ""
            echo "As configurações são lidas de: experiments/config/experiments.json"
            exit 0
            ;;
        *)
            echo "Opção desconhecida: $1"
            exit 1
            ;;
    esac
done

#==============================================================================
# FUNÇÕES AUXILIARES
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[OK]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERRO]${NC} $1"; }

log_section() {
    echo ""
    echo -e "${CYAN}=============================================================================="
    echo -e " $1"
    echo -e "==============================================================================${NC}"
}

# Função para ler configuração do JSON
get_config() {
    python3 "$CONFIG_READER" "$CONFIG_FILE" "$1"
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
    
    # Configurar limite de threads se especificado
    if [ -n "$MAX_THREADS" ] && [ "$MAX_THREADS" -gt 0 ]; then
        log_info "  Threads: ${MAX_THREADS}"
        export OMP_NUM_THREADS="$MAX_THREADS"
    fi
    
    if [ "$timeout_val" -gt 0 ]; then
        log_info "  Timeout: ${timeout_val}s"
        timeout "$timeout_val" "$EXECUTABLE" "$output_file" "$exp_type" || {
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                log_warning "Experimento interrompido por timeout"
            else
                log_error "Experimento falhou (código: $exit_code)"
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
# CARREGAR CONFIGURAÇÕES
#==============================================================================

log_section "QUBO Hybrid R-Flip Experiments"
echo "Projeto: ${PROJECT_DIR}"
echo "Config:  ${CONFIG_FILE}"
echo "Data:    $(date)"

# Verificar se o config_reader existe
if [ ! -f "$CONFIG_READER" ]; then
    log_error "Config reader não encontrado: ${CONFIG_READER}"
    exit 1
fi

# Carregar configurações do JSON
log_section "Carregando configurações"

if [ -f "$CONFIG_FILE" ]; then
    eval "$(python3 "$CONFIG_READER" "$CONFIG_FILE")"
    log_success "Configurações carregadas de ${CONFIG_FILE}"
else
    log_warning "Arquivo de configuração não encontrado, usando padrões"
    CONFIG_MAX_THREADS=0
    CONFIG_TIMEOUT_EVAL=0
    CONFIG_TIMEOUT_LS=0
    CONFIG_TIMEOUT_LS_COUNT=0
    CONFIG_TIMEOUT_LS_ALL=0
    CONFIG_TIMEOUT_LS_COUNT_ALL=0
    CONFIG_TIMEOUT_VNS=0
    CONFIG_TIMEOUT_VNS_COUNT=0
    CONFIG_TIMEOUT_TABLES=0
    CONFIG_RUN_EVAL=1
    CONFIG_RUN_LS=1
    CONFIG_RUN_LS_COUNT=1
    CONFIG_RUN_LS_ALL=1
    CONFIG_RUN_LS_COUNT_ALL=1
    CONFIG_RUN_VNS=1
    CONFIG_RUN_VNS_COUNT=1
    CONFIG_RUN_TABLES=1
fi

# Aplicar overrides de linha de comando
MAX_THREADS="${OVERRIDE_THREADS:-$CONFIG_MAX_THREADS}"
DEFAULT_TIMEOUT="${OVERRIDE_TIMEOUT:-0}"

TIMEOUT_EVAL="${CONFIG_TIMEOUT_EVAL:-$DEFAULT_TIMEOUT}"
TIMEOUT_LS="${CONFIG_TIMEOUT_LS:-$DEFAULT_TIMEOUT}"
TIMEOUT_LS_COUNT="${CONFIG_TIMEOUT_LS_COUNT:-$DEFAULT_TIMEOUT}"
TIMEOUT_LS_ALL="${CONFIG_TIMEOUT_LS_ALL:-$DEFAULT_TIMEOUT}"
TIMEOUT_LS_COUNT_ALL="${CONFIG_TIMEOUT_LS_COUNT_ALL:-$DEFAULT_TIMEOUT}"
TIMEOUT_VNS="${CONFIG_TIMEOUT_VNS:-$DEFAULT_TIMEOUT}"
TIMEOUT_VNS_COUNT="${CONFIG_TIMEOUT_VNS_COUNT:-$DEFAULT_TIMEOUT}"
TIMEOUT_TABLES="${CONFIG_TIMEOUT_TABLES:-$DEFAULT_TIMEOUT}"

# Se timeout override foi passado, aplicar a todos
if [ -n "$OVERRIDE_TIMEOUT" ]; then
    TIMEOUT_EVAL="$OVERRIDE_TIMEOUT"
    TIMEOUT_LS="$OVERRIDE_TIMEOUT"
    TIMEOUT_LS_COUNT="$OVERRIDE_TIMEOUT"
    TIMEOUT_LS_ALL="$OVERRIDE_TIMEOUT"
    TIMEOUT_LS_COUNT_ALL="$OVERRIDE_TIMEOUT"
    TIMEOUT_VNS="$OVERRIDE_TIMEOUT"
    TIMEOUT_VNS_COUNT="$OVERRIDE_TIMEOUT"
    TIMEOUT_TABLES="$OVERRIDE_TIMEOUT"
fi

RUN_EVAL="${CONFIG_RUN_EVAL:-1}"
RUN_LS="${CONFIG_RUN_LS:-1}"
RUN_LS_COUNT="${CONFIG_RUN_LS_COUNT:-1}"
RUN_LS_ALL="${CONFIG_RUN_LS_ALL:-1}"
RUN_LS_COUNT_ALL="${CONFIG_RUN_LS_COUNT_ALL:-1}"
RUN_VNS="${CONFIG_RUN_VNS:-1}"
RUN_VNS_COUNT="${CONFIG_RUN_VNS_COUNT:-1}"
RUN_TABLES="${CONFIG_RUN_TABLES:-1}"

# Mostrar configurações
echo ""
echo "Configurações de runtime:"
echo "  Max threads:    ${MAX_THREADS} (0 = ilimitado)"
echo "  Timeouts:       eval=${TIMEOUT_EVAL}s, ls=${TIMEOUT_LS}s, vns=${TIMEOUT_VNS}s, tables=${TIMEOUT_TABLES}s"
echo "  Experimentos:   eval=${RUN_EVAL}, ls=${RUN_LS}, ls_all=${RUN_LS_ALL}, vns=${RUN_VNS}, tables=${RUN_TABLES}"

#==============================================================================
# PREPARAÇÃO
#==============================================================================

log_section "Preparando diretórios"

mkdir -p "$RESULTS_DIR"
mkdir -p "$FIGURES_DIR"
mkdir -p "$TABLES_DIR"

log_success "Diretórios criados"

#==============================================================================
# COMPILAÇÃO
#==============================================================================

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

#==============================================================================
# VERIFICAR EXECUTÁVEL
#==============================================================================

if [ ! -x "$EXECUTABLE" ]; then
    log_error "Executável não encontrado: ${EXECUTABLE}"
    exit 1
fi

log_success "Executável encontrado: ${EXECUTABLE}"

#==============================================================================
# EXECUTAR EXPERIMENTOS
#==============================================================================

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

if [ "$RUN_LS_ALL" -eq 1 ]; then
    run_experiment "ls_all" "$RESULTS_LS_ALL" "ls_all" "$TIMEOUT_LS_ALL"
fi

if [ "$RUN_LS_COUNT_ALL" -eq 1 ]; then
    run_experiment "ls_count_all" "$RESULTS_LS_COUNT_ALL" "ls_count_all" "$TIMEOUT_LS_COUNT_ALL"
fi

if [ "$RUN_VNS" -eq 1 ]; then
    run_experiment "vns_figures" "$RESULTS_VNS" "vns_figures" "$TIMEOUT_VNS"
fi

if [ "$RUN_VNS_COUNT" -eq 1 ]; then
    run_experiment "vns_figures_count" "$RESULTS_VNS_COUNT" "vns_figures_count" "$TIMEOUT_VNS_COUNT"
fi

if [ "$RUN_TABLES" -eq 1 ]; then
    run_experiment "vns_tables_all" "$RESULTS_TABLES" "vns_tables_all" "$TIMEOUT_TABLES"
fi

#==============================================================================
# SEPARAR POR ESTRATÉGIA
#==============================================================================

if [ "$SPLIT_BY_STRATEGY" -eq 1 ] && [ -f "$SPLIT_SCRIPT" ]; then
    log_section "Organizando resultados por estratégia de local search"
    
    if [ -f "$RESULTS_LS_ALL" ]; then
        log_info "Separando ls_all por estratégia"
        python3 "$SPLIT_SCRIPT" "$RESULTS_LS_ALL" "$RESULTS_DIR" "ls"
    fi
    
    if [ -f "$RESULTS_LS_COUNT_ALL" ]; then
        log_info "Separando ls_count_all por estratégia"
        python3 "$SPLIT_SCRIPT" "$RESULTS_LS_COUNT_ALL" "$RESULTS_DIR" "ls_count"
    fi
    
    if [ -f "$RESULTS_TABLES" ]; then
        log_info "Separando vns_tables por estratégia"
        python3 "$SPLIT_SCRIPT" "$RESULTS_TABLES" "$RESULTS_DIR" "vns_tables"
    fi
    
    # Remover arquivos soltos
    log_info "Removendo arquivos soltos da raiz de results/"
    rm -f "$RESULTS_LS_ALL" "$RESULTS_LS_COUNT_ALL" 2>/dev/null || true
    rm -rf "${RESULTS_DIR}/by_strategy" 2>/dev/null || true
    rm -f "${RESULTS_DIR}/results_ls.json" "${RESULTS_DIR}/results_ls_count.json" 2>/dev/null || true
    rm -f "${RESULTS_DIR}/results_tables.json" 2>/dev/null || true
    rm -f "${RESULTS_DIR}/results_eval.json" 2>/dev/null || true
    rm -f "${RESULTS_DIR}/results_vns.json" "${RESULTS_DIR}/results_vns_count.json" 2>/dev/null || true
    rm -f "${RESULTS_DIR}/results_complete.json" 2>/dev/null || true
fi

#==============================================================================
# MESCLAR RESULTADOS
#==============================================================================

log_section "Mesclando resultados"

RESULT_FILES=()
[ -f "$RESULTS_EVAL" ] && RESULT_FILES+=("$RESULTS_EVAL")
[ -f "$RESULTS_LS" ] && RESULT_FILES+=("$RESULTS_LS")
[ -f "$RESULTS_LS_COUNT" ] && RESULT_FILES+=("$RESULTS_LS_COUNT")
[ -f "$RESULTS_LS_ALL" ] && RESULT_FILES+=("$RESULTS_LS_ALL")
[ -f "$RESULTS_LS_COUNT_ALL" ] && RESULT_FILES+=("$RESULTS_LS_COUNT_ALL")
[ -f "$RESULTS_VNS" ] && RESULT_FILES+=("$RESULTS_VNS")
[ -f "$RESULTS_VNS_COUNT" ] && RESULT_FILES+=("$RESULTS_VNS_COUNT")
[ -f "$RESULTS_TABLES" ] && RESULT_FILES+=("$RESULTS_TABLES")

if [ ${#RESULT_FILES[@]} -gt 0 ]; then
    merge_results "${RESULT_FILES[@]}"
    log_success "Resultados mesclados em ${RESULTS_COMPLETE}"
else
    log_warning "Nenhum arquivo de resultados encontrado para mesclar"
fi

#==============================================================================
# GERAR FIGURAS
#==============================================================================

if [ "$GENERATE_FIGURES" -eq 1 ]; then
    log_section "Gerando figuras"
    
    if [ -f "$GENFIGURES_SCRIPT" ]; then
        cd "$PROJECT_DIR"
        
        if [ -d ".venv" ]; then
            source .venv/bin/activate
            log_info "Ambiente virtual ativado"
        fi
        
        for STRATEGY_DIR in "$RESULTS_DIR"/*/; do
            if [ -d "$STRATEGY_DIR" ]; then
                STRATEGY_NAME=$(basename "$STRATEGY_DIR")
                STRATEGY_RESULTS="${STRATEGY_DIR}/results_complete.json"
                STRATEGY_FIGURES="${FIGURES_DIR}/${STRATEGY_NAME}"
                
                if [ -f "$STRATEGY_RESULTS" ]; then
                    log_info "Gerando figuras para: ${STRATEGY_NAME}"
                    mkdir -p "$STRATEGY_FIGURES"
                    python3 "$GENFIGURES_SCRIPT" "$STRATEGY_RESULTS" "$STRATEGY_FIGURES/" 2>&1 | grep -E '(✓|Error|No )' || true
                fi
            fi
        done
        
        log_info "Removendo figuras antigas da raiz"
        find "$FIGURES_DIR" -maxdepth 1 -type f \( -name "*.pdf" -o -name "*.eps" -o -name "*.tex" \) -delete 2>/dev/null || true
        
        PDF_COUNT=$(find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | wc -l)
        EPS_COUNT=$(find "$FIGURES_DIR" -name "*.eps" 2>/dev/null | wc -l)
        
        log_success "Figuras geradas: ${PDF_COUNT} PDFs, ${EPS_COUNT} EPS"
        
        find "$FIGURES_DIR" -type f \( -name "*.aux" -o -name "*.dvi" -o -name "*.log" \) -delete 2>/dev/null || true
    else
        log_warning "Script de figuras não encontrado: ${GENFIGURES_SCRIPT}"
    fi
fi

#==============================================================================
# GERAR TABELAS
#==============================================================================

if [ "$GENERATE_TABLES" -eq 1 ]; then
    log_section "Gerando tabelas"
    
    if [ -f "$GENTABLES_SCRIPT" ]; then
        for STRATEGY_DIR in "$RESULTS_DIR"/*/; do
            if [ -d "$STRATEGY_DIR" ]; then
                STRATEGY_NAME=$(basename "$STRATEGY_DIR")
                STRATEGY_RESULTS="${STRATEGY_DIR}/results_complete.json"
                STRATEGY_TABLES="${TABLES_DIR}/${STRATEGY_NAME}"
                
                if [ -f "$STRATEGY_RESULTS" ]; then
                    log_info "Gerando tabelas para: ${STRATEGY_NAME}"
                    mkdir -p "$STRATEGY_TABLES"
                    cd "$STRATEGY_TABLES"
                    python3 "$GENTABLES_SCRIPT" "$STRATEGY_RESULTS" > tables_output.txt 2>&1 || true
                    
                    if [ -s "tables_output.txt" ]; then
                        log_success "Tabelas geradas em ${STRATEGY_TABLES}/tables_output.txt"
                    fi
                fi
            fi
        done
        
        rm -f "${TABLES_DIR}/tables_output.txt" 2>/dev/null || true
    else
        log_warning "Script de tabelas não encontrado: ${GENTABLES_SCRIPT}"
    fi
fi

#==============================================================================
# RESUMO FINAL
#==============================================================================

log_section "Resumo Final"

echo "Diretório de resultados: ${RESULTS_DIR}"
echo ""
echo "Arquivos de resultados:"
ls -lh "$RESULTS_DIR"/*.json 2>/dev/null || echo "  (nenhum arquivo encontrado)"

echo ""
echo "Pastas por estratégia:"
ls -d "$RESULTS_DIR"/*/ 2>/dev/null || echo "  (nenhuma pasta encontrada)"

echo ""
echo "Figuras geradas:"
find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | head -5 || echo "  (nenhuma figura encontrada)"
PDF_COUNT=$(find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | wc -l)
if [ "$PDF_COUNT" -gt 5 ]; then
    echo "  ... e mais $((PDF_COUNT - 5)) figuras"
fi

log_section "Script concluído com sucesso!"
echo "Tempo total: $SECONDS segundos"
