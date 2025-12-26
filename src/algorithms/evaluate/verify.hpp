/**
 * @file verify.hpp
 * @brief Verification functions for debugging UBQP solutions.
 *
 * These functions verify the correctness of evaluation results,
 * reevaluation vectors, and change sets.
 */

#ifndef QUBO_ALGORITHMS_EVALUATE_VERIFY_HPP
#define QUBO_ALGORITHMS_EVALUATE_VERIFY_HPP

#include <memory>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"

namespace qubo {

/**
 * @brief Verifies that a neighbor solution has the correct objective function value.
 *
 * This is a debug function that recomputes the objective from scratch.
 *
 * @tparam eval Evaluation strategy
 * @param Q UBQP instance
 * @param y Incumbent solution
 * @param z Neighbor solution to verify
 * @return true if z.fz matches the recomputed value
 */
template <evaluation eval>
bool check_evaluation(const ubqp& Q, const incumbent_solution<eval>& y,
                      const neighbor_solution& z) {
  std::unique_ptr<bool[]> z_tmp(new bool[Q.n]());

  // Reconstruct neighbor solution
  for (size_t i = 0; i < Q.n; i++) z_tmp[i] = y.y[i];
  for (size_t i = 0; i < z.r01; i++) z_tmp[z.R01[i]] = 1;
  for (size_t i = 0; i < z.r10; i++) z_tmp[z.R10[i]] = 0;

  // Compute objective from scratch
  long fz_tmp = 0;
  for (size_t i = 0; i < Q.n; i++) {
    for (size_t j = 0; j <= i; j++) {
      if (z_tmp[i] && z_tmp[j]) fz_tmp += Q[i][j];
    }
  }

  return z.fz == fz_tmp;
}

/**
 * @brief Verifies that the reevaluation vector is correct.
 *
 * @tparam eval Evaluation strategy (must not be basic)
 * @param Q UBQP instance
 * @param y Incumbent solution
 * @return true if dx matches the expected values
 */
template <evaluation eval>
bool check_rv(const ubqp& Q, const incumbent_solution<eval>& y) {
  static_assert(eval != evaluation::basic);

  for (size_t i = 0; i < Q.n; i++) {
    long dxi_tmp = 0;
    for (size_t j = 0; j < Q.n; j++) {
      if (y.y[j]) dxi_tmp += Q[i][j];
    }
    if (y.dx[i] != dxi_tmp) return false;
  }
  return true;
}

/**
 * @brief Verifies that the change sets WX01 and WX10 are correct.
 *
 * @tparam eval Evaluation strategy (must not be basic)
 * @param y Incumbent solution
 * @return true if change sets are consistent
 */
template <evaluation eval>
bool check_sets(const incumbent_solution<eval>& y) {
  static_assert(eval != evaluation::basic);

  for (size_t k = 0; k < y.wx01; k++) {
    size_t i = y.WX01[k];
    if (y.x[i] != 0 || y.y[i] != 1) return false;
  }
  for (size_t k = 0; k < y.wx10; k++) {
    size_t i = y.WX10[k];
    if (y.x[i] != 1 || y.y[i] != 0) return false;
  }
  return true;
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_EVALUATE_VERIFY_HPP
