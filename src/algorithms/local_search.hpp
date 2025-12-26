/**
 * @file local_search.hpp
 * @brief Local Search algorithms for UBQP problems.
 *
 * This is the main include file for local search algorithms.
 * It provides various local search strategies that can be used
 * interchangeably in experiments:
 *
 * - first_improvement: Accept first improving neighbor (fast per iteration)
 * - best_improvement: Evaluate all neighbors, accept best (thorough)
 * - (future: simulated_annealing, tabu_search, etc.)
 *
 * Structure:
 * - local_search/ls_strategy.hpp        : Strategy enumeration
 * - local_search/randomize.hpp          : Solution initialization
 * - local_search/first_improvement.hpp  : First-improvement LS
 * - local_search/best_improvement.hpp   : Best-improvement LS
 *
 * Adding a new local search strategy:
 * 1. Add enum value to ls_strategy in ls_strategy.hpp
 * 2. Create new_strategy.hpp with ls_new_strategy<eval, count>()
 * 3. Include it here
 * 4. Add case to ls_dispatch<eval, count, strategy>()
 * 5. Add to get_ls_strategies() in ls_experiment.hpp
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_HPP

// Strategy enumeration
#include "local_search/ls_strategy.hpp"

// Solution initialization
#include "local_search/randomize.hpp"

// Local search strategies
#include "local_search/first_improvement.hpp"
#include "local_search/best_improvement.hpp"

// Dependencies for implementations
#include "evaluate/hybrid.hpp"
#include "neighbor/generate.hpp"
#include "neighbor/replace.hpp"

namespace qubo {

/**
 * @brief Dispatch to the appropriate local search strategy at runtime.
 *
 * This allows selecting the local search strategy dynamically.
 *
 * @tparam eval Evaluation strategy
 * @tparam count Whether to count basic vs delta evaluations
 * @param strategy Which local search strategy to use
 */
template <evaluation eval, bool count = false>
void ls_dispatch(
    ls_strategy strategy,
    const ubqp& Q,
    incumbent_solution<eval>& y,
    neighbor_solution& z,
    size_t r,
    size_t iters,
    std::unique_ptr<size_t[]>& N,
    std::mt19937& rng,
    size_t& basics,
    size_t& deltas) {
  
  switch (strategy) {
    case ls_strategy::first_improvement:
      ls_first_improvement<eval, count>(Q, y, z, r, iters, N, rng, basics, deltas);
      break;
    case ls_strategy::best_improvement:
      ls_best_improvement<eval, count>(Q, y, z, r, iters, N, rng, basics, deltas);
      break;
    // Add new strategies here:
    // case ls_strategy::simulated_annealing:
    //   ls_simulated_annealing<eval, count>(...);
    //   break;
    default:
      throw std::runtime_error("Unknown local search strategy");
  }
}

/**
 * @brief Get list of all available local search strategies.
 * 
 * Use this to iterate over all strategies in experiments.
 */
inline std::vector<ls_strategy> get_all_ls_strategies() {
  return {
    ls_strategy::first_improvement,
    ls_strategy::best_improvement,
    // Add new strategies here
  };
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_LOCAL_SEARCH_HPP

