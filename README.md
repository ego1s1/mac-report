# Mac Report

A high-performance, optimized C++ application that generates a detailed system status report for macOS. This tool is a native port of a Linux shell script, designed to provide accurate system metrics with a clean, text-based user interface.

## Features

- **System Information**: OS version, Kernel version, Hostname
- **Network**: Machine IP, Client IP (if connected via SSH), DNS servers
- **CPU**: Processor model, Core count (Physical/Logical), Socket count, and CPU Usage percentage
- **Memory**: Real-time memory usage (Active + Wired) with visual bar graph
- **Disk**: Root partition usage with visual bar graph
- **Load Averages**: 1, 5, and 15-minute load averages with visual bar graphs
- **User Activity**: Current user, Last login time, and System uptime

## Performance Optimizations

This implementation includes several macOS-specific optimizations for maximum performance:

### Caching Strategy
- **Static System Data**: OS name, kernel version, hostname, and current user are cached on first access
- **CPU Information**: CPU model, core counts, and frequency are cached (hardware doesn't change)
- **Memory**: Total memory and page size are cached, only active/wired memory is fetched dynamically
- **DNS Servers**: Cached to avoid repeated `scutil` command executions

### Asynchronous Data Fetching
- Slow operations (DNS lookup, client IP detection, login info) run in parallel using `std::async`
- Fast operations (OS info, CPU, memory, disk) execute immediately
- Results are synchronized only when needed for display, maximizing parallelism

### Compiler Optimizations
- Compiled with `-O3` for maximum optimization
- `-march=native` for CPU-specific optimizations
- `-flto` (Link-Time Optimization) for cross-module optimizations
- All functions marked `inline` for better inlining decisions
- `constexpr` constants for compile-time evaluation

### Code Optimizations
- Pre-reserved string and vector capacities to reduce allocations
- Const references used throughout to avoid unnecessary copies
- Optimized string comparisons (character-by-character for common cases)
- Minimal system calls through aggressive caching
- Replaced `std::endl` with `'\n'` to avoid buffer flushes

## Requirements

- macOS (tested on macOS 15.6.1)
- C++17 compatible compiler (e.g., clang++)
- Standard macOS system libraries

## Compilation

To compile the program with full optimizations:

```bash
clang++ -std=c++17 -O3 -march=native -flto -o machine_report machine_report.cpp
```

For debugging (without optimizations):

```bash
clang++ -std=c++17 -g -o machine_report machine_report.cpp
```

```bash
./machine_report
```

## Benchmarks

Performance comparison against [fastfetch](https://github.com/fastfetch-cli/fastfetch), a popular system information tool.

### Test Environment
- **Hardware**: Apple M3 Pro (11 cores)
- **OS**: macOS 15.6.1
- **Compiler**: clang++ with `-O3 -march=native -flto`
- **Iterations**: 10 runs per tool (averaged)

### Results

| Metric | machine_report | fastfetch | Winner |
|--------|----------------|-----------|--------|
| **Execution Time** | 0.063s | 0.038s | fastfetch |
| **Memory Usage** | 9.56 MB | 9.69 MB | machine_report |
| **Binary Size** | ~50 KB | ~800 KB | machine_report |

### Analysis

**Fastfetch Advantages:**
- **Faster execution** (~40% faster) - Highly optimized C codebase with minimal overhead
- **Rich features** - Extensive customization, logos, and display options
- **Mature ecosystem** - Well-established project with broad platform support

**Machine_report Advantages:**
- **Lower memory footprint** - Slightly more memory efficient
- **Smaller binary** - 16x smaller executable size
- **Focused scope** - Designed specifically for macOS system reporting
- **Minimal dependencies** - Only standard system libraries
- **Detailed metrics** - Specific focus on system administration needs

### Running Benchmarks

To run the benchmark suite yourself:

```bash
./benchmark.sh
```

**Note**: Requires `fastfetch` to be installed (`brew install fastfetch`). The script will automatically compile machine_report if needed.

### Performance

Typical execution time: **~0.06 seconds** on Apple Silicon (M3 Pro)

The asynchronous architecture allows the program to:
- Start processing immediately
- Fetch slow data (DNS, login info) in parallel
- Display results as soon as all data is available

## Example Output

```
┌┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┬┐
├┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┴┤
│            SYSTEM STATUS REPORT            │
│           TR-1000 MACHINE REPORT           │
├───────────────┬────────────────────────────┤
│ OS            │ macOS 15.6.1               │
│ KERNEL        │ Darwin 24.6.0              │
├───────────────┼────────────────────────────┤
│ HOSTNAME      │ Lakshits-MacBook-Pro.local │
│ MACHINE IP    │ 00.00.0.000                │
│ CLIENT  IP    │ Local Session              │
│ DNS  IP 1     │ 192.0.2.2                  │
│ USER          │ verma                      │
├───────────────┼────────────────────────────┤
│ PROCESSOR     │ Apple M3 Pro               │
│ CORES         │ 11 vCPU(s) / 1 Socket(s)   │
│ CPU USAGE     │ 18.97%                     │
│ LOAD  1m      │ ████░░░░░░░░░░░░░░░░░░░░░░ │
├───────────────┼────────────────────────────┤
│ VOLUME        │ 360.42/460.43 GB [78.28%]  │
│ DISK USAGE    │ ████████████████████░░░░░░ │
├───────────────┼────────────────────────────┤
│ MEMORY        │ 8.39/18.00 GiB [46.59%]    │
│ USAGE         │ ████████████░░░░░░░░░░░░░░ │
├───────────────┼────────────────────────────┤
│ LAST LOGIN    │ Wed Nov 19 04:37           │
│ UPTIME        │ 10d 9h 45m                 │
└───────────────┴────────────────────────────┘
```

## Technical Details

### Memory Calculation
Memory usage follows macOS Activity Monitor conventions:
- **Used Memory** = Active Memory + Wired Memory
- **Total Memory** = Physical RAM installed

### CPU Usage
CPU usage percentage is calculated as:
```
CPU Usage = (1-minute load average / logical cores) × 100
```

### Supported Platforms
- **Apple Silicon** (M1, M2, M3, etc.)
- **Intel-based Macs**

## License

This project is licensed under the BSD 3-Clause License. See the source code header for the full license text.

## Acknowledgments

Modified from United States Graphics Company's shell script: https://github.com/usgraphics/usgc-machine-report

## Contributing

Contributions are welcome! Please ensure that:
- Code follows the existing style and optimization patterns
- All optimizations maintain functionality
- Changes are tested on both Apple Silicon and Intel Macs (if possible)

