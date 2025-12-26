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


def load_json_robust(filepath: str) -> list:
    """Load JSON file, handling truncated files gracefully."""
    try:
        with open(filepath) as f:
            return json.load(f)
    except json.JSONDecodeError:
        # Try to recover partial data from truncated JSON
        results = []
        with open(filepath) as f:
            content = f.read()
        
        # Try to find the last complete object
        # Look for pattern: }, followed by newline/space and {
        import re
        # Match complete JSON objects in array format
        pattern = r'\{\s*"params"\s*:\s*\{[^}]+\}[^}]*"result"\s*:\s*\{[^}]+\}\s*\}'
        matches = re.findall(pattern, content, re.DOTALL)
        
        for match in matches:
            try:
                obj = json.loads(match)
                results.append(obj)
            except:
                pass
        
        if results:
            print(f"  Recovered {len(results)} results from truncated JSON")
        return results


def split_results(input_file: str, base_output_dir: str, exp_type: str = None):
    """
    Split results file by local search strategy.
    
    Creates structure: base_output_dir/{strategy_name}/results_{exp_type}.json
    Also creates/updates results_complete.json in each strategy folder.
    """
    
    # Load input file (robustly)
    results = load_json_robust(input_file)
    
    if not results:
        print("  No results to split")
        return {}
    
    if not isinstance(results, list):
        results = [results]
    
    # Group by strategy
    by_strategy = defaultdict(list)
    for result in results:
        strategy = get_strategy_name(result)
        by_strategy[strategy].append(result)
    
    # Write separate files in strategy subdirectories
    for strategy, strategy_results in sorted(by_strategy.items()):
        # Create strategy directory
        strategy_dir = os.path.join(base_output_dir, strategy)
        os.makedirs(strategy_dir, exist_ok=True)
        
        # Write results for this experiment type
        if exp_type:
            output_file = os.path.join(strategy_dir, f"results_{exp_type}.json")
        else:
            output_file = os.path.join(strategy_dir, "results.json")
        
        with open(output_file, "w") as f:
            json.dump(strategy_results, f, indent=1)
        print(f"    {strategy}: {len(strategy_results)} results -> {os.path.basename(output_file)}")
        
        # Update results_complete.json for this strategy
        complete_file = os.path.join(strategy_dir, "results_complete.json")
        existing_results = []
        if os.path.exists(complete_file):
            try:
                with open(complete_file) as f:
                    existing_results = json.load(f)
                    if not isinstance(existing_results, list):
                        existing_results = [existing_results]
            except:
                existing_results = []
        
        # Merge: remove old results of same exp type, add new ones
        if exp_type:
            existing_results = [r for r in existing_results 
                              if r.get("params", {}).get("exp") != strategy_results[0].get("params", {}).get("exp")]
        existing_results.extend(strategy_results)
        
        with open(complete_file, "w") as f:
            json.dump(existing_results, f, indent=1)
    
    return by_strategy


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 split_by_strategy.py <input.json> <output_dir> [exp_type]")
        print("Example: python3 split_by_strategy.py results_ls_all.json output/results/ ls")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_dir = sys.argv[2]
    exp_type = sys.argv[3] if len(sys.argv) > 3 else None
    
    if not os.path.exists(input_file):
        print(f"Error: Input file not found: {input_file}")
        sys.exit(1)
    
    print(f"  Splitting {os.path.basename(input_file)} by strategy...")
    split_results(input_file, output_dir, exp_type)


if __name__ == "__main__":
    main()
