/**
 * @file ubqp.hpp
 * @brief UBQP (Unconstrained Binary Quadratic Programming) problem definition.
 *
 * This file contains the data structure for representing UBQP problem instances,
 * including methods for loading instances from OR-Library (BQP) and Stanford Gset
 * (MaxCut) formats.
 */

#ifndef QUBO_CORE_UBQP_HPP
#define QUBO_CORE_UBQP_HPP

#include <cassert>
#include <fstream>
#include <memory>
#include <string>

namespace qubo {

/**
 * @brief A UBQP problem instance.
 *
 * Represents an Unconstrained Binary Quadratic Programming problem
 * with an n×n matrix Q. The objective is to minimize x'Qx where x ∈ {0,1}^n.
 */
struct ubqp {
  /** The number of variables in a solution vector. */
  size_t n;

  /** The matrix Q, stored in row-major order. */
  std::unique_ptr<long[]> Q;

  /**
   * @brief Constructs an UBQP object with n variables.
   * @param n Number of variables
   */
  explicit ubqp(size_t n) : n(n), Q(new long[n * n]()) {}

  /**
   * @brief Obtains the pointer to the beginning of a row of the matrix.
   * @param i Row index
   * @return Pointer to row i
   */
  long* operator[](size_t i) const { return &Q[n * i]; }

  /**
   * @brief Constructs a UBQP problem instance from a file label.
   *
   * Supports two formats:
   * - "bqpN.M" for OR-Library instances (e.g., "bqp250.1")
   * - "GN" for Stanford Gset MaxCut instances (e.g., "G1")
   *
   * @param label Instance label
   * @return UBQP instance
   */
  static ubqp load(const std::string& label) {
    size_t n, number;
    if (sscanf(label.c_str(), "bqp%zu.%zu", &n, &number) == 2) {
      return from_bqp(n, number);
    }
    if (sscanf(label.c_str(), "G%zu", &number) == 1) {
      return from_maxcut(number);
    }
    assert(false && "Unknown instance format");
    exit(1);
  }

  /**
   * @brief Constructs a UBQP instance from an OR-Library BQP file.
   *
   * @param n Problem size (50, 100, 250, 500, 1000, or 2500)
   * @param number Instance number (1-10)
   * @return UBQP instance
   */
  static ubqp from_bqp(size_t n, size_t number) {
    std::ifstream file("data/instances/bqp" + std::to_string(n) + std::string(".txt"),
                       std::ifstream::in);
    assert(file && "Could not open BQP file");

    size_t count;
    file >> count;

    for (size_t k = 1; k <= count; k++) {
      size_t nonzeros, i, j;
      long q;
      if (k == number) {
        file >> n >> nonzeros;
        ubqp Q(n);
        for (size_t l = 0; l < nonzeros; l++) {
          file >> i >> j >> q;
          i -= 1;
          j -= 1;
          // Convert maximization to minimization
          Q[i][j] = Q[j][i] = (i == j) ? -q : 2 * -q;
        }
        return Q;
      } else {
        file >> n >> nonzeros;
        for (size_t l = 0; l < nonzeros; l++) {
          file >> i >> j >> q;
        }
      }
    }

    assert(false && "Instance number not found");
    exit(1);
  }

  /**
   * @brief Constructs a UBQP instance from a Stanford Gset MaxCut file.
   *
   * @param number Graph number (1-54)
   * @return UBQP instance
   */
  static ubqp from_maxcut(size_t number) {
    std::ifstream file("data/instances/G" + std::to_string(number), std::ifstream::in);
    assert(file && "Could not open Gset file");

    size_t n, nonzeros;
    file >> n >> nonzeros;

    ubqp Q(n);
    for (size_t l = 0; l < nonzeros; l++) {
      size_t i, j;
      long q;
      file >> i >> j >> q;
      i -= 1;
      j -= 1;
      // MaxCut to QUBO transformation
      Q[i][i] -= q;
      Q[j][j] -= q;
      Q[i][j] += 2 * q;
      Q[j][i] += 2 * q;
    }

    return Q;
  }
};

}  // namespace qubo

#endif  // QUBO_CORE_UBQP_HPP
