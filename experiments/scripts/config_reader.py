#!/usr/bin/env python3
"""
Read experiment configuration and output shell-compatible variables.

Usage:
    python3 config_reader.py <config_file> [variable_name]

If variable_name is provided, outputs just that value.
Otherwise, outputs all runtime configuration as shell variables.
"""

import json
import sys
import os


def load_config(config_file: str) -> dict:
    """Load configuration from JSON file."""
    with open(config_file) as f:
        return json.load(f)


def get_runtime_config(config: dict) -> dict:
    """Extract runtime configuration."""
    return config.get("runtime", {})


def get_instances(config_file: str, instance_set: str) -> list:
    """Load instances from instances.json for a given set."""
    config_dir = os.path.dirname(config_file)
    instances_file = os.path.join(config_dir, "instances.json")
    
    with open(instances_file) as f:
        instances_config = json.load(f)
    
    instance_sets = instances_config.get("instance_sets", {})
    
    if instance_set not in instance_sets:
        return []
    
    set_data = instance_sets[instance_set]
    
    # Handle composed sets
    if "compose" in set_data:
        all_instances = []
        for sub_set in set_data["compose"]:
            all_instances.extend(get_instances(config_file, sub_set))
        return all_instances
    
    return set_data.get("instances", [])


def output_shell_vars(config: dict):
    """Output runtime configuration as shell variables."""
    runtime = get_runtime_config(config)
    
    # Max threads (0 = unlimited)
    max_threads = runtime.get("max_threads", 0)
    print(f"CONFIG_MAX_THREADS={max_threads}")
    
    # Timeouts
    timeouts = runtime.get("timeouts", {})
    print(f"CONFIG_TIMEOUT_EVAL={timeouts.get('eval', 0)}")
    print(f"CONFIG_TIMEOUT_LS={timeouts.get('ls', 0)}")
    print(f"CONFIG_TIMEOUT_LS_COUNT={timeouts.get('ls_count', 0)}")
    print(f"CONFIG_TIMEOUT_LS_ALL={timeouts.get('ls_all', 0)}")
    print(f"CONFIG_TIMEOUT_LS_COUNT_ALL={timeouts.get('ls_count_all', 0)}")
    print(f"CONFIG_TIMEOUT_VNS={timeouts.get('vns_figures', 0)}")
    print(f"CONFIG_TIMEOUT_VNS_COUNT={timeouts.get('vns_figures_count', 0)}")
    print(f"CONFIG_TIMEOUT_TABLES={timeouts.get('vns_tables', 0)}")
    
    # Enabled experiments
    enabled = runtime.get("enabled_experiments", {})
    print(f"CONFIG_RUN_EVAL={1 if enabled.get('eval', True) else 0}")
    print(f"CONFIG_RUN_LS={1 if enabled.get('ls', True) else 0}")
    print(f"CONFIG_RUN_LS_COUNT={1 if enabled.get('ls_count', True) else 0}")
    print(f"CONFIG_RUN_LS_ALL={1 if enabled.get('ls_all', True) else 0}")
    print(f"CONFIG_RUN_LS_COUNT_ALL={1 if enabled.get('ls_count_all', True) else 0}")
    print(f"CONFIG_RUN_VNS={1 if enabled.get('vns_figures', True) else 0}")
    print(f"CONFIG_RUN_VNS_COUNT={1 if enabled.get('vns_figures_count', True) else 0}")
    print(f"CONFIG_RUN_TABLES={1 if enabled.get('vns_tables', True) else 0}")


def get_single_value(config: dict, variable: str):
    """Get a single configuration value."""
    runtime = get_runtime_config(config)
    
    # Parse variable path
    if variable == "max_threads":
        return runtime.get("max_threads", 0)
    elif variable.startswith("timeout_"):
        exp_name = variable.replace("timeout_", "")
        return runtime.get("timeouts", {}).get(exp_name, 0)
    elif variable.startswith("enabled_"):
        exp_name = variable.replace("enabled_", "")
        return 1 if runtime.get("enabled_experiments", {}).get(exp_name, True) else 0
    else:
        return ""


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <config_file> [variable_name|instances:<set_name>]", file=sys.stderr)
        sys.exit(1)
    
    config_file = sys.argv[1]
    
    if not os.path.exists(config_file):
        print(f"Error: Config file not found: {config_file}", file=sys.stderr)
        sys.exit(1)
    
    config = load_config(config_file)
    
    if len(sys.argv) > 2:
        variable = sys.argv[2]
        
        # Handle instances query
        if variable.startswith("instances:"):
            instance_set = variable.split(":")[1]
            instances = get_instances(config_file, instance_set)
            print(" ".join(instances))
        else:
            value = get_single_value(config, variable)
            print(value)
    else:
        output_shell_vars(config)


if __name__ == "__main__":
    main()
