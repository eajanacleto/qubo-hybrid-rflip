/**
 * @file rfliprv.hpp
 * @brief R-Flip-RV evaluation algorithm for UBQP neighbor solutions.
 *
 * Uses the reevaluation vector for O(r²) evaluation instead of O(n²).
 */

#ifndef QUBO_ALGORITHMS_EVALUATE_RFLIPRV_HPP
#define QUBO_ALGORITHMS_EVALUATE_RFLIPRV_HPP

#include <cassert>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "verify.hpp"

namespace qubo {

/**
 * @brief Evaluates a neighbor solution using the r-flip-rv algorithm.
 *
 * Uses the reevaluation vector for O(r²) evaluation instead of O(n²).
 *
 * @tparam eval Evaluation strategy (must not be basic)
 * @param Q UBQP instance
 * @param y Incumbent solution
 * @param z Neighbor solution (output: fz)
 */
template <evaluation eval>
void evaluate_rfliprv(const ubqp& Q, const incumbent_solution<eval>& y, neighbor_solution& z) {
  static_assert(eval != evaluation::basic);

  z.fz = y.fy;

  // Subtract contributions from bits flipped 1→0
  for (size_t m = 0; m < z.r10; m++) {
    size_t i = z.R10[m];
    z.fz -= y.dx[i];
    for (size_t l = m + 1; l < z.r10; l++) {
      size_t j = z.R10[l];
      z.fz += Q[j][i];
    }
  }

  // Add contributions from bits flipped 0→1
  for (size_t m = 0; m < z.r01; m++) {
    size_t i = z.R01[m];
    z.fz += y.dx[i];
    z.fz += Q[i][i];

    for (size_t l = 0; l < m; l++) {
      size_t j = z.R01[l];
      z.fz += Q[i][j];
    }
    for (size_t l = 0; l < z.r10; l++) {
      size_t j = z.R10[l];
      z.fz -= (j <= i) ? Q[i][j] : Q[j][i];
    }
  }

  assert(check_evaluation(Q, y, z));
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_EVALUATE_RFLIPRV_HPP
