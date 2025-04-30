import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os
import re
from collections import defaultdict

def parse_simulation_output(filename):
    """Parse a simulation output file and extract relevant metrics."""
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    # Extract cache parameters
    params = {}
    for line in lines:
        if "Set Index Bits" in line:
            params["set_bits"] = int(line.split()[3])
        elif "Associativity" in line:
            params["associativity"] = int(line.split()[2])
        elif "Block Bits" in line:
            params["block_bits"] = int(line.split()[3])
        elif "Cache Size" in line:
            params["cache_size"] = float(line.split()[6])
            
    # Extract core statistics
    core_stats = []
    core_lines = []
    for i in range(4):
        core = f"Core {i} Statistics:"
        stats = {}
        for line in lines:
            if line.startswith(core):
                core_lines = line.split()
                stats["instructions"] = int(core_lines[3])
                stats["reads"] = int(core_lines[6])
                stats["writes"] = int(core_lines[9])
                stats["execution_cycles"] = int(core_lines[12])
                stats["idle_cycles"] = int(core_lines[15])
                stats["cache_misses"] = int(core_lines[18])
                stats["miss_rate"] = float(core_lines[21])
                stats["bus_invalidations"] = int(core_lines[26])
                stats["data_traffic"] = int(core_lines[30])
                break
        core_stats.append(stats)
    
    # Extract overall bus statistics
    bus_stats = {}
    for line in lines:
        if "Total Bus Transactions" in line:
            bus_stats["transactions"] = int(line.split()[4])
        elif "Total Bus Traffic" in line:
            bus_stats["traffic"] = int(line.split()[5])
        elif "Maximum Execution Time" in line:
            bus_stats["max_exec_time"] = int(line.split()[5])
    
    return params, core_stats, bus_stats

def run_multiple_simulations(test_prefix, num_runs=10):
    """Run simulator multiple times and collect statistics."""
    all_stats = []
    
    for i in range(num_runs):
        output_file = f"{test_prefix}_run{i}.txt"
        os.system(f"./L1simulate -t {test_prefix} -s 6 -E 2 -b 5 -o {output_file}")
        params, core_stats, bus_stats = parse_simulation_output(output_file)
        all_stats.append({
            "params": params,
            "core_stats": core_stats,
            "bus_stats": bus_stats
        })
    
    return all_stats

def analyze_parameter_variation(test_prefix, param_name, param_values):
    """Run simulations with varying parameter values and collect results."""
    results = []
    
    for value in param_values:
        output_file = f"{test_prefix}_{param_name}_{value}.txt"
        if param_name == "cache_size":
            os.system(f"./L1simulate -t {test_prefix} -s 6 -E 2 -b 5 -o {output_file} -c {value}")
        elif param_name == "associativity":
            os.system(f"./L1simulate -t {test_prefix} -s 6 -E {value} -b 5 -o {output_file}")
        elif param_name == "block_size":
            os.system(f"./L1simulate -t {test_prefix} -s 6 -E 2 -b {value} -o {output_file}")
        
        _, _, bus_stats = parse_simulation_output(output_file)
        results.append({
            "param_value": value,
            "max_exec_time": bus_stats["max_exec_time"]
        })
    
    return results

def plot_parameter_variation(results, param_name):
    """Plot maximum execution time vs. parameter value."""
    values = [r["param_value"] for r in results]
    exec_times = [r["max_exec_time"] for r in results]
    
    plt.figure(figsize=(10, 6))
    plt.plot(values, exec_times, marker='o')
    plt.xlabel(f"{param_name}")
    plt.ylabel("Maximum Execution Time (cycles)")
    plt.title(f"Maximum Execution Time vs. {param_name}")
    plt.grid(True)
    plt.savefig(f"{param_name}_variation.pdf")
    plt.close()

def main():
    # Run multiple simulations for default parameters
    print("Running multiple simulations with default parameters...")
    default_stats = run_multiple_simulations("test1")
    
    # Analyze parameter variations
    print("Analyzing parameter variations...")
    
    # Cache size variation (4KB, 8KB, 16KB)
    cache_sizes = [4, 8, 16]
    cache_size_results = analyze_parameter_variation("test1", "cache_size", cache_sizes)
    plot_parameter_variation(cache_size_results, "Cache Size")
    
    # Associativity variation (2, 4, 8)
    associativities = [2, 4, 8]
    assoc_results = analyze_parameter_variation("test1", "associativity", associativities)
    plot_parameter_variation(assoc_results, "Associativity")
    
    # Block size variation (32B, 64B, 128B)
    block_sizes = [32, 64, 128]
    block_size_results = analyze_parameter_variation("test1", "block_size", block_sizes)
    plot_parameter_variation(block_size_results, "Block Size")
    
    # Generate statistics for default parameters
    print("\nStatistics for default parameters (10 runs):")
    
    # Core statistics
    for core in range(4):
        print(f"\nCore {core} Statistics:")
        for metric in ["miss_rate", "bus_invalidations", "data_traffic"]:
            values = [stats["core_stats"][core][metric] for stats in default_stats]
            print(f"{metric}: Mean={np.mean(values):.2f}, Std Dev={np.std(values):.2f}")
    
    # Bus statistics
    print("\nBus Statistics:")
    for metric in ["transactions", "traffic", "max_exec_time"]:
        values = [stats["bus_stats"][metric] for stats in default_stats]
        print(f"{metric}: Mean={np.mean(values):.2f}, Std Dev={np.std(values):.2f}")

if __name__ == "__main__":
    main()
