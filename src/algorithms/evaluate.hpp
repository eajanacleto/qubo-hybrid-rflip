/**
 * @file evaluate.hpp
 * @brief Evaluation algorithms for UBQP neighbor solutions.
 *
 * Implements the basic evaluation, r-flip-rv evaluation, and hybrid
 * evaluation strategies for computing objective function values.
 */

#ifndef QUBO_ALGORITHMS_EVALUATE_HPP
#define QUBO_ALGORITHMS_EVALUATE_HPP

#include <cassert>
#include <memory>

#include "../core/evaluation.hpp"
#include "../core/solution.hpp"
#include "../core/ubqp.hpp"

namespace qubo {

// =============================================================================
// Verification Functions (Debug)
// =============================================================================

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

// =============================================================================
// Basic Evaluation Algorithm
// =============================================================================

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

// =============================================================================
// Reevaluation Vector Functions
// =============================================================================

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

/**
 * @brief Updates the reevaluation vector incrementally.
 *
 * Uses the change sets WX01 and WX10 to update dx.
 * Time complexity: O(n × r) where r = |WX01| + |WX10|
 *
 * @tparam eval Evaluation strategy (must not be basic)
 * @param Q UBQP instance
 * @param y Incumbent solution (output: dx updated)
 */
template <evaluation eval>
void update_rv(const ubqp& Q, incumbent_solution<eval>& y) {
  static_assert(eval != evaluation::basic);

  for (size_t i = 0; i < Q.n; i++) {
    for (size_t k = 0; k < y.wx01; k++) {
      size_t j = y.WX01[k];
      y.dx[i] += Q[i][j];
    }
    for (size_t k = 0; k < y.wx10; k++) {
      size_t j = y.WX10[k];
      y.dx[i] -= Q[i][j];
    }
  }

  assert(check_rv(Q, y));
}

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

// =============================================================================
// R-Flip-RV Evaluation Algorithm
// =============================================================================

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

// =============================================================================
// Hybrid Evaluation
// =============================================================================

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

#endif  // QUBO_ALGORITHMS_EVALUATE_HPP
