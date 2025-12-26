/**
 * @file experiments.hpp
 * @brief Experiment functions for UBQP algorithm evaluation.
 *
 * Contains functions for running various experiments comparing
 * evaluation strategies in local search and VNS algorithms.
 */

#ifndef QUBO_EXPERIMENTS_EXPERIMENTS_HPP
#define QUBO_EXPERIMENTS_EXPERIMENTS_HPP

#include <algorithm>
#include <functional>
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
#include "../algorithms/vns.hpp"
#include "../json.hpp"

namespace qubo {

using nlohmann::json;

/**
 * @brief Performs an experiment measuring evaluation strategy processing time.
 *
 * Measures the time required to evaluate neighbors using different strategies.
 *
 * @tparam eval Evaluation strategy to test
 * @param Q UBQP instance
 * @param params JSON parameters: r (neighborhood size), n1 (ones count), iters (iterations)
 * @return JSON with max, min, avg times
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

    // Compute objective
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

    // Reverse the move
    std::swap(y.R01, y.R10);
    std::swap(y.r01, y.r10);

    if constexpr (eval != evaluation::basic) {
      build_rv(Q, x);
      x.wx01 = x.wx10 = 0;
    }

    // Measure evaluation time
    long dt = measure([&]() { evaluate_hybrid<eval, false>(Q, x, y, N, basics, deltas); });
    sum += dt;
    if (dt > maximum) maximum = dt;
    if (dt < minimum) minimum = dt;
  }

  return {{"max", maximum}, {"min", minimum}, {"avg", sum / iters}};
}

/**
 * @brief Performs a local search experiment.
 *
 * Measures processing time or algorithm choice counts for local search.
 *
 * @tparam eval Evaluation strategy to use
 * @tparam count If true, count algorithm choices; if false, measure time
 * @param Q UBQP instance
 * @param params JSON parameters: r (neighborhood size), iters (max non-improving iterations)
 * @return JSON with either {basics, deltas} or {dt, fx}
 */
template <evaluation eval, bool count>
json ls_experiment(const ubqp& Q, const json params) {
  std::mt19937 rng;
  size_t r = params["r"];
  size_t iters = params["iters"];
  size_t basics = 0, deltas = 0;

  incumbent_solution<eval> y(Q.n);
  randomize(Q, y, rng);
  neighbor_solution z(Q.n);

  std::unique_ptr<size_t[]> N(new size_t[Q.n]());
  std::iota(&N[0], &N[Q.n], 0);

  if constexpr (count) {
    ls<eval, count>(Q, y, z, r, iters, N, rng, basics, deltas);
    return {{"basics", basics}, {"deltas", deltas}};
  } else {
    long dt = measure([&]() { ls<eval, count>(Q, y, z, r, iters, N, rng, basics, deltas); });
    return {{"dt", dt}, {"fx", y.fy}};
  }
}

/**
 * @brief Performs a Variable Neighborhood Search experiment.
 *
 * Measures processing time or algorithm choice counts for VNS.
 *
 * @tparam eval Evaluation strategy to use
 * @tparam count If true, count algorithm choices; if false, measure time
 * @param Q UBQP instance
 * @param params JSON parameters: iters, ls_iters, r_max, r_step
 * @return JSON with either {basics, deltas} or {dt, fx}
 */
template <evaluation eval, bool count>
json vns_experiment(const ubqp& Q, const json params) {
  std::mt19937 rng;
  size_t iters = params["iters"];
  size_t ls_iters = params["ls_iters"];
  size_t r_max = params["r_max"];
  size_t r_step = params["r_step"];
  size_t basics = 0, deltas = 0;

  incumbent_solution<eval> y_inc(Q.n), y(Q.n);
  randomize(Q, y_inc, rng);
  neighbor_solution z(Q.n), z2(Q.n);

  std::unique_ptr<size_t[]> N(new size_t[Q.n]());
  std::iota(&N[0], &N[Q.n], 0);

  if constexpr (count) {
    vns<eval, count>(Q, y_inc, y, z, z2, r_max, r_step, iters, ls_iters, N, rng, basics, deltas);
    return {{"basics", basics}, {"deltas", deltas}};
  } else {
    long dt = measure([&]() {
      vns<eval, count>(Q, y_inc, y, z, z2, r_max, r_step, iters, ls_iters, N, rng, basics, deltas);
    });
    return {{"dt", dt}, {"fx", y_inc.fy}};
  }
}

}  // namespace qubo

#endif  // QUBO_EXPERIMENTS_EXPERIMENTS_HPP
