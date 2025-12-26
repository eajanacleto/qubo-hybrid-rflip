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
#include "experiments/experiments.hpp"

using nlohmann::json;
using qubo::evaluation;
using qubo::ubqp;
using qubo::ExperimentRunner;
using qubo::eval_experiment;
using qubo::ls_experiment;
using qubo::vns_experiment;
using qubo::get_eval_experiments;
using qubo::get_ls_time_experiments;
using qubo::get_ls_count_experiments;
using qubo::get_vns_time_experiments;
using qubo::get_vns_count_experiments;
using qubo::get_vns_table_experiments;

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
          params["r"] = r;
          params["iters"] = Q->n;
          runner.run(p.second, *Q, params);
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

void run_vns_tables_experiment(ExperimentRunner& runner, const vector<string>& instances) {
  for (const string& instance : instances) {
    for (size_t r_div : {30, 60, 90}) {
      vector<future<void>> futures;
      auto Q = make_shared<ubqp>(move(ubqp::load(instance)));

      for (auto& p : get_vns_table_experiments()) {
        futures.emplace_back(async([&runner, p, Q, instance, r_div]() {
          json params;
          params["exp"] = "vns_tables";
          params["instance"] = instance;
          params["n"] = Q->n;
          params["eval"] = static_cast<int>(p.first);
          params["r_max"] = (Q->n * r_div) / 100;
          params["r_step"] = 1;
          params["iters"] = 10;
          params["ls_iters"] = Q->n;
          runner.run(p.second, *Q, params);
        }));
      }

      for (auto& f : futures) f.wait();
    }
  }
}

// ============================================================================
// Instance set definitions
// ============================================================================

const vector<string> INSTANCES_EVAL = {"G54"};

const vector<string> INSTANCES_LS = {"bqp250.1", "bqp500.1", "G43", "G22"};

const vector<string> INSTANCES_VNS_FIGURES = {"bqp100.1", "bqp250.1", "bqp500.1", "G1"};

const vector<string> INSTANCES_VNS_TABLES = {
    "bqp50.1",   "bqp50.2",   "bqp50.3",   "bqp50.4",    "bqp50.5",    "bqp50.6",
    "bqp50.7",   "bqp50.8",   "bqp50.9",   "bqp50.10",   "bqp100.1",   "bqp100.2",
    "bqp100.3",  "bqp100.4",  "bqp100.5",  "bqp100.6",   "bqp100.7",   "bqp100.8",
    "bqp100.9",  "bqp100.10", "bqp250.1",  "bqp250.2",   "bqp250.3",   "bqp250.4",
    "bqp250.5",  "bqp250.6",  "bqp250.7",  "bqp250.8",   "bqp250.9",   "bqp250.10",
    "bqp500.1",  "bqp500.2",  "bqp500.3",  "bqp500.4",   "bqp500.5",   "bqp500.6",
    "bqp500.7",  "bqp500.8",  "bqp500.9",  "bqp500.10",  "G1",         "G2",
    "G3",        "G4",        "G5",        "G6",         "G7",         "G8",
    "G9",        "G10",       "G11",       "G12",        "G13",        "G14",
    "G15",       "G16",       "G17",       "G18",        "G19",        "G20",
    "G21",       "bqp1000.1", "bqp1000.2", "bqp1000.3",  "bqp1000.4",  "bqp1000.5",
    "bqp1000.6", "bqp1000.7", "bqp1000.8", "bqp1000.9",  "bqp1000.10", "G43",
    "G44",       "G45",       "G46",       "G47",        "G51",        "G52",
    "G53",       "G54",       "G22",       "G23",        "G24",        "G25",
    "G26",       "G27",       "G28",       "G29",        "G30",
    "G31",       "G32",       "G33",       "G34",        "G35",        "G36",
    "G37",       "G38",       "G39",       "G40",        "G41",        "G42",
    "bqp2500.1", "bqp2500.2", "bqp2500.3", "bqp2500.4",  "bqp2500.5",  "bqp2500.6",
    "bqp2500.7", "bqp2500.8", "bqp2500.9", "bqp2500.10",
};

// ============================================================================
// Main
// ============================================================================

void print_usage(const char* program_name) {
  cout << "UBQP Hybrid R-Flip Experiments" << endl;
  cout << "==============================" << endl;
  cout << endl;
  cout << "Usage: " << program_name << " <results_file.json> <experiment_type>" << endl;
  cout << endl;
  cout << "Experiment types:" << endl;
  cout << "  eval              - Evaluation time comparison" << endl;
  cout << "  ls                - Local search time measurement" << endl;
  cout << "  ls_count          - Local search algorithm choice counting" << endl;
  cout << "  vns_figures       - VNS experiments for figures" << endl;
  cout << "  vns_figures_count - VNS algorithm choice counting" << endl;
  cout << "  vns_tables        - VNS benchmark for tables" << endl;
  cout << endl;
  cout << "Examples:" << endl;
  cout << "  " << program_name << " output/results/eval.json eval" << endl;
  cout << "  " << program_name << " output/results/ls.json ls" << endl;
}

int main(int argc, const char* argv[]) {
  if (argc < 3) {
    print_usage(argv[0]);
    return 1;
  }

  const string results_filename(argv[1]);
  const string experiment(argv[2]);

  ExperimentRunner runner(results_filename);

  try {
    if (experiment == "eval") {
      run_eval_experiment(runner, INSTANCES_EVAL);
    } 
    else if (experiment == "ls") {
      run_ls_experiment(runner, INSTANCES_LS);
    }
    else if (experiment == "ls_count") {
      run_ls_count_experiment(runner, INSTANCES_LS);
    }
    else if (experiment == "vns_figures") {
      run_vns_figures_experiment(runner, INSTANCES_VNS_FIGURES);
    }
    else if (experiment == "vns_figures_count") {
      run_vns_figures_count_experiment(runner, INSTANCES_VNS_FIGURES);
    }
    else if (experiment == "vns_tables") {
      run_vns_tables_experiment(runner, INSTANCES_VNS_TABLES);
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
