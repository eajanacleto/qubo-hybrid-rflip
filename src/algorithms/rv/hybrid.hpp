/**
 * @file hybrid.hpp
 * @brief Hybrid strategy for updating the reevaluation vector.
 *
 * Chooses between build_rv and update_rv based on estimated operation counts.
 */

#ifndef QUBO_ALGORITHMS_RV_HYBRID_HPP
#define QUBO_ALGORITHMS_RV_HYBRID_HPP

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "build.hpp"
#include "update.hpp"

namespace qubo {

/**
 * @brief Updates the reevaluation vector using hybrid strategy selection.
 *
 * Chooses between build_rv and update_rv based on estimated operation counts.
 *
 * @tparam eval Evaluation strategy (must not be basic)
 * @param Q UBQP instance
 * @param y Incumbent solution (output: dx, x, wx01, wx10 updated)
 */
template <evaluation eval>
void update_rv_hybrid(const ubqp& Q, incumbent_solution<eval>& y) {
  static_assert(eval != evaluation::basic);

  size_t r = y.wx01 + y.wx10;
  if (!r) return;

  size_t basic_ops = 0, delta_ops = 0;

  // Compute estimated operation counts based on strategy
  if constexpr (eval == evaluation::s) {
    basic_ops = Q.n * y.nx1;
    delta_ops = Q.n * r;
  } else if constexpr (eval == evaluation::a) {
    basic_ops = Q.n + y.nx1 + Q.n + Q.n * y.nx1 + Q.n * y.nx1;
    delta_ops = Q.n * (2 * r + 1);
  } else if constexpr (eval == evaluation::c) {
    basic_ops = Q.n + 1 + Q.n + Q.n + 1 + Q.n * (y.nx1 + 1);
    delta_ops = 1 + Q.n * (r + 3);
  } else if constexpr (eval == evaluation::m) {
    basic_ops = 3 + Q.n + y.nx1 + Q.n + Q.n + Q.n * y.nx1 + 3 * Q.n * y.nx1;
    delta_ops = 1 + Q.n * (4 * r + 3);
  } else if constexpr (eval == evaluation::ac) {
    basic_ops = Q.n + y.nx1 + Q.n + Q.n * y.nx1 + Q.n * y.nx1;
    basic_ops += Q.n + 1 + Q.n + Q.n + 1 + Q.n * (y.nx1 + 1);
    delta_ops = Q.n * (2 * r + 1);
    delta_ops += 1 + Q.n * (r + 3);
  } else if constexpr (eval == evaluation::am) {
    basic_ops = Q.n + y.nx1 + Q.n + Q.n * y.nx1 + Q.n * y.nx1;
    basic_ops += 3 + Q.n + y.nx1 + Q.n + Q.n + Q.n * y.nx1 + 3 * Q.n * y.nx1;
    delta_ops = Q.n * (2 * r + 1);
    delta_ops += 1 + Q.n * (4 * r + 3);
  } else if constexpr (eval == evaluation::cm) {
    basic_ops = Q.n + 1 + Q.n + Q.n + 1 + Q.n * (y.nx1 + 1);
    basic_ops += 3 + Q.n + y.nx1 + Q.n + Q.n + Q.n * y.nx1 + 3 * Q.n * y.nx1;
    delta_ops = 1 + Q.n * (r + 3);
    delta_ops += 1 + Q.n * (4 * r + 3);
  } else if constexpr (eval == evaluation::acm) {
    basic_ops = Q.n + y.nx1 + Q.n + Q.n * y.nx1 + Q.n * y.nx1;
    basic_ops += Q.n + 1 + Q.n + Q.n + 1 + Q.n * (y.nx1 + 1);
    basic_ops += 3 + Q.n + y.nx1 + Q.n + Q.n + Q.n * y.nx1 + 3 * Q.n * y.nx1;
    delta_ops = Q.n * (2 * r + 1);
    delta_ops += 1 + Q.n * (r + 3);
    delta_ops += 1 + Q.n * (4 * r + 3);
  }

  // Choose algorithm
  if constexpr (eval == evaluation::rflip_rv) {
    update_rv(Q, y);
  } else {
    if (basic_ops < delta_ops)
      build_rv(Q, y);
    else
      update_rv(Q, y);
  }

  // Reset change tracking
  y.wx10 = y.wx01 = 0;
  for (size_t i = 0; i < Q.n; i++) y.x[i] = y.y[i];
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_RV_HYBRID_HPP
