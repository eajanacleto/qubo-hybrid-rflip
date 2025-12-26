/**
 * @file first_improvement.hpp
 * @brief First-improvement local search strategy.
 *
 * Accepts the first improving neighbor found in the neighborhood.
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_FIRST_IMPROVEMENT_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_FIRST_IMPROVEMENT_HPP

#include <memory>
#include <random>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "../evaluate/hybrid.hpp"
#include "../neighbor/generate.hpp"
#include "../neighbor/replace.hpp"

namespace qubo {

/**
 * @brief Performs first-improvement local search on an incumbent solution.
 *
 * Uses r-flip neighborhood with first-improvement acceptance.
 * Terminates after `iters` consecutive non-improving iterations.
 *
 * @tparam eval Evaluation strategy
 * @tparam count Whether to count algorithm choices
 * @param Q UBQP instance
 * @param y Incumbent solution (input/output)
 * @param z Neighbor solution workspace
 * @param r Neighborhood size (number of flips)
 * @param iters Maximum consecutive non-improving iterations
 * @param N Permutation array workspace
 * @param rng Random number generator
 * @param basics Counter for basic evaluations (output if count=true)
 * @param deltas Counter for delta evaluations (output if count=true)
 */
template <evaluation eval, bool count>
void ls_first_improvement(const ubqp& Q, incumbent_solution<eval>& y, 
                          neighbor_solution& z, size_t r, size_t iters,
                          std::unique_ptr<size_t[]>& N, std::mt19937& rng, 
                          size_t& basics, size_t& deltas) {
  for (size_t l = 1; l <= iters; l++) {
    random_neighbor_solution(Q, y, z, r, N, rng);
    evaluate_hybrid<eval, count>(Q, y, z, N, basics, deltas);

    if (z.fz < y.fy) {
      replace_incumbent(y, z);
      l = 0;  // Reset counter on improvement
    }
  }
}

/**
 * @brief Alias for backward compatibility.
 */
template <evaluation eval, bool count>
inline void ls(const ubqp& Q, incumbent_solution<eval>& y, 
               neighbor_solution& z, size_t r, size_t iters,
               std::unique_ptr<size_t[]>& N, std::mt19937& rng, 
               size_t& basics, size_t& deltas) {
  ls_first_improvement<eval, count>(Q, y, z, r, iters, N, rng, basics, deltas);
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_LOCAL_SEARCH_FIRST_IMPROVEMENT_HPP
