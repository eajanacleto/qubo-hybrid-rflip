/**
 * @file update.hpp
 * @brief Functions to update the reevaluation vector incrementally.
 */

#ifndef QUBO_ALGORITHMS_RV_UPDATE_HPP
#define QUBO_ALGORITHMS_RV_UPDATE_HPP

#include <cassert>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "../evaluate/verify.hpp"

namespace qubo {

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

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_RV_UPDATE_HPP
