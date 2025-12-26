#!/bin/bash
#==============================================================================
# QUBO Hybrid R-Flip - Experiment Runner
#==============================================================================
# Script unificado para executar experimentos.
# Lê configurações de experiments/config/experiments.json e instances.json
#
# Uso:
#   ./run.sh                    # Lista experimentos e pede para escolher
#   ./run.sh <experiment>       # Executa experimento específico
#   ./run.sh --list             # Lista experimentos disponíveis
#   ./run.sh --help             # Mostra ajuda
#
# Opções:
#   --threads N     Número de threads (0 = ilimitado)
#   --timeout N     Override de timeout em segundos
#   --no-compile    Não compilar antes de executar
#   --no-figures    Não gerar figuras
#   --no-tables     Não gerar tabelas
#==============================================================================

set -e

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
INSTANCES_FILE="${PROJECT_DIR}/experiments/config/instances.json"
GENFIGURES_SCRIPT="${PROJECT_DIR}/experiments/scripts/genfigures.py"
GENTABLES_SCRIPT="${PROJECT_DIR}/experiments/scripts/gentables.py"
SPLIT_SCRIPT="${PROJECT_DIR}/experiments/scripts/split_by_strategy.py"

#==============================================================================
# CORES E FUNÇÕES
#==============================================================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

log_info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[OK]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error()   { echo -e "${RED}[ERRO]${NC} $1"; }

log_section() {
    echo ""
    echo -e "${CYAN}=============================================================================="
    echo -e " $1"
    echo -e "==============================================================================${NC}"
}

#==============================================================================
# FUNÇÕES DE CONFIGURAÇÃO
#==============================================================================

# Lista experimentos disponíveis
list_experiments() {
    echo -e "${BOLD}Experimentos disponíveis:${NC}"
    echo ""
    python3 << 'EOF'
import json
import sys

with open("experiments/config/experiments.json") as f:
    config = json.load(f)

experiments = config.get("experiments", {})
for name, exp in experiments.items():
    desc = exp.get("description", "No description")
    algo = exp.get("algorithm", "N/A")
    instances = exp.get("instance_set", "N/A")
    timeout = exp.get("timeout", 0)
    timeout_str = f"{timeout}s" if timeout > 0 else "unlimited"
    
    print(f"  {name:20s} - {desc}")
    print(f"                       Algorithm: {algo}, Instances: {instances}, Timeout: {timeout_str}")
    print()
EOF
}

# Obtém configuração de um experimento
get_experiment_config() {
    local exp_name="$1"
    python3 << EOF
import json
import sys
import shlex

with open("$CONFIG_FILE") as f:
    config = json.load(f)

exp = config.get("experiments", {}).get("$exp_name")
if not exp:
    print("ERROR: Experiment not found", file=sys.stderr)
    sys.exit(1)

# Output shell variables (escape values for shell)
def shell_escape(s):
    return shlex.quote(str(s)) if s else "''"

print(f"EXP_ALGORITHM={shell_escape(exp.get('algorithm', ''))}")
print(f"EXP_INSTANCE_SET={shell_escape(exp.get('instance_set', ''))}")
print(f"EXP_TIMEOUT={exp.get('timeout', 0)}")
print(f"EXP_GEN_FIGURES={1 if exp.get('generate_figures', False) else 0}")
print(f"EXP_GEN_TABLES={1 if exp.get('generate_tables', False) else 0}")
print(f"EXP_DESCRIPTION={shell_escape(exp.get('description', ''))}")

# Check for pipeline steps
steps = exp.get('steps', [])
if steps:
    print(f"EXP_IS_PIPELINE=1")
    print(f"EXP_STEPS={','.join(steps)}")
else:
    print(f"EXP_IS_PIPELINE=0")
    print(f"EXP_STEPS=")
EOF
}

# Lista instâncias de um conjunto
list_instances() {
    local set_name="$1"
    python3 << EOF
import json

with open("$INSTANCES_FILE") as f:
    config = json.load(f)

def get_instances(name):
    s = config["instance_sets"].get(name, {})
    if "compose" in s:
        result = []
        for sub in s["compose"]:
            result.extend(get_instances(sub))
        return result
    return s.get("instances", [])

instances = get_instances("$set_name")
print(f"Instance set '$set_name': {len(instances)} instances")
print(f"  {', '.join(instances[:5])}{'...' if len(instances) > 5 else ''}")
EOF
}

#==============================================================================
# FUNÇÕES DE EXECUÇÃO
#==============================================================================

compile_project() {
    log_section "Compilando projeto"
    cd "$PROJECT_DIR"
    make clean && make
    log_success "Compilação concluída"
}

