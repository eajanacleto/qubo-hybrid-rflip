/**
 * @file main.cpp
 * @brief Main entry point for UBQP hybrid r-flip experiments.
 *
 * This program runs experiments comparing different evaluation strategies
 * for the Unconstrained Binary Quadratic Programming (UBQP) problem.
 *
 * Usage: ./main <results_file.json> <experiment_type> [options]
 *
 * Experiment types:
 * - eval: Evaluation time comparison
 * - ls: Local search time comparison
 * - ls_count: Local search algorithm choice counting
 * - vns_figures: VNS experiments for generating figures
 * - vns_figures_count: VNS algorithm choice counting for figures
 * - vns_tables: VNS experiments for generating tables
 *
 * Configuration files are loaded from experiments/config/
 */

#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "json.hpp"
#include "core/evaluation.hpp"
#include "core/ubqp.hpp"
#include "algorithms/local_search/ls_strategy.hpp"
#include "experiments/experiments.hpp"

using nlohmann::json;
using qubo::evaluation;
using qubo::ubqp;
using qubo::ls_strategy;
using qubo::ExperimentRunner;
using qubo::eval_experiment;
using qubo::ls_experiment;
using qubo::vns_experiment;
using qubo::get_eval_experiments;
using qubo::get_ls_time_experiments;
using qubo::get_ls_count_experiments;
using qubo::get_all_ls_time_experiments;
using qubo::get_all_ls_count_experiments;
using qubo::get_vns_time_experiments;
using qubo::get_vns_count_experiments;
using qubo::get_vns_table_experiments;
using qubo::get_vns_table_experiments_first_improvement;
using qubo::get_vns_table_experiments_best_improvement;
using qubo::get_vns_table_experiments_runtime;
using qubo::get_all_ls_strategies;

using std::async;
using std::cout;
using std::cerr;
using std::endl;
using std::function;
using std::future;
using std::ifstream;
using std::lock_guard;
using std::make_shared;
using std::max;
using std::move;
using std::mutex;
using std::ofstream;
using std::pair;
using std::setw;
using std::string;
using std::vector;

// ============================================================================
// Configuration loading
// ============================================================================

const string CONFIG_PATH = "experiments/config/instances.json";

/**
 * @brief Load instance set from JSON configuration file.
 */
vector<string> load_instances(const string& set_name) {
  ifstream file(CONFIG_PATH);
  if (!file) {
    throw std::runtime_error("Cannot open config file: " + CONFIG_PATH);
  }
  
  json config;
  file >> config;
  
  const auto& sets = config["instance_sets"];
  if (!sets.contains(set_name)) {
    throw std::runtime_error("Unknown instance set: " + set_name);
  }
  
  vector<string> result;
  const auto& set = sets[set_name];
  
  if (set.contains("compose")) {
    // Recursively load composed sets
    for (const auto& sub_name : set["compose"]) {
      auto sub_instances = load_instances(sub_name.get<string>());
      result.insert(result.end(), sub_instances.begin(), sub_instances.end());
    }
  } else if (set.contains("instances")) {
    for (const auto& inst : set["instances"]) {
      result.push_back(inst.get<string>());
    }
  }
  
  return result;
}

// ============================================================================
// Experiment runners
// ============================================================================

void run_eval_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  for (const string& instance : instances) {
    ubqp Q = ubqp::load(instance);
    size_t n1_step = max(Q.n / 7, size_t(1));
    size_t r_step = max(Q.n / 100, size_t(1));

    for (size_t n1 = n1_step; n1 <= n1_step * 6 + 1; n1 += n1_step) {
      for (const auto& [eval, experiment_fn] : get_eval_experiments()) {
        for (size_t r = r_step; r <= Q.n; r += r_step) {
          runner.run(experiment_fn, Q,
              {{"exp", "eval"},
               {"instance", instance},
               {"n", Q.n},
               {"eval", static_cast<int>(eval)},
               {"n1", n1},
               {"r", r},
               {"iters", 100}});
        }
      }
    }
  }
}

