/**
 * @file eval_experiment.hpp
 * @brief Evaluation time comparison experiment.
 *
 * Measures the processing time for different evaluation strategies
 * when evaluating neighbor solutions.
 */

#ifndef QUBO_EXPERIMENTS_EVAL_EXPERIMENT_HPP
#define QUBO_EXPERIMENTS_EVAL_EXPERIMENT_HPP

#include <algorithm>
#include <limits>
#include <memory>
#include <numeric>
#include <random>

#include "../core/evaluation.hpp"
#include "../core/solution.hpp"
#include "../core/ubqp.hpp"
#include "../core/utils.hpp"
#include "../algorithms/evaluate.hpp"
#include "../algorithms/local_search.hpp"
#include "../json.hpp"

namespace qubo {

/**
 * @brief Performs an experiment measuring evaluation strategy processing time.
 *
 * Measures the time required to evaluate neighbors using different strategies.
 * The experiment:
 * 1. Creates a random solution with n1 ones
 * 2. Generates a random r-flip neighbor
 * 3. Evaluates the neighbor and measures time
 *
 * @tparam eval Evaluation strategy to test
 * @param Q UBQP instance
 * @param params JSON parameters:
 *   - r: neighborhood size (number of bits to flip)
 *   - n1: number of ones in the solution
 *   - iters: number of iterations for averaging
 * @return JSON with {max, min, avg} times in nanoseconds
 */
template <evaluation eval>
json eval_experiment(const ubqp& Q, const json params) {
  std::mt19937 rng;
  size_t r = params["r"];
  size_t n1 = params["n1"];
  size_t iters = params["iters"];
  size_t basics, deltas;
  long maximum = 0, minimum = std::numeric_limits<long>::max(), sum = 0;

  std::unique_ptr<size_t[]> N(new size_t[Q.n]());
  std::iota(&N[0], &N[Q.n], 0);

  incumbent_solution<eval> x(Q.n);
  randomize(Q, x, rng);
  neighbor_solution y(Q.n);

  for (size_t _ = 0; _ < iters; _++) {
    // Setup solution with n1 ones
    for (size_t m = 0; m < n1; m++) {
      std::swap(N[m], N[m + (rng() % (Q.n - m))]);
      x.y[N[m]] = 1;
    }
    for (size_t m = n1; m < Q.n; m++) x.y[N[m]] = 0;

    // Compute objective function from scratch
    x.fy = 0;
    for (size_t i = 0; i < Q.n; i++) {
      for (size_t j = 0; j <= i; j++) {
        if (x.y[i] && x.y[j]) x.fy += Q[i][j];
      }
    }

    if constexpr (eval != evaluation::basic) {
      build_rv(Q, x);
      x.wx01 = x.wx10 = 0;
    }

    // Generate and evaluate neighbor
    random_neighbor_solution(Q, x, y, r, N, rng);
    evaluate_hybrid<eval, false>(Q, x, y, N, basics, deltas);
    replace_incumbent(x, y);

    // Reverse the move for next iteration
    std::swap(y.R01, y.R10);
    std::swap(y.r01, y.r10);

    if constexpr (eval != evaluation::basic) {
      build_rv(Q, x);
      x.wx01 = x.wx10 = 0;
    }

    // Measure evaluation time
    long dt = measure([&]() { 
      evaluate_hybrid<eval, false>(Q, x, y, N, basics, deltas); 
    });
    
    sum += dt;
    if (dt > maximum) maximum = dt;
    if (dt < minimum) minimum = dt;
  }

  return {{"max", maximum}, {"min", minimum}, {"avg", sum / iters}};
}

/**
 * @brief Get all evaluation experiment functions.
 */
inline auto get_eval_experiments() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::basic, eval_experiment<evaluation::basic>},
    {evaluation::rflip_rv, eval_experiment<evaluation::rflip_rv>},
    {evaluation::s, eval_experiment<evaluation::s>},
    {evaluation::a, eval_experiment<evaluation::a>},
    {evaluation::c, eval_experiment<evaluation::c>},
    {evaluation::m, eval_experiment<evaluation::m>},
    {evaluation::ac, eval_experiment<evaluation::ac>},
    {evaluation::am, eval_experiment<evaluation::am>},
    {evaluation::cm, eval_experiment<evaluation::cm>},
    {evaluation::acm, eval_experiment<evaluation::acm>},
  };
}

}  // namespace qubo

#endif  // QUBO_EXPERIMENTS_EVAL_EXPERIMENT_HPP
