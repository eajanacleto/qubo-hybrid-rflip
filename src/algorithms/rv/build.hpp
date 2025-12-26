/**
 * @file build.hpp
 * @brief Functions to build the reevaluation vector from scratch.
 */

#ifndef QUBO_ALGORITHMS_RV_BUILD_HPP
#define QUBO_ALGORITHMS_RV_BUILD_HPP

#include <cassert>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "../evaluate/verify.hpp"

namespace qubo {

/**
 * @brief Rebuilds the reevaluation vector from scratch.
 *
 * Computes dx[i] = Σ_j Q[i][j] * y[j] for all i.
 * Time complexity: O(n²)
 *
 * @tparam eval Evaluation strategy (must not be basic)
 * @param Q UBQP instance
 * @param y Incumbent solution (output: dx updated)
 */
template <evaluation eval>
void build_rv(const ubqp& Q, incumbent_solution<eval>& y) {
  static_assert(eval != evaluation::basic);

  for (size_t i = 0; i < Q.n; i++) y.dx[i] = 0;

  for (size_t j = 0; j < Q.n; j++) {
    if (y.y[j]) {
      for (size_t i = 0; i < Q.n; i++) {
        y.dx[i] += Q[i][j];
      }
    }
  }

  assert(check_rv(Q, y));
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_RV_BUILD_HPP
