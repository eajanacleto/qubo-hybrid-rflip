/**
 * @file ls_strategy.hpp
 * @brief Local Search Strategy enumeration and registry.
 *
 * This file defines the available local search strategies and provides
 * a registry system that allows new strategies to be automatically
 * included in experiments.
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_LS_STRATEGY_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_LS_STRATEGY_HPP

#include <string>

namespace qubo {

/**
 * @brief Enumeration of available local search strategies.
 *
 * Add new strategies here - they will be automatically included
 * in experiments if the corresponding implementation is provided.
 */
enum class ls_strategy : int {
  first_improvement = 0,   ///< Accept first improving neighbor
  best_improvement = 1,    ///< Evaluate all neighbors, accept best
  // Add new strategies here:
  // simulated_annealing = 2,
  // tabu_search = 3,
};

/**
 * @brief Get string name for a local search strategy.
 */
inline std::string ls_strategy_name(ls_strategy s) {
  switch (s) {
    case ls_strategy::first_improvement: return "first_improvement";
    case ls_strategy::best_improvement: return "best_improvement";
    default: return "unknown";
  }
}

/**
 * @brief Get short name for a local search strategy (for file naming).
 */
inline std::string ls_strategy_short_name(ls_strategy s) {
  switch (s) {
    case ls_strategy::first_improvement: return "fi";
    case ls_strategy::best_improvement: return "bi";
    default: return "?";
  }
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_LOCAL_SEARCH_LS_STRATEGY_HPP
