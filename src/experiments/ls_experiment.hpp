/**
 * @file ls_experiment.hpp
 * @brief Local Search experiment functions.
 *
 * Measures processing time and algorithm choices for stochastic
 * local search with r-flip neighborhoods.
 */

#ifndef QUBO_EXPERIMENTS_LS_EXPERIMENT_HPP
#define QUBO_EXPERIMENTS_LS_EXPERIMENT_HPP

#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "../core/evaluation.hpp"
#include "../core/solution.hpp"
#include "../core/ubqp.hpp"
#include "../core/utils.hpp"
#include "../algorithms/local_search.hpp"
#include "../json.hpp"

namespace qubo {

/**
 * @brief Performs a local search experiment.
 *
 * Measures processing time or algorithm choice counts for local search.
 *
 * @tparam eval Evaluation strategy to use
 * @tparam count If true, count algorithm choices; if false, measure time
 * @param Q UBQP instance
 * @param params JSON parameters:
 *   - r: neighborhood size (number of bits to flip)
 *   - iters: maximum number of non-improving iterations
 * @return JSON with either {basics, deltas} (if count=true) or {dt, fx} (if count=false)
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
    long dt = measure([&]() { 
      ls<eval, count>(Q, y, z, r, iters, N, rng, basics, deltas); 
    });
    return {{"dt", dt}, {"fx", y.fy}};
  }
}

/**
 * @brief Get all local search time experiment functions.
 */
inline auto get_ls_time_experiments() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::basic, ls_experiment<evaluation::basic, false>},
    {evaluation::rflip_rv, ls_experiment<evaluation::rflip_rv, false>},
    {evaluation::s, ls_experiment<evaluation::s, false>},
    {evaluation::a, ls_experiment<evaluation::a, false>},
    {evaluation::c, ls_experiment<evaluation::c, false>},
    {evaluation::m, ls_experiment<evaluation::m, false>},
    {evaluation::ac, ls_experiment<evaluation::ac, false>},
    {evaluation::am, ls_experiment<evaluation::am, false>},
    {evaluation::cm, ls_experiment<evaluation::cm, false>},
    {evaluation::acm, ls_experiment<evaluation::acm, false>},
  };
}

/**
 * @brief Get all local search counting experiment functions.
 */
inline auto get_ls_count_experiments() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::s, ls_experiment<evaluation::s, true>},
    {evaluation::a, ls_experiment<evaluation::a, true>},
    {evaluation::c, ls_experiment<evaluation::c, true>},
    {evaluation::m, ls_experiment<evaluation::m, true>},
    {evaluation::ac, ls_experiment<evaluation::ac, true>},
    {evaluation::am, ls_experiment<evaluation::am, true>},
    {evaluation::cm, ls_experiment<evaluation::cm, true>},
    {evaluation::acm, ls_experiment<evaluation::acm, true>},
  };
}

}  // namespace qubo

#endif  // QUBO_EXPERIMENTS_LS_EXPERIMENT_HPP
