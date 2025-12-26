/**
 * @file solution.hpp
 * @brief Solution data structures for UBQP search algorithms.
 *
 * Defines incumbent and neighbor solution structures used by local search
 * and VNS algorithms.
 */

#ifndef QUBO_CORE_SOLUTION_HPP
#define QUBO_CORE_SOLUTION_HPP

#include <memory>

#include "evaluation.hpp"

namespace qubo {

// Forward declaration
struct ubqp;

/**
 * @brief Represents an incumbent solution in a UBQP search process.
 *
 * Contains solution vector, objective value, and auxiliary data structures
 * for efficient reevaluation. The template parameter determines which
 * auxiliary structures are maintained.
 *
 * @tparam eval Evaluation strategy determining auxiliary data requirements
 */
template <evaluation eval>
struct incumbent_solution {
  /** The incumbent's objective function value. */
  long fy;
  /** The incumbent's solution vector. */
  std::unique_ptr<bool[]> y;
  /** The previous solution vector with an updated reevaluation vector. */
  std::unique_ptr<bool[]> x;
  /** The number of non-zero components in x. */
  size_t nx1;
  /** The number of components which have changed from 0 to 1 between x and y. */
  size_t wx01;
  /** The number of components which have changed from 1 to 0 between x and y. */
  size_t wx10;
  /** The components which have changed from 0 to 1 between x and y. */
  std::unique_ptr<size_t[]> WX01;
  /** The components which have changed from 1 to 0 between x and y. */
  std::unique_ptr<size_t[]> WX10;
  /** The reevaluation vector of x. */
  std::unique_ptr<long[]> dx;

  /**
   * Constructs an incumbent solution.
   * @param n Problem dimension (number of variables)
   */
  explicit incumbent_solution(size_t n)
      : fy(0),
        y(new bool[n]()),
        x(new bool[n]()),
        nx1(0),
        wx01(0),
        wx10(0),
        WX01(new size_t[n]()),
        WX10(new size_t[n]()),
        dx(new long[n]()) {}
};

/**
 * @brief Specialization for basic evaluation (no auxiliary structures).
 *
 * When using basic evaluation, we don't need the reevaluation vector
 * or change tracking, saving memory.
 */
template <>
struct incumbent_solution<evaluation::basic> {
  /** The incumbent's objective function value. */
  long fy;
  /** The incumbent's solution vector. */
  std::unique_ptr<bool[]> y;

  /**
   * Constructs an incumbent solution.
   * @param n Problem dimension (number of variables)
   */
  explicit incumbent_solution(size_t n) : fy(0), y(new bool[n]()) {}
};

/**
 * @brief Represents a neighbor solution obtained from an r-flip move.
 *
 * Stores information about which bits were flipped to obtain this
 * neighbor from the incumbent, enabling efficient evaluation.
 */
struct neighbor_solution {
  /** The neighbor's objective function value. */
  long fz;
  /** The number of non-zero components in the neighbor. */
  size_t n1z;
  /** The number of components which changed from 0 to 1 (0→1 flips). */
  size_t r01;
  /** The number of components which changed from 1 to 0 (1→0 flips). */
  size_t r10;
  /** The non-zero components in the neighbor (indices where z[i] = 1). */
  std::unique_ptr<size_t[]> N1z;
  /** Indices of components that changed from 0 to 1. */
  std::unique_ptr<size_t[]> R01;
  /** Indices of components that changed from 1 to 0. */
  std::unique_ptr<size_t[]> R10;

  /**
   * Constructs a neighbor solution workspace.
   * @param n Maximum problem dimension
   */
  explicit neighbor_solution(size_t n)
      : fz(0), n1z(0), r01(0), r10(0),
        N1z(new size_t[n]()),
        R01(new size_t[n]()),
        R10(new size_t[n]()) {}
};

}  // namespace qubo

#endif  // QUBO_CORE_SOLUTION_HPP
