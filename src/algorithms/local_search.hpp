/**
 * @file local_search.hpp
 * @brief Local Search algorithm for UBQP problems.
 *
 * Implements a stochastic local search using r-flip neighborhood.
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_HPP

#include <algorithm>
#include <memory>
#include <random>

#include "../core/evaluation.hpp"
#include "../core/solution.hpp"
#include "../core/ubqp.hpp"
#include "evaluate.hpp"

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
 * @brief Generates a random r-flip neighbor of the incumbent.
 *
 * @tparam eval Evaluation strategy
 * @param Q UBQP instance
 * @param y Incumbent solution
 * @param z Neighbor solution (output)
 * @param r Number of bits to flip
 * @param N Permutation array (modified)
 * @param rng Random number generator
 */
template <evaluation eval>
void random_neighbor_solution(const ubqp& Q, const incumbent_solution<eval>& y,
                              neighbor_solution& z, size_t r, std::unique_ptr<size_t[]>& N,
                              std::mt19937& rng) {
  z.r01 = z.r10 = 0;
  if constexpr (eval != evaluation::basic) z.n1z = y.nx1;

  // Select r random positions using Fisher-Yates shuffle prefix
  for (size_t m = 0; m < r; m++) {
    std::swap(N[m], N[m + (rng() % (Q.n - m))]);

    if (!y.y[N[m]]) {
      z.R01[z.r01++] = N[m];  // 0 → 1 flip
      if constexpr (eval != evaluation::basic) z.n1z++;
    } else {
      z.R10[z.r10++] = N[m];  // 1 → 0 flip
      if constexpr (eval != evaluation::basic) z.n1z--;
    }
  }
}

/**
 * @brief Replaces the incumbent solution with a neighbor.
 *
 * @tparam eval Evaluation strategy
 * @param y Incumbent solution (output: replaced)
 * @param z Neighbor solution
 */
template <evaluation eval>
void replace_incumbent(incumbent_solution<eval>& y, const neighbor_solution& z) {
  y.fy = z.fz;

  // Apply flips
  for (size_t k = 0; k < z.r01; k++) y.y[z.R01[k]] = 1;
  for (size_t k = 0; k < z.r10; k++) y.y[z.R10[k]] = 0;

  // Update change tracking if needed
  if constexpr (eval != evaluation::basic) {
    y.nx1 = z.n1z;

    // Remove from WX01 any bits that are now 0
    size_t k = 0;
    while (k < y.wx01) {
      size_t i = y.WX01[k];
      if (y.y[i] == 0)
        std::swap(y.WX01[k], y.WX01[--y.wx01]);
      else
        k++;
    }

    // Remove from WX10 any bits that are now 1
    k = 0;
    while (k < y.wx10) {
      size_t i = y.WX10[k];
      if (y.y[i] == 1)
        std::swap(y.WX10[k], y.WX10[--y.wx10]);
      else
        k++;
    }

    // Add new changes
    for (size_t k = 0; k < z.r01; k++) {
      size_t i = z.R01[k];
      if (y.x[i] == 0) y.WX01[y.wx01++] = i;
    }
    for (size_t k = 0; k < z.r10; k++) {
      size_t i = z.R10[k];
      if (y.x[i] == 1) y.WX10[y.wx10++] = i;
    }

    assert(check_sets(y));
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
