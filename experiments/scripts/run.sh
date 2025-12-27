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
    default_timeout = exp.get("timeout", 0)
    
    # Support algorithms as list of strings or objects
    raw_algos = exp.get("algorithms", [])
    if not raw_algos:
        single = exp.get("algorithm", "")
        raw_algos = [single] if single else []
    
    algo_parts = []
    for algo in raw_algos:
        if isinstance(algo, dict):
            name_str = algo.get("name", "?")
            t = algo.get("timeout", default_timeout)
            algo_parts.append(f"{name_str}({t}s)")
        else:
            algo_parts.append(str(algo))
    
    algo_str = ", ".join(algo_parts) if algo_parts else "N/A"
    instances = exp.get("instance_set", "N/A")
    timeout_str = f"{default_timeout}s" if default_timeout > 0 else "unlimited"
    
    print(f"  {name:20s} - {desc}")
    print(f"                       Algorithms: {algo_str}")
    print(f"                       Instances: {instances}, Default timeout: {timeout_str}")
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

# Support both 'algorithm' (single) and 'algorithms' (list)
# Each algorithm can be a string or {"name": "algo", "timeout": 123}
raw_algorithms = exp.get('algorithms', [])
if not raw_algorithms:
    single = exp.get('algorithm', '')
    raw_algorithms = [single] if single else []

default_timeout = exp.get('timeout', 0)
algo_names = []
algo_timeouts = []

for algo in raw_algorithms:
    if isinstance(algo, dict):
        algo_names.append(algo.get('name', ''))
        algo_timeouts.append(str(algo.get('timeout', default_timeout)))
    else:
        algo_names.append(str(algo))
        algo_timeouts.append(str(default_timeout))

print(f"EXP_ALGORITHMS={shell_escape(','.join(algo_names))}")
print(f"EXP_ALGO_TIMEOUTS={shell_escape(','.join(algo_timeouts))}")
print(f"EXP_INSTANCE_SET={shell_escape(exp.get('instance_set', ''))}")
print(f"EXP_TIMEOUT={default_timeout}")
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
    
    # Cada algoritmo tem seu próprio arquivo de saída
    local output_file="${RESULTS_DIR}/results_${exp_name}_${algorithm}.json"
    local stdout_log="${RESULTS_DIR}/.stdout_${exp_name}_${algorithm}.log"
    
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
    
    # Executar com ou sem timeout, capturando stdout
    local was_timeout=0
    if [ "$timeout" -gt 0 ]; then
        log_info "  Timeout: ${timeout}s"
        timeout "$timeout" "$EXECUTABLE" "$output_file" "$algorithm" "$instance_set" 2>&1 | tee "$stdout_log" || {
            local exit_code=$?
            if [ $exit_code -eq 124 ]; then
                log_warning "Experimento interrompido por timeout"
                was_timeout=1
            else
                log_error "Experimento falhou (código: $exit_code)"
                rm -f "$stdout_log"
                return 1
            fi
        }
    else
        "$EXECUTABLE" "$output_file" "$algorithm" "$instance_set" 2>&1 | tee "$stdout_log"
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    # Validar e corrigir JSON se necessário (pode estar truncado pelo timeout)
    if [ -f "$output_file" ]; then
        python3 << EOF
import json
import sys
import re
import os

output_file = "$output_file"
stdout_log = "$stdout_log"
was_timeout = $was_timeout

def recover_from_stdout(log_file):
    """Recover results from stdout log (<<< params / >>> result pairs)"""
    if not os.path.exists(log_file):
        return []
    
    results = []
    current_params = None
    
    with open(log_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('<<< '):
                try:
                    current_params = json.loads(line[4:])
                except:
                    current_params = None
            elif line.startswith('>>> ') and current_params:
                try:
                    result = json.loads(line[4:])
                    results.append({"params": current_params, "result": result})
                except:
                    pass
                current_params = None
    
    return results

try:
    # Verificar se arquivo existe e tem conteúdo
    if not os.path.exists(output_file) or os.path.getsize(output_file) == 0:
        # Tentar recuperar do stdout
        if was_timeout and os.path.exists(stdout_log):
            results = recover_from_stdout(stdout_log)
            if results:
                with open(output_file, 'w') as f:
                    json.dump(results, f)
                print(f"Recuperado do stdout: {len(results)} resultados")
            else:
                with open(output_file, 'w') as f:
                    json.dump([], f)
                print("Nenhum resultado recuperado")
        else:
            with open(output_file, 'w') as f:
                json.dump([], f)
            print("Arquivo vazio criado")
        sys.exit(0)
    
    with open(output_file, 'r') as f:
        content = f.read().strip()
    
    # Tentar parsear normalmente
    try:
        data = json.loads(content)
        count = len(data) if isinstance(data, list) else 1
        print(f"JSON válido: {count} resultados")
        sys.exit(0)
    except json.JSONDecodeError as e:
        print(f"JSON truncado, tentando recuperar do stdout...")
    
    # JSON truncado - recuperar do stdout log
    if os.path.exists(stdout_log):
        results = recover_from_stdout(stdout_log)
        if results:
            with open(output_file, 'w') as f:
                json.dump(results, f)
            print(f"Recuperado do stdout: {len(results)} resultados")
            sys.exit(0)
    
    # Última tentativa: regex no arquivo JSON
    pattern = r'\{"params":\s*(\{[^}]+\}),\s*"result":\s*(\{[^}]+\})\}'
    matches = re.findall(pattern, content)
    
    results = []
    for params_str, result_str in matches:
        try:
            params = json.loads(params_str)
            result = json.loads(result_str)
            results.append({"params": params, "result": result})
        except:
            pass
    
    if results:
        with open(output_file, 'w') as f:
            json.dump(results, f)
        print(f"Recuperado com regex: {len(results)} resultados")
    else:
        with open(output_file, 'w') as f:
            json.dump([], f)
        print("Nenhum resultado recuperado")

except Exception as e:
    print(f"Erro: {e}", file=sys.stderr)
    with open(output_file, 'w') as f:
        json.dump([], f)
EOF
        
        # Limpar log de stdout
        rm -f "$stdout_log"
        
        local results=$(python3 -c "import json; print(len(json.load(open('$output_file'))))" 2>/dev/null || echo 0)
        log_success "Experimento ${exp_name}/${algorithm} concluído em ${duration}s (${results} resultados)"
    else
        log_warning "Arquivo de saída não gerado"
    fi
}

