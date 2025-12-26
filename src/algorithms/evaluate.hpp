/**
 * @file evaluate.hpp
 * @brief Main include file for all evaluation algorithms.
 *
 * This header provides a convenient way to include all evaluation-related
 * functionality. For more granular includes, use the individual headers
 * in the evaluate/ and rv/ subdirectories.
 *
 * Structure:
 * - evaluate/basic.hpp    : O(n²) basic evaluation
 * - evaluate/rfliprv.hpp  : O(r²) r-flip-rv evaluation
 * - evaluate/hybrid.hpp   : Hybrid strategy selection
 * - evaluate/verify.hpp   : Debug verification functions
 * - rv/build.hpp          : Build reevaluation vector from scratch
 * - rv/update.hpp         : Incremental reevaluation vector update
 * - rv/hybrid.hpp         : Hybrid RV update strategy
 */

#ifndef QUBO_ALGORITHMS_EVALUATE_HPP
#define QUBO_ALGORITHMS_EVALUATE_HPP

// Evaluation algorithms
#include "evaluate/basic.hpp"
#include "evaluate/rfliprv.hpp"
#include "evaluate/hybrid.hpp"
#include "evaluate/verify.hpp"

// Reevaluation vector algorithms
#include "rv/build.hpp"
#include "rv/update.hpp"
#include "rv/hybrid.hpp"

#endif  // QUBO_ALGORITHMS_EVALUATE_HPP
