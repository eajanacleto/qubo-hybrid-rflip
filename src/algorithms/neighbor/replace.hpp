/**
 * @file replace.hpp
 * @brief Functions for replacing incumbent solutions.
 */

#ifndef QUBO_ALGORITHMS_NEIGHBOR_REPLACE_HPP
#define QUBO_ALGORITHMS_NEIGHBOR_REPLACE_HPP

#include <algorithm>
#include <cassert>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../evaluate/verify.hpp"

namespace qubo {

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

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_NEIGHBOR_REPLACE_HPP
