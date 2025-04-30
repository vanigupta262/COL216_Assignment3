import subprocess
import re
import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

# Configuration
EXECUTABLE = "./L1simulate"  # Path to the simulator executable
TRACE_PREFIX = "test1"        # Trace file prefix
RESULTS_FILE = "cache_sim_results.csv"
PLOT_DIR = "plots"            # Directory to save plots

# Parameter values for independent variations
CACHE_SIZES = [4, 8, 16]      # Cache sizes in KB
ASSOCIATIVITIES = [2, 4, 8]   # Associativity values
BLOCK_SIZES = [32, 64, 128]   # Block sizes in bytes

# Default parameters
DEFAULT_CACHE_SIZE = 4
DEFAULT_ASSOCIATIVITY = 2
DEFAULT_BLOCK_SIZE = 32

# Ensure plot directory exists
if not os.path.exists(PLOT_DIR):
    os.makedirs(PLOT_DIR)

# Function to run the simulator and extract max execution time
def run_simulation(param, value, cache_size, associativity, block_size):
    cmd = [EXECUTABLE, "-t", TRACE_PREFIX, "-s", str(int(np.log2(cache_size * 1024 / (associativity * block_size)))),
           "-E", str(associativity), "-b", str(int(np.log2(block_size))), "-o", f"{param}_{value}.txt"]
    
    print(f"Running simulation with {param}={value} (CacheSize={cache_size}KB, E={associativity}, BlockSize={block_size}B)...")
    
    try:
        # Run the simulator and capture output
        subprocess.run(cmd, check=True)
        
        # Parse the output file
        with open(f"{param}_{value}.txt", 'r') as f:
            output = f.read()
            
        # Extract max execution time
        match = re.search(r"Maximum Execution Time \(cycles\): (\d+)", output)
        if not match:
            print(f"Error: Could not extract MaxExecutionTime for {param}={value}.")
            return None
        max_time = int(match.group(1))
        
        return {
            "Parameter": param,
            "Value": value,
            "MaxExecutionTime": max_time,
            "CacheSize": cache_size,
            "Associativity": associativity,
            "BlockSize": block_size
        }
    
    except subprocess.CalledProcessError as e:
        print(f"Error: Simulation failed for {param}={value}. Error: {e}")
        return None
    except Exception as e:
        print(f"Error: Unexpected error for {param}={value}. Error: {e}")
        return None

# Check if trace files exist
for i in range(4):
    trace_file = f"{TRACE_PREFIX}_proc{i}.trace"
    if not os.path.exists(trace_file):
        print(f"Error: Trace file {trace_file} not found.")
        exit(1)

# Check if executable exists
if not os.path.exists(EXECUTABLE):
    print(f"Error: Simulator executable {EXECUTABLE} not found.")
    exit(1)

# Collect results
results = []

# Vary cache size
for size in CACHE_SIZES:
    result = run_simulation("CacheSize", size, size, DEFAULT_ASSOCIATIVITY, DEFAULT_BLOCK_SIZE)
    if result:
        results.append(result)

# Vary associativity
for assoc in ASSOCIATIVITIES:
    result = run_simulation("Associativity", assoc, DEFAULT_CACHE_SIZE, assoc, DEFAULT_BLOCK_SIZE)
    if result:
        results.append(result)

# Vary block size
for block in BLOCK_SIZES:
    result = run_simulation("BlockSize", block, DEFAULT_CACHE_SIZE, DEFAULT_ASSOCIATIVITY, block)
    if result:
        results.append(result)

# Save results to CSV
if results:
    df = pd.DataFrame(results)
    df.to_csv(RESULTS_FILE, index=False)
    print(f"Results saved to {RESULTS_FILE}")
else:
    print("No results to save. Exiting.")
    exit(1)

# Plotting
# Split data by parameter
cache_size_data = df[df['Parameter'] == 'CacheSize']
associativity_data = df[df['Parameter'] == 'Associativity']
block_size_data = df[df['Parameter'] == 'BlockSize']

# Plot 1: Maximum Execution Time vs. Cache Size
plt.figure(figsize=(8, 6))
plt.plot(cache_size_data['Value'], cache_size_data['MaxExecutionTime'], marker='o', label='Cache Size')
plt.xlabel('Cache Size (KB)')
plt.ylabel('Maximum Execution Time (cycles)')
plt.title('Maximum Execution Time vs. Cache Size')
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(PLOT_DIR, 'cache_size_variation.png'))
plt.close()

# Plot 2: Maximum Execution Time vs. Associativity
plt.figure(figsize=(8, 6))
plt.plot(associativity_data['Value'], associativity_data['MaxExecutionTime'], marker='s', label='Associativity')
plt.xlabel('Associativity')
plt.ylabel('Maximum Execution Time (cycles)')
plt.title('Maximum Execution Time vs. Associativity')
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(PLOT_DIR, 'associativity_variation.png'))
plt.close()

# Plot 3: Maximum Execution Time vs. Block Size
plt.figure(figsize=(8, 6))
plt.plot(block_size_data['Value'], block_size_data['MaxExecutionTime'], marker='^', label='Block Size')
plt.xlabel('Block Size (bytes)')
plt.ylabel('Maximum Execution Time (cycles)')
plt.title('Maximum Execution Time vs. Block Size')
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(PLOT_DIR, 'block_size_variation.png'))
plt.close()

print(f"Plots saved in {PLOT_DIR}/: cache_size_variation.png, associativity_variation.png, block_size_variation.png")

