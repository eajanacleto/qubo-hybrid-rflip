/**
 * @file evaluation.hpp
 * @brief Evaluation strategies for UBQP solutions.
 *
 * Defines the different evaluation algorithms (basic, r-flip-rv, and hybrid strategies)
 * used to evaluate neighbor solutions in local search and VNS algorithms.
 */

#ifndef QUBO_CORE_EVALUATION_HPP
#define QUBO_CORE_EVALUATION_HPP

namespace qubo {

/**
 * @brief Evaluation strategy enumeration.
 *
 * Represents different evaluation algorithms and hybrid strategies
 * for evaluating UBQP solutions:
 *
 * - basic: O(n²) basic evaluation
 * - rflip_rv: Always uses delta evaluation with reevaluation vector
 * - s, a, c, m: Single hybrid strategies
 * - ac, am, cm: Double hybrid strategies
 * - acm: Triple hybrid strategy
 */
enum struct evaluation {
  basic,     ///< Basic O(n²) evaluation
  rflip_rv,  ///< Delta evaluation with reevaluation vector
  s,         ///< Hybrid strategy S
  a,         ///< Hybrid strategy A
  c,         ///< Hybrid strategy C
  m,         ///< Hybrid strategy M
  ac,        ///< Combined strategy AC
  am,        ///< Combined strategy AM
  cm,        ///< Combined strategy CM
  acm        ///< Combined strategy ACM
};

/**
 * @brief Array of evaluation strategy names.
 */
constexpr const char* evaluation_names[] = {
    "basic", "rflip_rv", "s", "a", "c", "m", "ac", "am", "cm", "acm"};

/**
 * @brief Number of evaluation strategies.
 */
constexpr size_t evaluation_count = 10;

}  // namespace qubo

#endif  // QUBO_CORE_EVALUATION_HPP