void run_ls_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  for (const string& instance : instances) {
    ubqp Q = ubqp::load(instance);
    size_t r_step = max(Q.n / 100, size_t(1));

    for (const auto& [eval, experiment_fn] : get_ls_time_experiments()) {
      for (size_t r = r_step; r <= Q.n; r += r_step) {
        runner.run(experiment_fn, Q,
            {{"exp", "ls"},
             {"instance", instance},
             {"n", Q.n},
             {"eval", static_cast<int>(eval)},
             {"ls_strategy", 0},  // first_improvement
             {"r", r},
             {"iters", Q.n}});
      }
    }
  }
}

/**
 * @brief Run LS experiments with ALL local search strategies.
 * 
 * This runs experiments comparing different evaluation strategies
 * across all available local search strategies (first_improvement,
 * best_improvement, etc.)
 */
void run_ls_all_strategies_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  using qubo::ls_strategy;
  
  for (const string& instance : instances) {
    ubqp Q = ubqp::load(instance);
    size_t r_step = max(Q.n / 100, size_t(1));

    for (const auto& [ls_strat, eval, experiment_fn] : get_all_ls_time_experiments()) {
      for (size_t r = r_step; r <= Q.n; r += r_step) {
        runner.run(experiment_fn, Q,
            {{"exp", "ls_all"},
             {"instance", instance},
             {"n", Q.n},
             {"eval", static_cast<int>(eval)},
             {"ls_strategy", static_cast<int>(ls_strat)},
             {"ls_strategy_name", qubo::ls_strategy_name(ls_strat)},
             {"r", r},
             {"iters", Q.n}});
      }
    }
  }
}

void run_ls_count_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  vector<future<void>> futures;

  for (const string& instance : instances) {
    auto Q = make_shared<ubqp>(move(ubqp::load(instance)));
    size_t r_step = max(Q->n / 100, size_t(1));

    for (auto p : get_ls_count_experiments()) {
      for (size_t r = r_step; r <= Q->n; r += r_step) {
        futures.emplace_back(async([&runner, p, Q, instance, r]() {
          json params;
          params["exp"] = "ls_count";
          params["instance"] = instance;
          params["n"] = Q->n;
          params["eval"] = static_cast<int>(p.first);
          params["ls_strategy"] = 0;  // first_improvement
          params["r"] = r;
          params["iters"] = Q->n;
          runner.run(p.second, *Q, params);
        }));
      }
    }
  }

  for (auto& f : futures) f.wait();
}

/**
 * @brief Run LS count experiments with ALL local search strategies.
 */
void run_ls_all_strategies_count_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  using qubo::ls_strategy;
  vector<future<void>> futures;

  for (const string& instance : instances) {
    auto Q = make_shared<ubqp>(move(ubqp::load(instance)));
    size_t r_step = max(Q->n / 100, size_t(1));

    for (auto [ls_strat, eval, fn] : get_all_ls_count_experiments()) {
      for (size_t r = r_step; r <= Q->n; r += r_step) {
        futures.emplace_back(async([&runner, fn, Q, instance, r, eval, ls_strat]() {
          json params;
          params["exp"] = "ls_count_all";
          params["instance"] = instance;
          params["n"] = Q->n;
          params["eval"] = static_cast<int>(eval);
          params["ls_strategy"] = static_cast<int>(ls_strat);
          params["ls_strategy_name"] = qubo::ls_strategy_name(ls_strat);
          params["r"] = r;
          params["iters"] = Q->n;
          runner.run(fn, *Q, params);
        }));
      }
    }
  }

  for (auto& f : futures) f.wait();
}

void run_vns_figures_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  for (const string& instance : instances) {
    ubqp Q = ubqp::load(instance);
    size_t r_step = max(Q.n / 100, size_t(1));

    for (const auto& [eval, experiment_fn] : get_vns_time_experiments()) {
      for (size_t r = r_step; r <= Q.n; r += r_step) {
        runner.run(experiment_fn, Q,
            {{"exp", "vns_figures"},
             {"instance", instance},
             {"n", Q.n},
             {"eval", static_cast<int>(eval)},
             {"r_max", r},
             {"r_step", 1},
             {"iters", 10},
             {"ls_iters", Q.n}});
      }
    }
  }
}

