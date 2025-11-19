# Mac Report (uwu edition)

A high-performance, optimized C++ application that generates a detailed system status report for macOS. This is an **uwufied** version of the original project, featuring cute kaomoji (ᕙ(⇀‸↼‶)ᕗ), pastel colors, and adorable ASCII art while maintaining all the performance optimizations and accuracy of the original.

This tool is a native port of a Linux shell script, designed to provide accurate system metrics with a kawaii, text-based user interface.

## Features

- **uwu aesthetic**: Cute kaomoji, pastel colors, and adorable formatting ✧(｡•̀ᴗ-)✧
- **System Information**: OS version, Kernel version, Hostname
- **Network**: Machine IP, Client IP (if connected via SSH), DNS servers
- **CPU**: Processor model, Core count, and CPU Usage percentage (integer format)
- **Memory**: Real-time memory usage (Active + Wired) with visual bar graph
- **Disk**: Root partition usage with visual bar graph
- **Load Averages**: 1, 5, and 15-minute load averages with visual bar graphs
- **User Activity**: Current user, Last login time (day + 12-hour format), and System uptime
- **Integer Formatting**: All percentages and storage values displayed as integers for cleaner output
- **Lowercase Display**: All labels and text in lowercase for consistent uwu aesthetic

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
| **Execution Time** | 0.026s | 0.052s | machine_report |
| **Memory Usage** | 9.56 MB | 9.69 MB | machine_report |
| **Binary Size** | ~50 KB | ~800 KB | machine_report |

### Analysis

**Machine_report Advantages:**
- **Faster execution** (~2x faster) - Highly optimized C++ codebase with async operations
- **Lower memory footprint** - Slightly more memory efficient
- **Smaller binary** - 16x smaller executable size
- **Focused scope** - Designed specifically for macOS system reporting
- **Minimal dependencies** - Only standard system libraries
- **Detailed metrics** - Specific focus on system administration needs
- **uwu aesthetic** - Cute formatting with Japanese characters and pastel colors

**Fastfetch Advantages:**
- **Rich features** - Extensive customization, logos, and display options
- **Mature ecosystem** - Well-established project with broad platform support

### Running Benchmarks

To run the benchmark suite yourself:

```bash
./benchmark.sh
```

**Note**: Requires `fastfetch` to be installed (`brew install fastfetch`). The script will automatically compile machine_report if needed.

### Performance

Typical execution time: **~0.026 seconds** on Apple Silicon (M2/M3)

The asynchronous architecture allows the program to:
- Start processing immediately
- Fetch slow data (DNS, login info) in parallel
- Display results as soon as all data is available

## Example Output

```
╭──────────────────────────────────────────────────╮
│   ✧･ﾟ: *✧･ﾟ:* SYSTEM STATUS REPORT *:･ﾟ✧*:･ﾟ✧    │
│        uwu TR-1000 Machine Report (◕‿◕✿)         │
├──────────────────────────────────────────────────┤
│ os:             macos 26.1                       │
│ kernel:         darwin kernel version 25.1.0...  │
├──────────────────────────────────────────────────┤
│ hostname:       priyanshus-macbook-air.local     │
│ machine ip:     10.146.3.51                      │
│ client ip:      n/a                              │
│ dns ip 1:       45.112.149.2                     │
│ dns ip 2:       45.112.149.2                     │
│ user:           priyanshusharma                  │
├──────────────────────────────────────────────────┤
│ processor:      しょり apple m2                   │
│ cores:          8 cores                          │
│ hypervisor:     bare metal                       │
│ cpu usage:      26%                              │
│ load 1m:        ▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱   │
│ load 5m:        ▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱    │
│ load 15m:       ▰▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱     │
├──────────────────────────────────────────────────┤
│ volume:         きおくいき 130/228 gb [57%]        │
│ disk usage:     ▰▰▰▰▰▰▰▰▰▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱    │
├──────────────────────────────────────────────────┤
│ memory:         きおく 8/16 gib [51%]             │
│ usage:          ▰▰▰▰▰▰▰▰▰▰▰▰▰▰▱▱▱▱▱▱▱▱▱▱▱▱▱▱▱    │
├──────────────────────────────────────────────────┤
│ last login:     じこく wed 10:34 pm               │
│ uptime:          3:49                            │
╰──────────────────────────────────────────────────╯
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