run_single_experiment() {
    local exp_name="$1"
    local algorithm="$2"
    local instance_set="$3"
    local timeout="$4"
    
    local output_file="${RESULTS_DIR}/results_${exp_name}.json"
    
    log_info "Executando: ${exp_name}"
    log_info "  Algoritmo: ${algorithm}"
    log_info "  Instâncias: ${instance_set}"
    log_info "  Saída: ${output_file}"
    
    local start_time=$(date +%s)
    
    # Configurar threads
    if [ -n "$MAX_THREADS" ] && [ "$MAX_THREADS" -gt 0 ]; then
        log_info "  Threads: ${MAX_THREADS}"
        export OMP_NUM_THREADS="$MAX_THREADS"
    fi
    
    # Executar com ou sem timeout
    if [ "$timeout" -gt 0 ]; then
        log_info "  Timeout: ${timeout}s"
        timeout "$timeout" "$EXECUTABLE" "$output_file" "$algorithm" "$instance_set" || {
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                log_warning "Experimento interrompido por timeout"
            else
                log_error "Experimento falhou (código: $exit_code)"
                return 1
            fi
        }
    else
        "$EXECUTABLE" "$output_file" "$algorithm" "$instance_set"
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    if [ -f "$output_file" ]; then
        local results=$(grep -c '"result"' "$output_file" 2>/dev/null || echo 0)
        log_success "Experimento ${exp_name} concluído em ${duration}s (${results} resultados)"
    else
        log_warning "Arquivo de saída não gerado"
    fi
}

run_experiment() {
    local exp_name="$1"
    
    log_section "Experimento: ${exp_name}"
    
    # Carregar configuração do experimento
    eval "$(get_experiment_config "$exp_name")"
    
    if [ -z "$EXP_ALGORITHM" ] && [ "$EXP_IS_PIPELINE" != "1" ]; then
        log_error "Experimento não encontrado: ${exp_name}"
        return 1
    fi
    
    log_info "Descrição: ${EXP_DESCRIPTION}"
    
    # Se for pipeline, executar cada step
    if [ "$EXP_IS_PIPELINE" = "1" ]; then
        log_info "Pipeline com ${EXP_STEPS//,/ -> }"
        IFS=',' read -ra STEPS <<< "$EXP_STEPS"
        for step in "${STEPS[@]}"; do
            run_experiment "$step"
        done
        return 0
    fi
    
    # Mostrar instâncias
    list_instances "$EXP_INSTANCE_SET"
    
    # Aplicar override de timeout se especificado
    local timeout="${OVERRIDE_TIMEOUT:-$EXP_TIMEOUT}"
    
    # Executar experimento
    run_single_experiment "$exp_name" "$EXP_ALGORITHM" "$EXP_INSTANCE_SET" "$timeout"
    
    # Separar por estratégia se necessário
    local output_file="${RESULTS_DIR}/results_${exp_name}.json"
    if [ -f "$output_file" ] && [ -f "$SPLIT_SCRIPT" ]; then
        if grep -q '"ls_strategy_name"' "$output_file" 2>/dev/null; then
            log_info "Separando resultados por estratégia de local search..."
            python3 "$SPLIT_SCRIPT" "$output_file" "$RESULTS_DIR" "${exp_name}"
        fi
    fi
    
    # Gerar figuras
    if [ "$EXP_GEN_FIGURES" = "1" ] && [ "$GENERATE_FIGURES" = "1" ]; then
        generate_figures "$output_file"
    fi
    
    # Gerar tabelas
    if [ "$EXP_GEN_TABLES" = "1" ] && [ "$GENERATE_TABLES" = "1" ]; then
        generate_tables "$output_file"
    fi
}