void run_vns_figures_count_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  vector<future<void>> futures;

  for (const string& instance : instances) {
    auto Q = make_shared<ubqp>(move(ubqp::load(instance)));
    size_t r_step = max(Q->n / 100, size_t(1));

    for (auto p : get_vns_count_experiments()) {
      for (size_t r = r_step; r <= Q->n; r += r_step) {
        futures.emplace_back(async([&runner, p, Q, instance, r]() {
          json params;
          params["exp"] = "vns_figures_count";
          params["instance"] = instance;
          params["n"] = Q->n;
          params["eval"] = static_cast<int>(p.first);
          params["r_max"] = r;
          params["r_step"] = 1;
          params["iters"] = 10;
          params["ls_iters"] = Q->n;
          runner.run(p.second, *Q, params);
        }));
      }
    }
  }

  for (auto& f : futures) f.wait();
}

/**
 * @brief Run VNS tables experiment with a specific local search strategy.
 */
template<typename GetExpFunc>
void run_vns_tables_experiment_with_strategy(
    ExperimentRunner& runner, 
    const vector<string>& instances,
    GetExpFunc get_experiments,
    int ls_strategy_num,
    const string& ls_strategy_name) {
  
  for (const string& instance : instances) {
    for (size_t r_div : {30, 60, 90}) {
      vector<future<void>> futures;
      auto Q = make_shared<ubqp>(move(ubqp::load(instance)));

      for (auto& p : get_experiments()) {
        futures.emplace_back(async([&runner, p, Q, instance, r_div, ls_strategy_num, &ls_strategy_name]() {
          json params;
          params["exp"] = "vns_tables";
          params["instance"] = instance;
          params["n"] = Q->n;
          params["eval"] = static_cast<int>(p.first);
          params["r_max"] = (Q->n * r_div) / 100;
          params["r_step"] = 1;
          params["iters"] = 10;
          params["ls_iters"] = Q->n;
          params["ls_strategy"] = ls_strategy_num;
          params["ls_strategy_name"] = ls_strategy_name;
          runner.run(p.second, *Q, params);
        }));
      }

      for (auto& f : futures) f.wait();
    }
  }
}

/**
 * @brief Run VNS tables experiment for all LS strategies using runtime dispatch.
 * 
 * This version uses vns_runtime which allows selecting the LS strategy at runtime,
 * avoiding the need to modify vns.hpp when adding new LS strategies.
 */
void run_vns_tables_all_strategies_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  auto experiments = get_vns_table_experiments_runtime();
  
  for (ls_strategy ls_strat : get_all_ls_strategies()) {
    string ls_name = (ls_strat == ls_strategy::first_improvement) ? "first_improvement" : "best_improvement";
    cout << "Running VNS tables with " << ls_name << "..." << endl;
    
    for (const string& instance : instances) {
      for (size_t r_div : {30, 60, 90}) {
        vector<future<void>> futures;
        auto Q = make_shared<ubqp>(move(ubqp::load(instance)));

        for (auto& p : experiments) {
          futures.emplace_back(async([&runner, p, Q, instance, r_div, ls_strat, &ls_name]() {
            json params;
            params["exp"] = "vns_tables";
            params["instance"] = instance;
            params["n"] = Q->n;
            params["eval"] = static_cast<int>(p.first);
            params["r_max"] = (Q->n * r_div) / 100;
            params["r_step"] = 1;
            params["iters"] = 10;
            params["ls_iters"] = Q->n;
            params["ls_strategy"] = static_cast<int>(ls_strat);
            params["ls_strategy_name"] = ls_name;
            runner.run(p.second, *Q, params);
          }));
        }

        for (auto& f : futures) f.wait();
      }
    }
  }
}

