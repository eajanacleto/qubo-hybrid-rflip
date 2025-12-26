/**
 * @file common.hpp
 * @brief Common utilities and types for experiments.
 *
 * Contains shared utilities used across all experiment types.
 */

#ifndef QUBO_EXPERIMENTS_COMMON_HPP
#define QUBO_EXPERIMENTS_COMMON_HPP

#include <algorithm>
#include <functional>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../core/ubqp.hpp"
#include "../core/utils.hpp"
#include "../json.hpp"

namespace qubo {

using nlohmann::json;

/**
 * @brief Experiment runner that handles caching and result storage.
 */
class ExperimentRunner {
 public:
  explicit ExperimentRunner(const std::string& results_filename)
      : results_filename_(results_filename) {
    // Load existing results if any
    std::ifstream file(results_filename, std::ifstream::in);
    if (file) {
      file >> results_;
    }
  }

  /**
   * @brief Run an experiment if not already computed.
   * 
   * @param exp Experiment function
   * @param Q UBQP instance
   * @param params Experiment parameters
   */
  void run(std::function<json(const ubqp&, json)> exp, 
           const ubqp& Q, 
           json params) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (const auto& j : results_) {
        if (j["params"] == params) return;
      }
      std::cout << "<<< " << params << std::endl;
    }

    json result = exp(Q, params);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      std::cout << ">>> " << result << std::endl;
      results_.push_back({{"params", params}, {"result", result}});
      save();
    }
  }

  /**
   * @brief Save results to file.
   */
  void save() const {
    std::ofstream file(results_filename_, std::ofstream::out);
    file << std::setw(1) << results_;
  }

  /**
   * @brief Get the results JSON.
   */
  const json& results() const { return results_; }

 private:
  std::string results_filename_;
  json results_;
  std::mutex mutex_;
};

/**
 * @brief Load experiment configuration from JSON file.
 */
inline json load_config(const std::string& config_path) {
  std::ifstream file(config_path);
  if (!file) {
    throw std::runtime_error("Cannot open config file: " + config_path);
  }
  json config;
  file >> config;
  return config;
}

/**
 * @brief Get instance list from configuration.
 * 
 * Handles both direct instance lists and composed sets.
 */
inline std::vector<std::string> get_instances(
    const json& instances_config, 
    const std::string& set_name) {
  
  std::vector<std::string> result;
  const auto& sets = instances_config["instance_sets"];
  
  if (!sets.contains(set_name)) {
    throw std::runtime_error("Unknown instance set: " + set_name);
  }
  
  const auto& set = sets[set_name];
  
  if (set.contains("compose")) {
    // Composed set: recursively collect instances
    for (const auto& sub_set : set["compose"]) {
      auto sub_instances = get_instances(instances_config, sub_set);
      result.insert(result.end(), sub_instances.begin(), sub_instances.end());
    }
  } else if (set.contains("instances")) {
    // Direct instance list
    for (const auto& inst : set["instances"]) {
      result.push_back(inst);
    }
  }
  
  return result;
}

/**
 * @brief Strategy enumeration mapping from string.
 */
inline std::vector<evaluation> parse_strategies(
    const json& exp_config,
    const json& strategy_sets) {
  
  std::vector<evaluation> strategies;
  const auto& strat_list = exp_config["evaluation_strategies"];
  
  std::vector<std::string> strategy_names;
  
  for (const auto& s : strat_list) {
    std::string name = s;
    if (strategy_sets.contains(name)) {
      for (const auto& n : strategy_sets[name]) {
        strategy_names.push_back(n);
      }
    } else {
      strategy_names.push_back(name);
    }
  }
  
  for (const auto& name : strategy_names) {
    if (name == "basic") strategies.push_back(evaluation::basic);
    else if (name == "rflip_rv") strategies.push_back(evaluation::rflip_rv);
    else if (name == "s") strategies.push_back(evaluation::s);
    else if (name == "a") strategies.push_back(evaluation::a);
    else if (name == "c") strategies.push_back(evaluation::c);
    else if (name == "m") strategies.push_back(evaluation::m);
    else if (name == "ac") strategies.push_back(evaluation::ac);
    else if (name == "am") strategies.push_back(evaluation::am);
    else if (name == "cm") strategies.push_back(evaluation::cm);
    else if (name == "acm") strategies.push_back(evaluation::acm);
  }
  
  return strategies;
}

}  // namespace qubo

#endif  // QUBO_EXPERIMENTS_COMMON_HPP
