/**
 * @file local_search.hpp
 * @brief Local Search algorithm for UBQP problems.
 *
 * Implements a stochastic local search using r-flip neighborhood.
 *
 * Structure:
 * - neighbor/generate.hpp : Generate random neighbors
 * - neighbor/replace.hpp  : Replace incumbent with neighbor
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_HPP

#include <algorithm>
#include <memory>
#include <random>

#include "../core/evaluation.hpp"
#include "../core/solution.hpp"
#include "../core/ubqp.hpp"
#include "evaluate/hybrid.hpp"
#include "neighbor/generate.hpp"
#include "neighbor/replace.hpp"

namespace qubo {

/**
 * @brief Randomizes an incumbent solution and computes its reevaluation vector.
 *
 * @tparam eval Evaluation strategy
 * @param Q UBQP instance
 * @param y Incumbent solution (output: randomized)
 * @param rng Random number generator
 */
template <evaluation eval>
void randomize(const ubqp& Q, incumbent_solution<eval>& y, std::mt19937& rng) {
  // Randomly set each bit to 0 or 1
  for (size_t i = 0; i < Q.n; i++) {
    if (rng() % 2) {
      y.y[i] = 1;
      if constexpr (eval != evaluation::basic) {
        y.x[i] = 1;
        y.nx1++;
      }
    }
  }

  // Compute objective function
  for (size_t i = 0; i < Q.n; i++) {
    for (size_t j = 0; j <= i; j++) {
      if (y.y[i] && y.y[j]) y.fy += Q[i][j];
    }
  }

  // Build reevaluation vector if needed
  if constexpr (eval != evaluation::basic) {
    for (size_t i = 0; i < Q.n; i++) {
      for (size_t j = 0; j < Q.n; j++) {
        if (y.y[j]) y.dx[i] += Q[i][j];
      }
    }
  }
}

/**
 * @brief Performs a stochastic local search on an incumbent solution.
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
 * @param basics Counter for basic evaluations
 * @param deltas Counter for delta evaluations
 */
template <evaluation eval, bool count>
void ls(const ubqp& Q, incumbent_solution<eval>& y, neighbor_solution& z, size_t r, size_t iters,
        std::unique_ptr<size_t[]>& N, std::mt19937& rng, size_t& basics, size_t& deltas) {
  for (size_t l = 1; l <= iters; l++) {
    random_neighbor_solution(Q, y, z, r, N, rng);
    evaluate_hybrid<eval, count>(Q, y, z, N, basics, deltas);

    if (z.fz < y.fy) {
      replace_incumbent(y, z);
      l = 0;  // Reset counter on improvement
    }
  }
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_LOCAL_SEARCH_HPP
