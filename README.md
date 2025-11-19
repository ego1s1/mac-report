# Mac Report (uwu edition)

A high-performance, optimized C++ application that generates a detailed system status report for macOS. This is an **uwufied** version of the original project, featuring cute kaomoji (ᕙ(⇀‸↼‶)ᕗ), pastel colors, and adorable ASCII art while maintaining all the performance optimizations and accuracy of the original.

This tool is a native port of a Linux shell script, designed to provide accurate system metrics with a kawaii, text-based user interface.

## Features

- **uwu aesthetic**: Cute kaomoji, pastel colors, and adorable formatting ✧(｡•̀ᴗ-)✧
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
✧･ﾟ: *✧･ﾟ:* SYSTEM STATUS REPORT *:･ﾟ✧*:･ﾟ✧
uwu TR-1000 Machine Report (◕‿◕✿)

OS:             macOS 26.1
KERNEL:         Darwin Kernel Version 25.1.0...

HOSTNAME:       Priyanshus-MacBook-Air.local
MACHINE IP:     10.146.3.51
CLIENT IP:      N/A
DNS IP 1:       45.112.149.2
USER:           priyanshusharma

PROCESSOR:      ᕙ(⇀‸↼‶)ᕗ Apple M2
CORES:          8 vCPU(s) / 1 Socket(s)
HYPERVISOR:     Bare Metal
CPU USAGE:      27.06%
LOAD 1m:        ▰▰▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱

VOLUME:         ✧(｡•̀ᴗ-)✧ 130.85/228.27 GB [57.32%]
DISK USAGE:     ▰▰▰▰▰▰▰▰▰▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱

MEMORY:         (｡◕‿◕｡) 7.94/16.00 GiB [49.60%]
USAGE:          ▰▰▰▰▰▰▰▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱

LAST LOGIN:     ⸜(｡˃ ᵕ ˂ )⸝♡ Wed Nov 19 22:34
UPTIME:          3:22
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

This is an **uwufied** version of the original project, adding cute kaomoji, pastel colors, and adorable formatting while maintaining all performance optimizations.

Original project: United States Graphics Company's shell script - https://github.com/usgraphics/usgc-machine-report

## Contributing

Contributions are welcome! Please ensure that:
- Code follows the existing style and optimization patterns
- All optimizations maintain functionality
- Changes are tested on both Apple Silicon and Intel Macs (if possible)

