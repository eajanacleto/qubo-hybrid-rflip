/**
 * @file vns.hpp
 * @brief Variable Neighborhood Search algorithm for UBQP problems.
 *
 * Implements VNS with varying neighborhood sizes for UBQP.
 */

#ifndef QUBO_ALGORITHMS_VNS_HPP
#define QUBO_ALGORITHMS_VNS_HPP

#include <algorithm>
#include <memory>
#include <random>

#include "../core/evaluation.hpp"
#include "../core/solution.hpp"
#include "../core/ubqp.hpp"
#include "evaluate.hpp"
#include "local_search.hpp"

namespace qubo {

/**
 * @brief Performs Variable Neighborhood Search.
 *
 * VNS explores increasingly larger neighborhoods to escape local minima.
 * After each local search, if no improvement is found, the neighborhood
 * size increases. On improvement, it resets to the smallest neighborhood.
 *
 * @tparam eval Evaluation strategy
 * @tparam count Whether to count algorithm choices
 * @param Q UBQP instance
 * @param y_inc Best incumbent solution (input/output)
 * @param y Working solution (workspace)
 * @param z Neighbor solution workspace
 * @param z2 Second neighbor solution workspace (for local search)
 * @param r_max Maximum neighborhood size
 * @param r_step Neighborhood size increment
 * @param iters VNS iterations
 * @param ls_iters Local search iterations
 * @param N Permutation array workspace
 * @param rng Random number generator
 * @param basics Counter for basic evaluations
 * @param deltas Counter for delta evaluations
 */
template <evaluation eval, bool count>
void vns(const ubqp& Q, incumbent_solution<eval>& y_inc, incumbent_solution<eval>& y,
         neighbor_solution& z, neighbor_solution& z2, size_t r_max, size_t r_step,
         size_t iters, size_t ls_iters, std::unique_ptr<size_t[]>& N,
         std::mt19937& rng, size_t& basics, size_t& deltas) {
  for (size_t l = 1; l <= iters; l++) {
    for (size_t r = 1; r <= r_max; r += r_step) {
      // Copy incumbent to working solution
      y.fy = y_inc.fy;
      for (size_t i = 0; i < Q.n; i++) y.y[i] = y_inc.y[i];

      if constexpr (eval != evaluation::basic) {
        y.nx1 = y_inc.nx1;
        y.wx01 = y_inc.wx01;
        y.wx10 = y_inc.wx10;
        for (size_t i = 0; i < Q.n; i++) y.x[i] = y_inc.x[i];
        for (size_t i = 0; i < Q.n; i++) y.dx[i] = y_inc.dx[i];
        for (size_t i = 0; i < y.wx01; i++) y.WX01[i] = y_inc.WX01[i];
        for (size_t i = 0; i < y.wx10; i++) y.WX10[i] = y_inc.WX10[i];
      }

      // Shake: generate random neighbor with r flips
      random_neighbor_solution(Q, y, z, r, N, rng);
      evaluate_hybrid<eval, count>(Q, y, z, N, basics, deltas);
      replace_incumbent(y, z);

      // Local search on shaken solution
      ls<eval, count>(Q, y, z2, r, ls_iters, N, rng, basics, deltas);

      // Move or not
      if (y.fy < y_inc.fy) {
        std::swap(y_inc, y);
        r = 0;
        l = 1;
      }
    }
  }
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_VNS_HPP
