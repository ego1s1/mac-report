#!/bin/bash

# Benchmark script for machine_report vs fastfetch
# Tests execution time and resource usage

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MACHINE_REPORT="$SCRIPT_DIR/machine_report"
RESULTS_FILE="$SCRIPT_DIR/benchmark_results.txt"
NUM_RUNS=10

echo "==================================================================="
echo "  Machine Report vs Fastfetch Benchmark Suite"
echo "==================================================================="
echo ""

# Check if machine_report exists
if [ ! -f "$MACHINE_REPORT" ]; then
    echo "Error: machine_report not found. Compiling..."
    clang++ -std=c++17 -O3 -march=native -flto -o "$MACHINE_REPORT" "$SCRIPT_DIR/machine_report.cpp"
    echo "Compilation complete."
    echo ""
fi

# Check if fastfetch is installed
if ! command -v fastfetch &> /dev/null; then
    echo "Warning: fastfetch not found. Install with: brew install fastfetch"
    echo "Skipping fastfetch benchmarks."
    FASTFETCH_AVAILABLE=false
else
    FASTFETCH_AVAILABLE=true
fi

# Clear results file
> "$RESULTS_FILE"

echo "Running benchmarks with $NUM_RUNS iterations each..."
echo ""

# Function to run benchmark
run_benchmark() {
    local name=$1
    local cmd=$2
    local times=()
    
    echo "Benchmarking: $name"
    
    # Warm-up run
    eval "$cmd" > /dev/null 2>&1
    
    # Actual benchmark runs
    for i in $(seq 1 $NUM_RUNS); do
        local start=$(gdate +%s.%N 2>/dev/null || date +%s)
        eval "$cmd" > /dev/null 2>&1
        local end=$(gdate +%s.%N 2>/dev/null || date +%s)
        
        if command -v gdate &> /dev/null; then
            local elapsed=$(echo "$end - $start" | bc)
        else
            local elapsed=$((end - start))
        fi
        
        times+=($elapsed)
        printf "  Run %2d: %.3f seconds\n" $i $elapsed
    done
    
    # Calculate average
    local sum=0
    for time in "${times[@]}"; do
        sum=$(echo "$sum + $time" | bc)
    done
    local avg=$(echo "scale=3; $sum / $NUM_RUNS" | bc)
    
    echo "  Average: ${avg}s"
    echo ""
    
    echo "$name: ${avg}s" >> "$RESULTS_FILE"
}

# Benchmark machine_report
run_benchmark "machine_report" "$MACHINE_REPORT"

# Benchmark fastfetch if available
if [ "$FASTFETCH_AVAILABLE" = true ]; then
    run_benchmark "fastfetch" "fastfetch"
fi

echo "==================================================================="
echo "  Benchmark Results Summary"
echo "==================================================================="
echo ""
cat "$RESULTS_FILE"
echo ""

# Calculate speedup if both tools were tested
if [ "$FASTFETCH_AVAILABLE" = true ]; then
    MACHINE_TIME=$(grep "machine_report" "$RESULTS_FILE" | awk '{print $2}' | sed 's/s//')
    FASTFETCH_TIME=$(grep "fastfetch" "$RESULTS_FILE" | awk '{print $2}' | sed 's/s//')
    
    if command -v bc &> /dev/null; then
        SPEEDUP=$(echo "scale=2; $FASTFETCH_TIME / $MACHINE_TIME" | bc)
        echo "Speedup: ${SPEEDUP}x"
        echo ""
        
        if (( $(echo "$MACHINE_TIME < $FASTFETCH_TIME" | bc -l) )); then
            echo "✅ machine_report is faster than fastfetch!"
        else
            echo "⚠️  fastfetch is faster than machine_report"
        fi
    fi
fi

echo ""
echo "Detailed results saved to: $RESULTS_FILE"
echo ""

# Memory usage comparison
echo "==================================================================="
echo "  Memory Usage Comparison"
echo "==================================================================="
echo ""

if command -v /usr/bin/time &> /dev/null; then
    echo "machine_report memory usage:"
    /usr/bin/time -l "$MACHINE_REPORT" 2>&1 | grep "maximum resident set size" || echo "  (memory stats not available)"
    echo ""
    
    if [ "$FASTFETCH_AVAILABLE" = true ]; then
        echo "fastfetch memory usage:"
        /usr/bin/time -l fastfetch 2>&1 | grep "maximum resident set size" || echo "  (memory stats not available)"
        echo ""
    fi
fi

echo "==================================================================="
echo "  Benchmark Complete"
echo "==================================================================="