generate_figures() {
    local results_file="$1"
    
    if [ ! -f "$GENFIGURES_SCRIPT" ]; then
        log_warning "Script de figuras não encontrado"
        return
    fi
    
    log_info "Gerando figuras..."
    
    if [ -d "$PROJECT_DIR/.venv" ]; then
        source "$PROJECT_DIR/.venv/bin/activate"
    fi
    
    mkdir -p "$FIGURES_DIR"
    python3 "$GENFIGURES_SCRIPT" "$results_file" "$FIGURES_DIR/" 2>&1 || {
        log_warning "Alguns gráficos podem não ter sido gerados"
    }
    
    # Limpar temporários
    rm -f "$FIGURES_DIR"/*.aux "$FIGURES_DIR"/*.dvi "$FIGURES_DIR"/*.log 2>/dev/null || true
    
    local pdf_count=$(find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | wc -l)
    log_success "Figuras geradas: ${pdf_count} PDFs"
}

generate_tables() {
    local results_file="$1"
    
    if [ ! -f "$GENTABLES_SCRIPT" ]; then
        log_warning "Script de tabelas não encontrado"
        return
    fi
    
    log_info "Gerando tabelas..."
    
    mkdir -p "$TABLES_DIR"
    local tables_output="$TABLES_DIR/tables_output.txt"
    
    python3 "$GENTABLES_SCRIPT" "$results_file" > "$tables_output" 2>&1 || true
    
    if [ -s "$tables_output" ]; then
        local lines=$(wc -l < "$tables_output")
        log_success "Tabelas geradas: ${lines} linhas"
    else
        log_warning "Nenhuma tabela gerada (dados insuficientes)"
    fi
}

#==============================================================================
# MENU INTERATIVO
#==============================================================================

show_menu() {
    echo -e "${CYAN}"
    echo "=============================================="
    echo "  QUBO Hybrid R-Flip - Experiment Runner"
    echo "=============================================="
    echo -e "${NC}"
    echo ""
    
    list_experiments
    
    echo -e "${BOLD}Digite o nome do experimento (ou 'q' para sair):${NC}"
    read -r choice
    
    if [ "$choice" = "q" ] || [ "$choice" = "quit" ] || [ "$choice" = "exit" ]; then
        echo "Saindo..."
        exit 0
    fi
    
    echo "$choice"
}

#==============================================================================
# PARSING DE ARGUMENTOS
#==============================================================================

EXPERIMENT=""
MAX_THREADS=""
OVERRIDE_TIMEOUT=""
COMPILE_FIRST=1
GENERATE_FIGURES=1
GENERATE_TABLES=1

while [[ $# -gt 0 ]]; do
    case $1 in
        --list|-l)
            list_experiments
            exit 0
            ;;
        --threads)
            MAX_THREADS="$2"
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
        --help|-h)
            echo "QUBO Hybrid R-Flip - Experiment Runner"
            echo ""
            echo "Uso: $0 [experiment] [opções]"
            echo ""
            echo "Se nenhum experimento for especificado, mostra menu interativo."
            echo ""
            echo "Opções:"
            echo "  --list, -l       Lista experimentos disponíveis"
            echo "  --threads N      Número de threads (0 = ilimitado)"
            echo "  --timeout N      Override de timeout em segundos"
            echo "  --no-compile     Não compilar antes de executar"
            echo "  --no-figures     Não gerar figuras"
            echo "  --no-tables      Não gerar tabelas"
            echo "  --help, -h       Mostra esta ajuda"
            echo ""
            echo "Exemplos:"
            echo "  $0 quick_test                    # Executa quick_test"
            echo "  $0 vns_tables_small --threads 4 # Com 4 threads"
            echo "  $0 --list                       # Lista experimentos"
            echo ""
            echo "Configurações em:"
            echo "  experiments/config/experiments.json  - Definição dos experimentos"
            echo "  experiments/config/instances.json    - Conjuntos de instâncias"
            exit 0
            ;;
        -*)
            log_error "Opção desconhecida: $1"
            exit 1
            ;;
        *)
            EXPERIMENT="$1"
            shift
            ;;
    esac
done

#==============================================================================
# MAIN
#==============================================================================

cd "$PROJECT_DIR"

# Se nenhum experimento especificado, mostrar menu
if [ -z "$EXPERIMENT" ]; then
    EXPERIMENT=$(show_menu)
fi

# Verificar se experimento existe
python3 -c "
import json
with open('$CONFIG_FILE') as f:
    config = json.load(f)
if '$EXPERIMENT' not in config.get('experiments', {}):
    print('ERROR')
    exit(1)
" || {
    log_error "Experimento não encontrado: ${EXPERIMENT}"
    echo ""
    list_experiments
    exit 1
}

log_section "QUBO Hybrid R-Flip Experiments"
echo "Projeto:     ${PROJECT_DIR}"
echo "Experimento: ${EXPERIMENT}"
echo "Data:        $(date)"

# Preparar diretórios
mkdir -p "$RESULTS_DIR" "$FIGURES_DIR" "$TABLES_DIR"

# Compilar se necessário
if [ "$COMPILE_FIRST" -eq 1 ]; then
    compile_project
fi

# Verificar executável
if [ ! -x "$EXECUTABLE" ]; then
    log_error "Executável não encontrado: ${EXECUTABLE}"
    log_info "Execute 'make' primeiro"
    exit 1
fi

# Executar experimento
run_experiment "$EXPERIMENT"

# Resumo final
log_section "Resumo"

echo ""
echo "Resultados em: ${RESULTS_DIR}"
ls -lh "$RESULTS_DIR"/*.json 2>/dev/null | head -10 || echo "  (nenhum arquivo)"

echo ""
echo "Figuras em: ${FIGURES_DIR}"
pdf_count=$(find "$FIGURES_DIR" -name "*.pdf" 2>/dev/null | wc -l)
echo "  PDFs: $pdf_count"

echo ""
echo "Tabelas em: ${TABLES_DIR}"
if [ -f "$TABLES_DIR/tables_output.txt" ]; then
    lines=$(wc -l < "$TABLES_DIR/tables_output.txt")
    echo "  Linhas: $lines"
fi

echo ""
echo -e "${GREEN}=============================================="
echo "  Experimento concluído!"
echo "=============================================="
echo -e "${NC}"
echo "Tempo total: $SECONDS segundos"
