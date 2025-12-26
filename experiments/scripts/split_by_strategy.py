#!/usr/bin/env python3
"""
Split experiment results by local search strategy.

This script takes a JSON results file and splits it into separate files
based on the local search strategy used.

Usage:
    python3 split_by_strategy.py <input.json> <output_dir>

Example:
    python3 split_by_strategy.py results_ls_all.json output/results/by_strategy/
"""

import json
import os
import sys
from collections import defaultdict


def get_strategy_name(result: dict) -> str:
    """Extract strategy name from result params."""
    params = result.get("params", {})
    
    # Try to get strategy name directly
    if "ls_strategy_name" in params:
        return params["ls_strategy_name"]
    
    # Fall back to strategy number
    ls_strategy = params.get("ls_strategy", 0)
    strategy_names = {
        0: "first_improvement",
        1: "best_improvement",
        # Add new strategies here
    }
    return strategy_names.get(ls_strategy, f"strategy_{ls_strategy}")


def split_results(input_file: str, output_dir: str):
    """Split results file by local search strategy."""
    
    # Load input file
    with open(input_file) as f:
        results = json.load(f)
    
    if not isinstance(results, list):
        results = [results]
    
    # Group by strategy
    by_strategy = defaultdict(list)
    for result in results:
        strategy = get_strategy_name(result)
        by_strategy[strategy].append(result)
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Write separate files
    for strategy, strategy_results in by_strategy.items():
        output_file = os.path.join(output_dir, f"results_{strategy}.json")
        with open(output_file, "w") as f:
            json.dump(strategy_results, f, indent=1)
        print(f"  {strategy}: {len(strategy_results)} results -> {output_file}")
    
    # Also create a summary file
    summary = {
        "total_results": len(results),
        "strategies": {
            strategy: len(strategy_results)
            for strategy, strategy_results in by_strategy.items()
        },
        "input_file": input_file,
    }
    summary_file = os.path.join(output_dir, "summary.json")
    with open(summary_file, "w") as f:
        json.dump(summary, f, indent=2)
    print(f"\nSummary written to {summary_file}")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2]
    
    if not os.path.exists(input_file):
        print(f"Error: Input file not found: {input_file}")
        sys.exit(1)
    
    print(f"Splitting {input_file} by local search strategy...")
    split_results(input_file, output_dir)
    print("Done!")


if __name__ == "__main__":
    main()
