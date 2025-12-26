/**
 * @file utils.hpp
 * @brief Utility functions for UBQP experiments.
 *
 * Contains helper functions for timing measurements and other utilities.
 */

#ifndef QUBO_CORE_UTILS_HPP
#define QUBO_CORE_UTILS_HPP

#include <chrono>
#include <functional>

namespace qubo {

/**
 * @brief Measures the execution time of a function in nanoseconds.
 *
 * @param f The function to measure
 * @return Execution time in nanoseconds
 */
inline size_t measure(std::function<void(void)> f) {
  auto t1 = std::chrono::steady_clock::now();
  f();
  auto t2 = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
}

}  // namespace qubo

#endif  // QUBO_CORE_UTILS_HPP
