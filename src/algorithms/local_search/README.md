# Local Search Strategies

Este módulo contém as implementações de estratégias de busca local para o problema UBQP.

## Estrutura

```
local_search/
├── README.md                  # Esta documentação
├── ls_strategy.hpp            # Enum com todas as estratégias
├── randomize.hpp              # Inicialização de soluções
├── first_improvement.hpp      # Estratégia first-improvement
└── best_improvement.hpp       # Estratégia best-improvement
```

## Estratégias Disponíveis

| Estratégia | Arquivo | Descrição |
|------------|---------|-----------|
| `first_improvement` | `first_improvement.hpp` | Aceita o primeiro vizinho que melhora a solução |
| `best_improvement` | `best_improvement.hpp` | Avalia múltiplos vizinhos e aceita o melhor |

## Como Adicionar uma Nova Estratégia

Para adicionar uma nova estratégia de busca local (ex: `tabu_search`), siga estes passos:

### Passo 1: Adicionar ao enum `ls_strategy`

Edite `ls_strategy.hpp` e adicione o novo valor ao enum:

```cpp
enum class ls_strategy : int {
  first_improvement = 0,
  best_improvement = 1,
  tabu_search = 2,        // <- NOVO
};
```

Atualize também as funções de nome:

```cpp
inline std::string ls_strategy_name(ls_strategy s) {
  switch (s) {
    case ls_strategy::first_improvement: return "first_improvement";
    case ls_strategy::best_improvement: return "best_improvement";
    case ls_strategy::tabu_search: return "tabu_search";  // <- NOVO
    default: return "unknown";
  }
}

inline std::string ls_strategy_short_name(ls_strategy s) {
  switch (s) {
    case ls_strategy::first_improvement: return "fi";
    case ls_strategy::best_improvement: return "bi";
    case ls_strategy::tabu_search: return "ts";  // <- NOVO
    default: return "?";
  }
}
```

### Passo 2: Criar o arquivo da estratégia

Crie `tabu_search.hpp` seguindo o template:

```cpp
#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_TABU_SEARCH_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_TABU_SEARCH_HPP

#include <memory>
#include <random>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "../evaluate/hybrid.hpp"
#include "../neighbor/generate.hpp"
#include "../neighbor/replace.hpp"

namespace qubo {

template <evaluation eval, bool count = false>
void ls_tabu_search(
    const ubqp& Q,
    incumbent_solution<eval>& y,
    neighbor_solution& z,
    size_t r,
    size_t iters,
    std::unique_ptr<size_t[]>& N,
    std::mt19937& rng,
    size_t& basics,
    size_t& deltas) {
  
  // Sua implementação aqui
  // Use random_neighbor_solution(), evaluate_hybrid<>(), replace_incumbent()
}

}  // namespace qubo

#endif
```

### Passo 3: Incluir no arquivo principal

Edite `../local_search.hpp` e adicione o include:

```cpp
#include "local_search/tabu_search.hpp"  // <- NOVO
```

### Passo 4: Adicionar ao dispatcher

No mesmo arquivo `../local_search.hpp`, adicione o case no `ls_dispatch`:

```cpp
template <evaluation eval, bool count = false>
void ls_dispatch(ls_strategy strategy, ...) {
  switch (strategy) {
    case ls_strategy::first_improvement:
      ls_first_improvement<eval, count>(...);
      break;
    case ls_strategy::best_improvement:
      ls_best_improvement<eval, count>(...);
      break;
    case ls_strategy::tabu_search:           // <- NOVO
      ls_tabu_search<eval, count>(...);      // <- NOVO
      break;
    default:
      throw std::runtime_error("Unknown local search strategy");
  }
}
```

E adicione à lista de estratégias:

```cpp
inline std::vector<ls_strategy> get_all_ls_strategies() {
  return {
    ls_strategy::first_improvement,
    ls_strategy::best_improvement,
    ls_strategy::tabu_search,  // <- NOVO
  };
}
```

### Passo 5: Registrar nos experimentos

Edite `../../experiments/ls_experiment.hpp` e adicione nas funções `get_all_ls_time_experiments()` e `get_all_ls_count_experiments()`:

```cpp
inline auto get_all_ls_time_experiments() {
  std::vector<...> result;
  
  // Estratégias existentes...
  
  // Tabu Search <- NOVO
  for (const auto& [eval, fn] : get_ls_time_experiments_for_strategy<ls_strategy::tabu_search>()) {
    result.emplace_back(ls_strategy::tabu_search, eval, fn);
  }
  
  return result;
}
```

### Passo 6: Recompilar e testar

```bash
make clean && make
./main.out /tmp/test.json ls_all
```

## Executando Experimentos

### Experimentos por estratégia específica (backward compatible)

```bash
# Apenas first_improvement (padrão)
./main.out output/results/ls.json ls
./main.out output/results/ls_count.json ls_count
```

### Experimentos com TODAS as estratégias

```bash
# Todas as estratégias de LS
./main.out output/results/ls_all.json ls_all
./main.out output/results/ls_count_all.json ls_count_all
```

## Formato dos Resultados

Os resultados JSON incluem informações sobre a estratégia usada:

```json
{
  "params": {
    "exp": "ls_all",
    "instance": "bqp100.1",
    "eval": 2,
    "ls_strategy": 1,
    "ls_strategy_name": "best_improvement",
    "r": 50,
    "iters": 100
  },
  "result": {
    "dt": 123456789,
    "fx": -7324
  }
}
```

## API das Funções de Local Search

Todas as estratégias devem seguir esta assinatura:

```cpp
template <evaluation eval, bool count = false>
void ls_STRATEGY(
    const ubqp& Q,                      // Instância do problema
    incumbent_solution<eval>& y,         // Solução incumbente (entrada/saída)
    neighbor_solution& z,                // Workspace para vizinho
    size_t r,                            // Tamanho da vizinhança (r-flip)
    size_t iters,                        // Máximo de iterações sem melhoria
    std::unique_ptr<size_t[]>& N,        // Permutação de índices
    std::mt19937& rng,                   // Gerador de números aleatórios
    size_t& basics,                      // [out] Contagem de avaliações básicas
    size_t& deltas);                     // [out] Contagem de avaliações delta
```

## Funções Auxiliares Disponíveis

- `randomize(Q, y, rng)` - Inicializa solução aleatória
- `random_neighbor_solution(Q, y, z, r, N, rng)` - Gera vizinho aleatório
- `evaluate_hybrid<eval, count>(Q, y, z, N, basics, deltas)` - Avalia vizinho
- `replace_incumbent(y, z)` - Substitui incumbente pelo vizinho
