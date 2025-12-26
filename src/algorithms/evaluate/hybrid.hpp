/**
 * @file hybrid.hpp
 * @brief Hybrid evaluation strategy for UBQP neighbor solutions.
 *
 * Chooses between basic and r-flip-rv evaluation based on estimated costs.
 */

#ifndef QUBO_ALGORITHMS_EVALUATE_HYBRID_HPP
#define QUBO_ALGORITHMS_EVALUATE_HYBRID_HPP

#include <memory>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "../rv/hybrid.hpp"
#include "basic.hpp"
#include "rfliprv.hpp"

namespace qubo {

/**
 * @brief Evaluates a neighbor solution using hybrid strategy selection.
 *
 * Chooses between basic and r-flip-rv evaluation based on estimated costs.
 *
 * @tparam eval Evaluation strategy
 * @tparam count Whether to count algorithm choices
 * @param Q UBQP instance
 * @param y Incumbent solution
 * @param z Neighbor solution (output: fz)
 * @param N Permutation array
 * @param basics Counter for basic evaluations (output if count=true)
 * @param deltas Counter for delta evaluations (output if count=true)
 */
template <evaluation eval, bool count>
void evaluate_hybrid(const ubqp& Q, incumbent_solution<eval>& y, neighbor_solution& z,
                     const std::unique_ptr<size_t[]>& N, size_t& basics, size_t& deltas) {
  size_t r = z.r01 + z.r10;
  size_t basic_ops = 0, delta_ops = 0;

  // Compute estimated operation counts
  if constexpr (eval == evaluation::s) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = (Q.n - z.r10) + ny1 * ny1;
    delta_ops = r * r;
  } else if constexpr (eval == evaluation::a) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = (Q.n - z.r10) + ny1 * (ny1 + 3);
    delta_ops = 2 + z.r01 + r * (r + 2);
  } else if constexpr (eval == evaluation::c) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = 3 + z.r01 + 2 * (Q.n - r) + ny1 * ((ny1 + 3) / 2);
    delta_ops = 2 + z.r01 + ((r * (r + 3)) / 2);
  } else if constexpr (eval == evaluation::m) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = 5 + (Q.n - r) + ny1 * (ny1 + 3);
    delta_ops = 3 + 2 * z.r01 + r * (r + 2);
  } else if constexpr (eval == evaluation::ac) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = (Q.n - z.r10) + ny1 * (ny1 + 3) + 3 + z.r01 + 2 * (Q.n - r) + ny1 * ((ny1 + 3) / 2);
    delta_ops = 2 + z.r01 + r * (r + 2) + 2 + z.r01 + ((r * (r + 3)) / 2);
  } else if constexpr (eval == evaluation::am) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = (Q.n - z.r10) + ny1 * (ny1 + 3) + 5 + (Q.n - r) + ny1 * (ny1 + 3);
    delta_ops = 2 + z.r01 + r * (r + 2) + 3 + 2 * z.r01 + r * (r + 2);
  } else if constexpr (eval == evaluation::cm) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = 3 + z.r01 + 2 * (Q.n - r) + ny1 * ((ny1 + 3) / 2) + 5 + (Q.n - r) + ny1 * (ny1 + 3);
    delta_ops = 2 + z.r01 + z.r01 + ((r * (r + 3)) / 2) + 3 + 2 * z.r01 + r * (r + 2);
  } else if constexpr (eval == evaluation::acm) {
    size_t ny1 = (y.nx1 + z.r01) - z.r10;
    basic_ops = (Q.n - z.r10) + ny1 * (ny1 + 3) + 3 + z.r01 + 2 * (Q.n - r) +
                ny1 * ((ny1 + 3) / 2) + 5 + (Q.n - r) + ny1 * (ny1 + 3);
    delta_ops =
        2 + z.r01 + r * (r + 2) + 2 + z.r01 + ((r * (r + 3)) / 2) + 3 + 2 * z.r01 + r * (r + 2);
  }

  // Choose and execute algorithm
  if constexpr (eval == evaluation::basic) {
    if constexpr (count) basics++;
    evaluate_basic(Q, y, z, N);
  } else if constexpr (eval == evaluation::rflip_rv) {
    if constexpr (count) deltas++;
    update_rv_hybrid(Q, y);
    evaluate_rfliprv(Q, y, z);
  } else if (delta_ops < basic_ops) {
    if constexpr (count) deltas++;
    update_rv_hybrid(Q, y);
    evaluate_rfliprv(Q, y, z);
  } else {
    if constexpr (count) basics++;
    evaluate_basic(Q, y, z, N);
  }
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_EVALUATE_HYBRID_HPP
