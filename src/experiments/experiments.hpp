/**
 * @file experiments.hpp
 * @brief Main include file for all experiment types.
 *
 * This header includes all experiment modules and provides a unified
 * interface for running UBQP experiments.
 */

#ifndef QUBO_EXPERIMENTS_EXPERIMENTS_HPP
#define QUBO_EXPERIMENTS_EXPERIMENTS_HPP

// Common utilities
#include "common.hpp"

// Individual experiment types
#include "eval_experiment.hpp"
#include "ls_experiment.hpp"
#include "vns_experiment.hpp"

namespace qubo {

/**
 * @brief Get experiment function by evaluation strategy and type.
 * 
 * @param eval Evaluation strategy
 * @param exp_type Experiment type: "eval", "ls", "ls_count", "vns", "vns_count"
 * @return Experiment function
 */
inline std::function<json(const ubqp&, json)> get_experiment_fn(
    evaluation eval, 
    const std::string& exp_type) {
  
  if (exp_type == "eval") {
    for (const auto& [e, fn] : get_eval_experiments()) {
      if (e == eval) return fn;
    }
  } else if (exp_type == "ls") {
    for (const auto& [e, fn] : get_ls_time_experiments()) {
      if (e == eval) return fn;
    }
  } else if (exp_type == "ls_count") {
    for (const auto& [e, fn] : get_ls_count_experiments()) {
      if (e == eval) return fn;
    }
  } else if (exp_type == "vns" || exp_type == "vns_figures") {
    for (const auto& [e, fn] : get_vns_time_experiments()) {
      if (e == eval) return fn;
    }
  } else if (exp_type == "vns_count" || exp_type == "vns_figures_count") {
    for (const auto& [e, fn] : get_vns_count_experiments()) {
      if (e == eval) return fn;
    }
  } else if (exp_type == "vns_tables") {
    for (const auto& [e, fn] : get_vns_table_experiments()) {
      if (e == eval) return fn;
    }
  }
  
  throw std::runtime_error("Unknown experiment type or strategy: " + exp_type);
}

/**
 * @brief Get all experiment functions for a given type.
 */
inline auto get_all_experiments(const std::string& exp_type) {
  if (exp_type == "eval") return get_eval_experiments();
  if (exp_type == "ls") return get_ls_time_experiments();
  if (exp_type == "ls_count") return get_ls_count_experiments();
  if (exp_type == "vns" || exp_type == "vns_figures") return get_vns_time_experiments();
  if (exp_type == "vns_count" || exp_type == "vns_figures_count") return get_vns_count_experiments();
  if (exp_type == "vns_tables") return get_vns_table_experiments();
  
  throw std::runtime_error("Unknown experiment type: " + exp_type);
}

}  // namespace qubo

#endif  // QUBO_EXPERIMENTS_EXPERIMENTS_HPP
