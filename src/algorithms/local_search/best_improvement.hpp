/**
 * @file best_improvement.hpp
 * @brief Best-improvement local search strategy.
 *
 * This strategy evaluates multiple neighbors in the r-flip neighborhood
 * and selects the best improving one. More thorough but slower
 * per iteration than first-improvement.
 */

#ifndef QUBO_ALGORITHMS_LOCAL_SEARCH_BEST_IMPROVEMENT_HPP
#define QUBO_ALGORITHMS_LOCAL_SEARCH_BEST_IMPROVEMENT_HPP

#include <algorithm>
#include <limits>
#include <memory>
#include <numeric>
#include <random>

#include "../../core/evaluation.hpp"
#include "../../core/solution.hpp"
#include "../../core/ubqp.hpp"
#include "../evaluate/hybrid.hpp"
#include "../neighbor/generate.hpp"
#include "../neighbor/replace.hpp"

namespace qubo {

/**
 * @brief Best-improvement local search.
 *
 * Evaluates multiple neighbors and accepts the best improving move.
 * Uses a configurable number of candidates per iteration.
 *
 * This implementation evaluates `num_candidates` random neighbors
 * and selects the best one. When num_candidates=1, it behaves like
 * first-improvement; higher values make it more thorough but slower.
 *
 * @tparam eval Evaluation strategy to use
 * @tparam count If true, count basic vs delta evaluations
 * @param Q UBQP instance
 * @param y Current incumbent solution (modified in place)
 * @param z Temporary neighbor solution
 * @param r Neighborhood size (number of bits to flip)
 * @param iters Maximum number of non-improving iterations
 * @param N Permutation array (modified in place)
 * @param rng Random number generator
 * @param basics [out] Count of basic evaluations (if count=true)
 * @param deltas [out] Count of delta evaluations (if count=true)
 */
template <evaluation eval, bool count = false>
void ls_best_improvement(
    const ubqp& Q,
    incumbent_solution<eval>& y,
    neighbor_solution& z,
    size_t r,
    size_t iters,
    std::unique_ptr<size_t[]>& N,
    std::mt19937& rng,
    size_t& basics,
    size_t& deltas) {
  
  size_t n = Q.n;
  
  // Number of candidates to evaluate per iteration
  // More candidates = more thorough but slower
  const size_t num_candidates = std::min(n, size_t(10));
  
  // Storage for best candidate
  neighbor_solution best_z(n);
  
  for (size_t l = 1; l <= iters; l++) {
    long best_fz = y.fy;  // Looking for fz < y.fy (improvement)
    bool found_improving = false;
    
    // Store best candidate data
    size_t best_n1z = 0, best_r01 = 0, best_r10 = 0;
    std::unique_ptr<size_t[]> best_N1z(new size_t[n]());
    std::unique_ptr<size_t[]> best_R01(new size_t[n]());
    std::unique_ptr<size_t[]> best_R10(new size_t[n]());
    
    // Evaluate multiple random neighbors
    for (size_t c = 0; c < num_candidates; ++c) {
      random_neighbor_solution(Q, y, z, r, N, rng);
      evaluate_hybrid<eval, count>(Q, y, z, N, basics, deltas);
      
      if (z.fz < best_fz) {
        best_fz = z.fz;
        // Save the best neighbor data
        best_n1z = z.n1z;
        best_r01 = z.r01;
        best_r10 = z.r10;
        std::copy(&z.N1z[0], &z.N1z[z.n1z], &best_N1z[0]);
        std::copy(&z.R01[0], &z.R01[z.r01], &best_R01[0]);
        std::copy(&z.R10[0], &z.R10[z.r10], &best_R10[0]);
        found_improving = true;
      }
    }
    
    if (found_improving) {
      // Restore best neighbor into z
      z.fz = best_fz;
      z.n1z = best_n1z;
      z.r01 = best_r01;
      z.r10 = best_r10;
      std::copy(&best_N1z[0], &best_N1z[best_n1z], &z.N1z[0]);
      std::copy(&best_R01[0], &best_R01[best_r01], &z.R01[0]);
      std::copy(&best_R10[0], &best_R10[best_r10], &z.R10[0]);
      
      replace_incumbent(y, z);
      l = 0;  // Reset counter on improvement
    }
  }
}

/**
 * @brief Backward-compatible alias for best-improvement local search.
 */
template <evaluation eval, bool count = false>
void ls_bi(
    const ubqp& Q,
    incumbent_solution<eval>& y,
    neighbor_solution& z,
    size_t r,
    size_t iters,
    std::unique_ptr<size_t[]>& N,
    std::mt19937& rng,
    size_t& basics,
    size_t& deltas) {
  ls_best_improvement<eval, count>(Q, y, z, r, iters, N, rng, basics, deltas);
}

}  // namespace qubo

#endif  // QUBO_ALGORITHMS_LOCAL_SEARCH_BEST_IMPROVEMENT_HPP