run_experiment() {
    local exp_name="$1"
    
    log_section "Experimento: ${exp_name}"
    
    # Carregar configuração do experimento
    eval "$(get_experiment_config "$exp_name")"
    
    if [ -z "$EXP_ALGORITHMS" ] && [ "$EXP_IS_PIPELINE" != "1" ]; then
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
    
    # Executar todos os algoritmos do experimento com seus timeouts específicos
    IFS=',' read -ra ALGO_LIST <<< "$EXP_ALGORITHMS"
    IFS=',' read -ra TIMEOUT_LIST <<< "$EXP_ALGO_TIMEOUTS"
    local algo_count=${#ALGO_LIST[@]}
    local algo_idx=0
    
    for algorithm in "${ALGO_LIST[@]}"; do
        # Timeout: override > específico do algoritmo > padrão do experimento
        local algo_timeout="${TIMEOUT_LIST[$algo_idx]}"
        local timeout="${OVERRIDE_TIMEOUT:-$algo_timeout}"
        
        algo_idx=$((algo_idx + 1))
        if [ $algo_count -gt 1 ]; then
            log_info "Algoritmo $algo_idx/$algo_count: $algorithm (timeout: ${timeout}s)"
        fi
        run_single_experiment "$exp_name" "$algorithm" "$EXP_INSTANCE_SET" "$timeout"
    done
    
    # Juntar resultados de todos os algoritmos em um arquivo consolidado
    local combined_file="${RESULTS_DIR}/results_${exp_name}.json"
    log_info "Consolidando resultados..."
    
    python3 << EOF
import json
import glob
import os

results_dir = "$RESULTS_DIR"
exp_name = "$exp_name"
algorithms = "$EXP_ALGORITHMS".split(',')

all_results = []
files_found = []

for algo in algorithms:
    algo_file = os.path.join(results_dir, f"results_{exp_name}_{algo}.json")
    if os.path.exists(algo_file):
        try:
            with open(algo_file) as f:
                data = json.load(f)
                if isinstance(data, list):
                    all_results.extend(data)
                    files_found.append(f"{algo}: {len(data)} results")
        except Exception as e:
            print(f"  Aviso: Erro ao ler {algo_file}: {e}")

if all_results:
    with open("$combined_file", 'w') as f:
        json.dump(all_results, f)
    print(f"  Arquivos processados: {len(files_found)}")
    for f in files_found:
        print(f"    {f}")
    print(f"  Total consolidado: {len(all_results)} resultados")
else:
    print("  Nenhum resultado para consolidar")
EOF
    
    # Separar por estratégia se necessário
    if [ -f "$combined_file" ] && [ -f "$SPLIT_SCRIPT" ]; then
        if grep -q '"ls_strategy_name"' "$combined_file" 2>/dev/null; then
            log_info "Separando resultados por estratégia de local search..."
            python3 "$SPLIT_SCRIPT" "$combined_file" "$RESULTS_DIR" "${exp_name}"
        fi
    fi
    
    # Gerar figuras
    if [ "$EXP_GEN_FIGURES" = "1" ] && [ "$GENERATE_FIGURES" = "1" ]; then
        generate_figures "$combined_file"
    fi
    
    # Gerar tabelas
    if [ "$EXP_GEN_TABLES" = "1" ] && [ "$GENERATE_TABLES" = "1" ]; then
        generate_tables "$combined_file"
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
    echo -e "${CYAN}" >&2
    echo "==============================================" >&2
    echo "  QUBO Hybrid R-Flip - Experiment Runner" >&2
    echo "==============================================" >&2
    echo -e "${NC}" >&2
    echo "" >&2
    
    list_experiments >&2
    
    echo -e "${BOLD}Digite o nome do experimento (ou 'q' para sair):${NC}" >&2
    read -r choice </dev/tty
    
    if [ "$choice" = "q" ] || [ "$choice" = "quit" ] || [ "$choice" = "exit" ]; then
        echo "Saindo..." >&2
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
