/**
 * @file randomize.hpp
 * @brief Solution randomization for local search initialization.
 *
 * Provides functions to create random initial solutions.
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_RANDOMIZE_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_RANDOMIZE_HPP

#include <random>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"

namespace qubo {

/**
 * @brief Randomizes an incumbent solution and computes its reevaluation vector.
 *
 * Creates a random binary solution where each bit has 50% probability of being 1.
 * Also computes the objective function value and reevaluation vector.
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

  // Compute objective function: f(y) = sum_{i,j} Q[i][j] * y[i] * y[j]
  for (size_t i = 0; i < Q.n; i++) {
    for (size_t j = 0; j <= i; j++) {
      if (y.y[i] && y.y[j]) y.fy += Q[i][j];
    }
  }

  // Build reevaluation vector if needed: dx[i] = sum_j Q[i][j] * y[j]
  if constexpr (eval != evaluation::basic) {
    for (size_t i = 0; i < Q.n; i++) {
      for (size_t j = 0; j < Q.n; j++) {
        if (y.y[j]) y.dx[i] += Q[i][j];
      }
    }
  }
}

/**
 * @brief Randomizes a solution with a specified number of ones.
 *
 * Creates a random solution with exactly n1 bits set to 1.
 *
 * @tparam eval Evaluation strategy
 * @param Q UBQP instance
 * @param y Incumbent solution (output: randomized)
 * @param n1 Number of ones in the solution
 * @param N Permutation array workspace (size n)
 * @param rng Random number generator
 */
template <evaluation eval>
void randomize_with_density(const ubqp& Q, incumbent_solution<eval>& y, 
                            size_t n1, std::unique_ptr<size_t[]>& N,
                            std::mt19937& rng) {
  // Reset solution
  for (size_t i = 0; i < Q.n; i++) {
    y.y[i] = 0;
    if constexpr (eval != evaluation::basic) {
      y.x[i] = 0;
      y.dx[i] = 0;
    }
  }
  y.fy = 0;
  if constexpr (eval != evaluation::basic) {
    y.nx1 = 0;
    y.wx01 = 0;
    y.wx10 = 0;
  }

  // Create permutation and select first n1 elements
  std::iota(&N[0], &N[Q.n], 0);
  for (size_t m = 0; m < n1; m++) {
    std::swap(N[m], N[m + (rng() % (Q.n - m))]);
    y.y[N[m]] = 1;
    if constexpr (eval != evaluation::basic) {
      y.x[N[m]] = 1;
      y.nx1++;
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

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_LOCAL_SEARCH_RANDOMIZE_HPP
