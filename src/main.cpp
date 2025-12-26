/**
 * @file main.cpp
 * @brief Main entry point for UBQP hybrid r-flip experiments.
 *
 * This program runs experiments comparing different evaluation strategies
 * for the Unconstrained Binary Quadratic Programming (UBQP) problem.
 *
 * Usage: ./main <results_file.json> <experiment_type>
 *
 * Experiment types:
 * - eval: Evaluation time comparison
 * - ls: Local search time comparison
 * - ls_count: Local search algorithm choice counting
 * - vns_figures: VNS experiments for generating figures
 * - vns_figures_count: VNS algorithm choice counting for figures
 * - vns_tables: VNS experiments for generating tables
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
using qubo::eval_experiment;
using qubo::ls_experiment;
using qubo::vns_experiment;

using std::async;
using std::cout;
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

int main(int argc, const char* argv[]) {
  if (argc < 3) {
    cout << "Usage: " << argv[0] << " <results_file.json> <experiment_type>" << endl;
    cout << "Experiment types: eval, ls, ls_count, vns_figures, vns_figures_count, vns_tables" << endl;
    return 1;
  }

  const string results_filename(argv[1]);
  json results;
  mutex results_mutex;

  // Load existing results if any
  {
    ifstream file(results_filename, ifstream::in);
    if (file) file >> results;
  }

  /**
   * Runs an experiment if not already computed, and stores results in JSON file.
   */
  auto run = [&](function<json(const ubqp&, json)> exp, const ubqp& Q, json params) {
    {
      lock_guard<mutex> lock(results_mutex);
      for (auto& j : results) {
        if (j["params"] == params) return;
      }
      cout << "<<< " << params << endl;
    }

    json result = exp(Q, params);

    {
      lock_guard<mutex> lock(results_mutex);
      cout << ">>> " << result << endl;
      results.push_back({{"params", params}, {"result", result}});
      ofstream file(results_filename, ifstream::out);
      file << setw(1) << results;
    }
  };

  const string experiment(argv[2]);

  // ============================================================================
  // Evaluation experiment: measures time for different evaluation strategies
  // ============================================================================
  if (experiment == "eval") {
    for (string instance : {"G54"}) {
      ubqp Q = ubqp::load(instance);
      size_t n1_step = max(Q.n / 7, size_t(1));
      size_t r_step = max(Q.n / 100, size_t(1));

      for (size_t n1 = n1_step; n1 <= n1_step * 6 + 1; n1 += n1_step) {
        for (auto& [eval, experiment_fn] : {
                 pair{evaluation::basic, eval_experiment<evaluation::basic>},
                 pair{evaluation::rflip_rv, eval_experiment<evaluation::rflip_rv>},
                 pair{evaluation::s, eval_experiment<evaluation::s>},
                 pair{evaluation::a, eval_experiment<evaluation::a>},
                 pair{evaluation::c, eval_experiment<evaluation::c>},
                 pair{evaluation::m, eval_experiment<evaluation::m>},
                 pair{evaluation::ac, eval_experiment<evaluation::ac>},
                 pair{evaluation::am, eval_experiment<evaluation::am>},
                 pair{evaluation::cm, eval_experiment<evaluation::cm>},
                 pair{evaluation::acm, eval_experiment<evaluation::acm>},
             }) {
          for (size_t r = r_step; r <= Q.n; r += r_step) {
            run(experiment_fn, Q,
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

  // ============================================================================
  // Local Search experiment: measures processing time
  // ============================================================================
  if (experiment == "ls") {
    for (string instance : {"bqp250.1", "bqp500.1", "G43", "G22"}) {
      ubqp Q = ubqp::load(instance);
      size_t r_step = max(Q.n / 100, size_t(1));

      for (auto& [eval, experiment_fn] : {
               pair{evaluation::basic, ls_experiment<evaluation::basic, false>},
               pair{evaluation::rflip_rv, ls_experiment<evaluation::rflip_rv, false>},
               pair{evaluation::s, ls_experiment<evaluation::s, false>},
               pair{evaluation::a, ls_experiment<evaluation::a, false>},
               pair{evaluation::c, ls_experiment<evaluation::c, false>},
               pair{evaluation::m, ls_experiment<evaluation::m, false>},
               pair{evaluation::ac, ls_experiment<evaluation::ac, false>},
               pair{evaluation::am, ls_experiment<evaluation::am, false>},
               pair{evaluation::cm, ls_experiment<evaluation::cm, false>},
               pair{evaluation::acm, ls_experiment<evaluation::acm, false>},
           }) {
        for (size_t r = r_step; r <= Q.n; r += r_step) {
          run(experiment_fn, Q,
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

  // ============================================================================
  // Local Search counting experiment: counts algorithm choices (parallel)
  // ============================================================================
  if (experiment == "ls_count") {
    vector<future<void>> futures;

    for (string instance : {"bqp250.1", "bqp500.1", "G43", "G22"}) {
      auto Q = make_shared<ubqp>(move(ubqp::load(instance)));
      size_t r_step = max(Q->n / 100, size_t(1));

      for (auto p : {
               pair{evaluation::s, ls_experiment<evaluation::s, true>},
               pair{evaluation::a, ls_experiment<evaluation::a, true>},
               pair{evaluation::c, ls_experiment<evaluation::c, true>},
               pair{evaluation::m, ls_experiment<evaluation::m, true>},
               pair{evaluation::ac, ls_experiment<evaluation::ac, true>},
               pair{evaluation::am, ls_experiment<evaluation::am, true>},
               pair{evaluation::cm, ls_experiment<evaluation::cm, true>},
               pair{evaluation::acm, ls_experiment<evaluation::acm, true>},
           }) {
        for (size_t r = r_step; r <= Q->n; r += r_step) {
          futures.emplace_back(async([=]() {
            run(p.second, *Q,
                {{"exp", "ls_count"},
                 {"instance", instance},
                 {"n", Q->n},
                 {"eval", static_cast<int>(p.first)},
                 {"r", r},
                 {"iters", Q->n}});
          }));
        }
      }
    }

    for (auto& f : futures) f.wait();
  }

  // ============================================================================
  // VNS figures experiment: measures processing time for figure generation
  // ============================================================================
  if (experiment == "vns_figures") {
    for (string instance : {"bqp100.1", "bqp250.1", "bqp500.1", "G1"}) {
      ubqp Q = ubqp::load(instance);
      size_t r_step = max(Q.n / 100, size_t(1));

      for (auto& [eval, experiment_fn] : {
               pair{evaluation::basic, vns_experiment<evaluation::basic, false>},
               pair{evaluation::rflip_rv, vns_experiment<evaluation::rflip_rv, false>},
               pair{evaluation::s, vns_experiment<evaluation::s, false>},
               pair{evaluation::a, vns_experiment<evaluation::a, false>},
               pair{evaluation::c, vns_experiment<evaluation::c, false>},
               pair{evaluation::m, vns_experiment<evaluation::m, false>},
               pair{evaluation::ac, vns_experiment<evaluation::ac, false>},
               pair{evaluation::am, vns_experiment<evaluation::am, false>},
               pair{evaluation::cm, vns_experiment<evaluation::cm, false>},
               pair{evaluation::acm, vns_experiment<evaluation::acm, false>},
           }) {
        for (size_t r = r_step; r <= Q.n; r += r_step) {
          run(experiment_fn, Q,
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

  // ============================================================================
  // VNS figures counting experiment: counts algorithm choices (parallel)
  // ============================================================================
  if (experiment == "vns_figures_count") {
    vector<future<void>> futures;

    for (string instance : {"bqp100.1", "bqp250.1", "bqp500.1", "G1"}) {
      auto Q = make_shared<ubqp>(move(ubqp::load(instance)));
      size_t r_step = max(Q->n / 100, size_t(1));

      for (auto p : {
               pair{evaluation::s, vns_experiment<evaluation::s, true>},
               pair{evaluation::a, vns_experiment<evaluation::a, true>},
               pair{evaluation::c, vns_experiment<evaluation::c, true>},
               pair{evaluation::m, vns_experiment<evaluation::m, true>},
               pair{evaluation::ac, vns_experiment<evaluation::ac, true>},
               pair{evaluation::am, vns_experiment<evaluation::am, true>},
               pair{evaluation::cm, vns_experiment<evaluation::cm, true>},
               pair{evaluation::acm, vns_experiment<evaluation::acm, true>},
           }) {
        for (size_t r = r_step; r <= Q->n; r += r_step) {
          futures.emplace_back(async([=]() {
            run(p.second, *Q,
                {{"exp", "vns_figures_count"},
                 {"instance", instance},
                 {"n", Q->n},
                 {"eval", static_cast<int>(p.first)},
                 {"r_max", r},
                 {"r_step", 1},
                 {"iters", 10},
                 {"ls_iters", Q->n}});
          }));
        }
      }
    }

    for (auto& f : futures) f.wait();
  }

  // ============================================================================
  // VNS tables experiment: comprehensive benchmark for table generation
  // ============================================================================
  if (experiment == "vns_tables") {
    for (string instance : {
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
             "G25",       "G26",       "G27",       "G28",        "G29",        "G30",
             "G31",       "G32",       "G33",       "G34",        "G35",        "G36",
             "G37",       "G38",       "G39",       "G40",        "G41",        "G42",
             "bqp2500.1", "bqp2500.2", "bqp2500.3", "bqp2500.4",  "bqp2500.5",  "bqp2500.6",
             "bqp2500.7", "bqp2500.8", "bqp2500.9", "bqp2500.10",
         }) {
      for (size_t r_div : {30, 60, 90}) {
        vector<future<void>> futures;
        auto Q = make_shared<ubqp>(move(ubqp::load(instance)));

        for (auto& p : {
                 pair{evaluation::basic, vns_experiment<evaluation::basic, false>},
                 pair{evaluation::rflip_rv, vns_experiment<evaluation::rflip_rv, false>},
                 pair{evaluation::s, vns_experiment<evaluation::s, false>},
             }) {
          futures.emplace_back(async([=]() {
            run(p.second, *Q,
                {{"exp", "vns_tables"},
                 {"instance", instance},
                 {"n", Q->n},
                 {"eval", static_cast<int>(p.first)},
                 {"r_max", (Q->n * r_div) / 100},
                 {"r_step", 1},
                 {"iters", 10},
                 {"ls_iters", Q->n}});
          }));
        }

        for (auto& f : futures) f.wait();
      }
    }
  }

  return 0;
}