/**
 * @brief Backward compatibility: run VNS tables with first_improvement only.
 */
void run_vns_tables_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  run_vns_tables_experiment_with_strategy(
      runner, instances, get_vns_table_experiments_first_improvement,
      static_cast<int>(ls_strategy::first_improvement), "first_improvement");
}

// Instance sets are now loaded from experiments/config/instances.json

// ============================================================================
// Main
// ============================================================================

void print_usage(const char* program_name) {
  cout << "UBQP Hybrid R-Flip Experiments" << endl;
  cout << "==============================" << endl;
  cout << endl;
  cout << "Usage: " << program_name << " <results_file.json> <experiment_type> [instance_set]" << endl;
  cout << endl;
  cout << "Experiment types:" << endl;
  cout << "  eval              - Evaluation time comparison" << endl;
  cout << "  ls                - Local search time (first_improvement only)" << endl;
  cout << "  ls_count          - Local search counting (first_improvement only)" << endl;
  cout << "  ls_all            - Local search time (ALL LS strategies)" << endl;
  cout << "  ls_count_all      - Local search counting (ALL LS strategies)" << endl;
  cout << "  vns_figures       - VNS experiments for figures" << endl;
  cout << "  vns_figures_count - VNS algorithm choice counting" << endl;
  cout << "  vns_tables        - VNS benchmark for tables (first_improvement only)" << endl;
  cout << "  vns_tables_all    - VNS benchmark for tables (ALL LS strategies)" << endl;
  cout << endl;
  cout << "Instance sets (defined in experiments/config/instances.json):" << endl;
  cout << "  quick_test, eval, ls_figures, vns_figures, vns_tables_small," << endl;
  cout << "  vns_tables_medium, vns_tables_large, vns_tables_all, etc." << endl;
  cout << endl;
  cout << "Examples:" << endl;
  cout << "  " << program_name << " output/results/eval.json eval" << endl;
  cout << "  " << program_name << " output/results/ls.json ls_all quick_test" << endl;
  cout << "  " << program_name << " output/results/vns.json vns_tables_all vns_tables_small" << endl;
}

int main(int argc, const char* argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  const string results_filename(argv[1]);
  const string experiment(argv[2]);
  const string instance_set_override = (argc >= 4) ? argv[3] : "";

  ExperimentRunner runner(results_filename);

  // Helper to get instances - uses override if provided, otherwise default for experiment
  auto get_instances = [&](const string& default_set) {
    const string& set_name = instance_set_override.empty() ? default_set : instance_set_override;
    cout << "Using instance set: " << set_name << endl;
    return load_instances(set_name);
  };

  try {
    if (experiment == "eval") {
      run_eval_experiment(runner, get_instances("eval"));
    } 
    else if (experiment == "ls") {
      run_ls_experiment(runner, get_instances("ls_figures"));
    }
    else if (experiment == "ls_count") {
      run_ls_count_experiment(runner, get_instances("ls_figures"));
    }
    else if (experiment == "ls_all") {
      run_ls_all_strategies_experiment(runner, get_instances("ls_figures"));
    }
    else if (experiment == "ls_count_all") {
      run_ls_all_strategies_count_experiment(runner, get_instances("ls_figures"));
    }
    else if (experiment == "vns_figures") {
      run_vns_figures_experiment(runner, get_instances("vns_figures"));
    }
    else if (experiment == "vns_figures_count") {
      run_vns_figures_count_experiment(runner, get_instances("vns_figures"));
    }
    else if (experiment == "vns_tables") {
      run_vns_tables_experiment(runner, get_instances("vns_tables_all"));
    }
    else if (experiment == "vns_tables_all") {
      run_vns_tables_all_strategies_experiment(runner, get_instances("vns_tables_all"));
    }
    else {
      cerr << "Error: Unknown experiment type: " << experiment << endl;
      print_usage(argv[0]);
      return 1;
    }
  } catch (const std::exception& e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }

  cout << "Terminado" << endl;
  return 0;
}
