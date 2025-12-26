/**
 * @file basic.hpp
 * @brief Basic O(n²) evaluation algorithm for UBQP neighbor solutions.
 *
 * Computes the objective function by iterating over all non-zero components.
 */

#ifndef QUBO_ALGORITHMS_EVALUATE_BASIC_HPP
#define QUBO_ALGORITHMS_EVALUATE_BASIC_HPP

#include <cassert>
#include <memory>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "verify.hpp"

namespace qubo {

/**
 * @brief Evaluates a neighbor solution with the basic O(n²) algorithm.
 *
 * Computes the objective function from scratch by iterating over all
 * non-zero components.
 *
 * @tparam eval Evaluation strategy
 * @param Q UBQP instance
 * @param y Incumbent solution
 * @param z Neighbor solution (output: fz, N1z, n1z)
 * @param N Permutation array (first r elements are the flipped indices)
 */
template <evaluation eval>
void evaluate_basic(const ubqp& Q, const incumbent_solution<eval>& y, neighbor_solution& z,
                    const std::unique_ptr<size_t[]>& N) {
  // Build list of non-zero components in neighbor
  z.n1z = 0;
  for (size_t k = 0; k < z.r01; k++) {
    z.N1z[z.n1z++] = z.R01[k];
  }
  for (size_t k = z.r01 + z.r10; k < Q.n; k++) {
    size_t i = N[k];
    if (y.y[i]) z.N1z[z.n1z++] = i;
  }

  // Compute objective
  z.fz = 0;
  for (size_t m = 0; m < z.n1z; m++) {
    size_t i = z.N1z[m];
    z.fz += Q[i][i];
    for (size_t l = 0; l < m; l++) {
      size_t j = z.N1z[l];
      z.fz += Q[i][j];
    }
  }

  assert(check_evaluation(Q, y, z));
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_EVALUATE_BASIC_HPP
