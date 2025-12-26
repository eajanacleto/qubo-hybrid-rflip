/**
 * @file ls_experiment.hpp
 * @brief Local Search experiment functions.
 *
 * Measures processing time and algorithm choices for stochastic
 * local search with r-flip neighborhoods.
 * 
 * Supports multiple local search strategies:
 * - first_improvement: Accept first improving neighbor
 * - best_improvement: Evaluate all neighbors, accept best
 * 
 * To add a new strategy:
 * 1. Create the strategy in src/algorithms/local_search/
 * 2. Add to ls_strategy enum in ls_strategy.hpp
 * 3. Add to get_all_ls_strategies() in local_search.hpp
 * 4. The strategy will be automatically included in experiments
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
 * @brief Performs a local search experiment with a specific LS strategy.
 *
 * Measures processing time or algorithm choice counts for local search.
 *
 * @tparam eval Evaluation strategy to use
 * @tparam count If true, count algorithm choices; if false, measure time
 * @tparam strategy Local search strategy (first_improvement, best_improvement, etc.)
 * @param Q UBQP instance
 * @param params JSON parameters:
 *   - r: neighborhood size (number of bits to flip)
 *   - iters: maximum number of non-improving iterations
 * @return JSON with either {basics, deltas} (if count=true) or {dt, fx} (if count=false)
 */
template <evaluation eval, bool count, ls_strategy strategy>
json ls_experiment_with_strategy(const ubqp& Q, const json params) {
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
    ls_dispatch<eval, count>(strategy, Q, y, z, r, iters, N, rng, basics, deltas);
    return {
      {"basics", basics}, 
      {"deltas", deltas},
      {"ls_strategy", static_cast<int>(strategy)}
    };
  } else {
    long dt = measure([&]() { 
      ls_dispatch<eval, count>(strategy, Q, y, z, r, iters, N, rng, basics, deltas); 
    });
    return {
      {"dt", dt}, 
      {"fx", y.fy},
      {"ls_strategy", static_cast<int>(strategy)}
    };
  }
}

/**
 * @brief Original local search experiment (uses first_improvement for backward compatibility).
 */
template <evaluation eval, bool count>
json ls_experiment(const ubqp& Q, const json params) {
  return ls_experiment_with_strategy<eval, count, ls_strategy::first_improvement>(Q, params);
}

/**
 * @brief Get all local search time experiment functions for a specific LS strategy.
 */
template <ls_strategy strategy>
auto get_ls_time_experiments_for_strategy() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::basic, ls_experiment_with_strategy<evaluation::basic, false, strategy>},
    {evaluation::rflip_rv, ls_experiment_with_strategy<evaluation::rflip_rv, false, strategy>},
    {evaluation::s, ls_experiment_with_strategy<evaluation::s, false, strategy>},
    {evaluation::a, ls_experiment_with_strategy<evaluation::a, false, strategy>},
    {evaluation::c, ls_experiment_with_strategy<evaluation::c, false, strategy>},
    {evaluation::m, ls_experiment_with_strategy<evaluation::m, false, strategy>},
    {evaluation::ac, ls_experiment_with_strategy<evaluation::ac, false, strategy>},
    {evaluation::am, ls_experiment_with_strategy<evaluation::am, false, strategy>},
    {evaluation::cm, ls_experiment_with_strategy<evaluation::cm, false, strategy>},
    {evaluation::acm, ls_experiment_with_strategy<evaluation::acm, false, strategy>},
  };
}

/**
 * @brief Get all local search counting experiment functions for a specific LS strategy.
 */
template <ls_strategy strategy>
auto get_ls_count_experiments_for_strategy() {
  return std::vector<std::pair<evaluation, std::function<json(const ubqp&, json)>>>{
    {evaluation::s, ls_experiment_with_strategy<evaluation::s, true, strategy>},
    {evaluation::a, ls_experiment_with_strategy<evaluation::a, true, strategy>},
    {evaluation::c, ls_experiment_with_strategy<evaluation::c, true, strategy>},
    {evaluation::m, ls_experiment_with_strategy<evaluation::m, true, strategy>},
    {evaluation::ac, ls_experiment_with_strategy<evaluation::ac, true, strategy>},
    {evaluation::am, ls_experiment_with_strategy<evaluation::am, true, strategy>},
    {evaluation::cm, ls_experiment_with_strategy<evaluation::cm, true, strategy>},
    {evaluation::acm, ls_experiment_with_strategy<evaluation::acm, true, strategy>},
  };
}

/**
 * @brief Get all local search time experiment functions (first_improvement - backward compatible).
 */
inline auto get_ls_time_experiments() {
  return get_ls_time_experiments_for_strategy<ls_strategy::first_improvement>();
}

/**
 * @brief Get all local search counting experiment functions (first_improvement - backward compatible).
 */
inline auto get_ls_count_experiments() {
  return get_ls_count_experiments_for_strategy<ls_strategy::first_improvement>();
}

/**
 * @brief Get time experiments for ALL local search strategies.
 * 
 * Returns a map: ls_strategy -> vector of (evaluation, experiment_fn)
 */
inline auto get_all_ls_time_experiments() {
  std::vector<std::tuple<ls_strategy, evaluation, std::function<json(const ubqp&, json)>>> result;
  
  // First improvement
  for (const auto& [eval, fn] : get_ls_time_experiments_for_strategy<ls_strategy::first_improvement>()) {
    result.emplace_back(ls_strategy::first_improvement, eval, fn);
  }
  
  // Best improvement
  for (const auto& [eval, fn] : get_ls_time_experiments_for_strategy<ls_strategy::best_improvement>()) {
    result.emplace_back(ls_strategy::best_improvement, eval, fn);
  }
  
  // Add new strategies here as they are implemented
  
  return result;
}

/**
 * @brief Get count experiments for ALL local search strategies.
 */
inline auto get_all_ls_count_experiments() {
  std::vector<std::tuple<ls_strategy, evaluation, std::function<json(const ubqp&, json)>>> result;
  
  // First improvement
  for (const auto& [eval, fn] : get_ls_count_experiments_for_strategy<ls_strategy::first_improvement>()) {
    result.emplace_back(ls_strategy::first_improvement, eval, fn);
  }
  
  // Best improvement
  for (const auto& [eval, fn] : get_ls_count_experiments_for_strategy<ls_strategy::best_improvement>()) {
    result.emplace_back(ls_strategy::best_improvement, eval, fn);
  }
  
  // Add new strategies here as they are implemented
  
  return result;
}

}  // namespace qubo

#endif  // QUBO_EXPERIMENTS_LS_EXPERIMENT_HPP
