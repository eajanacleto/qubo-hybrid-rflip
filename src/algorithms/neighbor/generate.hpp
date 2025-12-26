/**
 * @file generate.hpp
 * @brief Functions for generating neighbor solutions.
 */

#ifndef QUBO_ALGORITHMS_NEIGHBOR_GENERATE_HPP
#define QUBO_ALGORITHMS_NEIGHBOR_GENERATE_HPP

#include <algorithm>
#include <memory>
#include <random>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"

namespace qubo {

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

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_NEIGHBOR_GENERATE_HPP