if __name__ == "__main__":
    # Configuration
    EXECUTABLE = "./L1simulate"  # Path to the simulator executable
    TRACE_PREFIX = "test1"        # Trace file prefix
    RESULTS_FILE = "cache_sim_results.csv"
    PLOT_DIR = "plots"            # Directory to save plots
    
    # Parameter values for independent variations
    CACHE_SIZES = [4, 8, 16]      # Cache sizes in KB
    ASSOCIATIVITIES = [2, 4, 8]   # Associativity values
    BLOCK_SIZES = [32, 64, 128]   # Block sizes in bytes
    
    # Default parameters
    DEFAULT_CACHE_SIZE = 4
    DEFAULT_ASSOCIATIVITY = 2
    DEFAULT_BLOCK_SIZE = 32
    
    # Ensure plot directory exists
    if not os.path.exists(PLOT_DIR):
        os.makedirs(PLOT_DIR)
    
    # Function to run the simulator and extract max execution time
    def run_simulation(param, value, cache_size, associativity, block_size):
        cmd = [EXECUTABLE, "-t", TRACE_PREFIX, "-s", str(int(np.log2(cache_size * 1024 / (associativity * block_size)))),
               "-E", str(associativity), "-b", str(int(np.log2(block_size))), "-o", f"{param}_{value}.txt"]
        
        print(f"Running simulation with {param}={value} (CacheSize={cache_size}KB, E={associativity}, BlockSize={block_size}B)...")
        
        try:
            # Run the simulator and capture output
            subprocess.run(cmd, check=True)
            
            # Parse the output file
            with open(f"{param}_{value}.txt", 'r') as f:
                output = f.read()
                
            # Extract max execution time
            match = re.search(r"Maximum Execution Time \(cycles\): (\d+)", output)
            if not match:
                print(f"Error: Could not extract MaxExecutionTime for {param}={value}.")
                return None
            max_time = int(match.group(1))
            
            return {
                "Parameter": param,
                "Value": value,
                "MaxExecutionTime": max_time,
                "CacheSize": cache_size,
                "Associativity": associativity,
                "BlockSize": block_size
            }
        
        except subprocess.CalledProcessError as e:
            print(f"Error: Simulation failed for {param}={value}. Error: {e}")
            return None
        except Exception as e:
            print(f"Error: Unexpected error for {param}={value}. Error: {e}")
            return None
    
    # Check if trace files exist
    for i in range(4):
        trace_file = f"{TRACE_PREFIX}_proc{i}.trace"
        if not os.path.exists(trace_file):
            print(f"Error: Trace file {trace_file} not found.")
            exit(1)
    
    # Check if executable exists
    if not os.path.exists(EXECUTABLE):
        print(f"Error: Simulator executable {EXECUTABLE} not found.")
        exit(1)
    
    # Collect results
    results = []
    
    # Vary cache size
    for size in CACHE_SIZES:
        result = run_simulation("CacheSize", size, size, DEFAULT_ASSOCIATIVITY, DEFAULT_BLOCK_SIZE)
        if result:
            results.append(result)
    
    # Vary associativity
    for assoc in ASSOCIATIVITIES:
        result = run_simulation("Associativity", assoc, DEFAULT_CACHE_SIZE, assoc, DEFAULT_BLOCK_SIZE)
        if result:
            results.append(result)
    
    # Vary block size
    for block in BLOCK_SIZES:
        result = run_simulation("BlockSize", block, DEFAULT_CACHE_SIZE, DEFAULT_ASSOCIATIVITY, block)
        if result:
            results.append(result)
    
    # Save results to CSV
    if results:
        df = pd.DataFrame(results)
        df.to_csv(RESULTS_FILE, index=False)
        print(f"Results saved to {RESULTS_FILE}")
    else:
        print("No results to save. Exiting.")
        exit(1)
    
    # Plotting
    # Split data by parameter
    cache_size_data = df[df['Parameter'] == 'CacheSize']
    associativity_data = df[df['Parameter'] == 'Associativity']
    block_size_data = df[df['Parameter'] == 'BlockSize']
    
    # Plot 1: Maximum Execution Time vs. Cache Size
    plt.figure(figsize=(8, 6))
    plt.plot(cache_size_data['Value'], cache_size_data['MaxExecutionTime'], marker='o', label='Cache Size')
    plt.xlabel('Cache Size (KB)')
    plt.ylabel('Maximum Execution Time (cycles)')
    plt.title('Maximum Execution Time vs. Cache Size')
    plt.grid(True)
    plt.legend()
    plt.savefig(os.path.join(PLOT_DIR, 'cache_size_variation.png'))
    plt.close()
    
    # Plot 2: Maximum Execution Time vs. Associativity
    plt.figure(figsize=(8, 6))
    plt.plot(associativity_data['Value'], associativity_data['MaxExecutionTime'], marker='s', label='Associativity')
    plt.xlabel('Associativity')
    plt.ylabel('Maximum Execution Time (cycles)')
    plt.title('Maximum Execution Time vs. Associativity')
    plt.grid(True)
    plt.legend()
    plt.savefig(os.path.join(PLOT_DIR, 'associativity_variation.png'))
    plt.close()
    
    # Plot 3: Maximum Execution Time vs. Block Size
    plt.figure(figsize=(8, 6))
    plt.plot(block_size_data['Value'], block_size_data['MaxExecutionTime'], marker='^', label='Block Size')
    plt.xlabel('Block Size (bytes)')
    plt.ylabel('Maximum Execution Time (cycles)')
    plt.title('Maximum Execution Time vs. Block Size')
    plt.grid(True)
    plt.legend()
    plt.savefig(os.path.join(PLOT_DIR, 'block_size_variation.png'))
    plt.close()
    
    print(f"Plots saved in {PLOT_DIR}/: cache_size_variation.png, associativity_variation.png, block_size_variation.png")