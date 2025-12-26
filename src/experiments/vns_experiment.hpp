/**
 * @file vns_experiment.hpp
 * @brief Variable Neighborhood Search experiment functions.
 *
 * Measures processing time and algorithm choices for VNS metaheuristic.
 */

#ifndef QUBO_EXPERIMENTS_VNS_EXPERIMENT_HPP
#define QUBO_EXPERIMENTS_VNS_EXPERIMENT_HPP

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
#include "../algorithms/vns.hpp"
#include "../json.hpp"

namespace qubo {

/**
 * @brief Performs a Variable Neighborhood Search experiment.
 *
 * Measures processing time or algorithm choice counts for VNS.
 *
 * @tparam eval Evaluation strategy to use
 * @tparam count If true, count algorithm choices; if false, measure time
 * @param Q UBQP instance
 * @param params JSON parameters:
 *   - iters: number of VNS iterations
 *   - ls_iters: maximum non-improving iterations in local search
 *   - r_max: maximum neighborhood size
 *   - r_step: neighborhood size increment
 * @return JSON with either {basics, deltas} (if count=true) or {dt, fx} (if count=false)
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

/**
 * @brief Get all VNS time experiment functions.
 */
inline auto get_vns_time_experiments() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::basic, vns_experiment<evaluation::basic, false>},
    {evaluation::rflip_rv, vns_experiment<evaluation::rflip_rv, false>},
    {evaluation::s, vns_experiment<evaluation::s, false>},
    {evaluation::a, vns_experiment<evaluation::a, false>},
    {evaluation::c, vns_experiment<evaluation::c, false>},
    {evaluation::m, vns_experiment<evaluation::m, false>},
    {evaluation::ac, vns_experiment<evaluation::ac, false>},
    {evaluation::am, vns_experiment<evaluation::am, false>},
    {evaluation::cm, vns_experiment<evaluation::cm, false>},
    {evaluation::acm, vns_experiment<evaluation::acm, false>},
  };
}

/**
 * @brief Get all VNS counting experiment functions.
 */
inline auto get_vns_count_experiments() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::s, vns_experiment<evaluation::s, true>},
    {evaluation::a, vns_experiment<evaluation::a, true>},
    {evaluation::c, vns_experiment<evaluation::c, true>},
    {evaluation::m, vns_experiment<evaluation::m, true>},
    {evaluation::ac, vns_experiment<evaluation::ac, true>},
    {evaluation::am, vns_experiment<evaluation::am, true>},
    {evaluation::cm, vns_experiment<evaluation::cm, true>},
    {evaluation::acm, vns_experiment<evaluation::acm, true>},
  };
}

/**
 * @brief Get VNS experiments for table generation (subset of strategies).
 */
inline auto get_vns_table_experiments() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::basic, vns_experiment<evaluation::basic, false>},
    {evaluation::rflip_rv, vns_experiment<evaluation::rflip_rv, false>},
    {evaluation::s, vns_experiment<evaluation::s, false>},
  };
}

}  // namespace qubo

#endif  // QUBO_EXPERIMENTS_VNS_EXPERIMENT_HPP
