/**
 * @file local_search.hpp
 * @brief Local Search algorithms for UBQP problems.
 *
 * This is the main include file for local search algorithms.
 * It provides various local search strategies:
 *
 * - first_improvement: Accept first improving neighbor
 * - (future: best_improvement, simulated_annealing, tabu_search, etc.)
 *
 * Structure:
 * - local_search/randomize.hpp       : Solution initialization
 * - local_search/first_improvement.hpp : First-improvement LS
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_HPP

#include "local_search/randomize.hpp"
#include "local_search/first_improvement.hpp"

// Note: The following includes are for backward compatibility
// and will be used by the local search implementations
#include "evaluate/hybrid.hpp"
#include "neighbor/generate.hpp"
#include "neighbor/replace.hpp"

namespace qubo {

// All local search types are available through their respective headers:
// - randomize<eval>()
// - ls<eval, count>()
// - ls_first_improvement<eval, count>()

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_LOCAL_SEARCH_HPP
