/*
 * Copyright (c) 2025, Lakshit Verma
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <future>
#include <ifaddrs.h>
#include <iomanip>
#include <iostream>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sstream>
#include <string>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// Cute pastel color constants
constexpr const char* PINK = "\033[38;5;213m";
constexpr const char* CYAN = "\033[38;5;159m";
constexpr const char* PURPLE = "\033[38;5;183m";
constexpr const char* YELLOW = "\033[38;5;229m";
constexpr const char* GREEN = "\033[38;5;156m";
constexpr const char* BLUE = "\033[38;5;117m";
constexpr const char* RESET = "\033[0m";
constexpr const char* BOLD = "\033[1m";

// Cute kaomoji/emoji
constexpr const char* KAWAII_CPU = "ᕙ(⇀‸↼‶)ᕗ";
constexpr const char* KAWAII_MEM = "(｡◕‿◕｡)";
constexpr const char* KAWAII_DISK = "✧(｡•̀ᴗ-)✧";
constexpr const char* KAWAII_NET = "(◕‿◕✿)";
constexpr const char* KAWAII_TIME = "⸜(｡˃ ᵕ ˂ )⸝♡";

// Your existing constants
constexpr int MIN_NAME_LEN = 5;
constexpr int MAX_NAME_LEN = 13;
constexpr int MIN_DATA_LEN = 20;
constexpr int MAX_DATA_LEN = 32;
constexpr int BORDERS_AND_PADDING = 7;
constexpr const char *REPORT_TITLE = "SYSTEM STATUS REPORT";

// Function to execute shell command
inline std::string execCommand(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  result.reserve(256);
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
    return "";
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }
  return result;
}

inline size_t getDisplayWidth(const std::string& str) {
  size_t width = 0;
  for (size_t i = 0; i < str.length(); ) {
    unsigned char c = static_cast<unsigned char>(str[i]);
    if (c == 0x1B) {
      size_t j = i + 1;
      if (j < str.length() && str[j] == '[') {
        j++;
        while (j < str.length()) {
          unsigned char ch = static_cast<unsigned char>(str[j]);
          if (ch >= 0x40 && ch <= 0x7E) {
            i = j + 1;
            break;
          }
          if (ch < 0x20 || ch > 0x3F) {
            break;
          }
          j++;
        }
        if (j >= str.length()) {
          i = str.length();
        }
        continue;
      }
      width += 1;
      i += 1;
    } else if ((c & 0x80) == 0) {
      width += 1;
      i += 1;
    } else if ((c & 0xE0) == 0xC0) {
      if (i + 1 < str.length()) {
        unsigned char c1 = static_cast<unsigned char>(str[i + 1]);
        if ((c == 0xC2 && c1 >= 0xA1 && c1 <= 0xAF) || (c == 0xC3 && c1 >= 0x80 && c1 <= 0xBF)) {
          width += 1;
        } else {
          width += 2;
        }
      } else {
        width += 2;
      }
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {
      width += 1;
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {
      width += 2;
      i += 4;
    } else {
      width += 1;
      i += 1;
    }
  }
  return width;
}

inline size_t maxLength(const std::vector<std::string> &strings) {
  size_t max_len = MIN_DATA_LEN;
  for (const auto &str : strings) {
    const size_t len = getDisplayWidth(str);
    if (len > max_len) {
      max_len = len;
    }
  }
  if (max_len > MAX_DATA_LEN) {
    max_len = MAX_DATA_LEN;
  }
  if (max_len < MIN_DATA_LEN) {
    max_len = MIN_DATA_LEN;
  }
  return max_len;
}

inline std::string formatBytes(uint64_t bytes) {
  const double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << gb;
  return ss.str();
}

inline std::string formatGiB(uint64_t bytes) {
  const double gib = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2) << gib;
  return ss.str();
}

// Gradient bar graph with blocks, cute dim
inline std::string drawBarGraph(double percent, int width) {
  const int num_blocks = static_cast<int>((percent / 100.0) * width);
  std::string graph;
  graph.reserve(width * 8);
  const char* bar_color;
  if (percent < 50) {
    bar_color = GREEN;
  } else if (percent < 75) {
    bar_color = YELLOW;
  } else {
    bar_color = PINK;
  }
  graph += bar_color;
  for (int i = 0; i < num_blocks; ++i)
    graph += "▰";
  graph += RESET;
  graph += "\033[2m";
  for (int i = num_blocks; i < width; ++i)
    graph += "▱";
  graph += RESET;
  return graph;
}

// Cutified: still efficient
inline void printHeader(int current_len) {
}

inline void printCenteredData(const std::string &text, int current_len, const char* color = CYAN) {
  std::cout << color << BOLD << text << RESET << "\n";
}

// Cute dividers
inline void printDivider(const std::string &side, int current_len) {
  std::cout << '\n';
}

inline void printData(std::string name, std::string data, int current_len,
                     const char* color = RESET, const char* emoji = "") {
  if (name.length() > MAX_NAME_LEN) {
    name = name.substr(0, MAX_NAME_LEN - 3) + "...";
  }
  
  const bool is_graph = (data.find("█") != std::string::npos ||
                         data.find("░") != std::string::npos ||
                         data.find("▰") != std::string::npos);

  size_t name_display_width = getDisplayWidth(name);
  const size_t label_width = MAX_NAME_LEN;
  size_t name_padding = (name_display_width < label_width) ? (label_width - name_display_width) : 0;

  std::string emoji_str = (emoji && *emoji) ? std::string(emoji) + " " : "";
  
  if (is_graph) {
    std::cout << color << BOLD << name << RESET << ":"
              << std::string(name_padding, ' ') << "  " << emoji_str << data << "\n";
  } else {
    size_t data_display_width = getDisplayWidth(data);
    if (data_display_width >= MAX_DATA_LEN) {
      data = data.substr(0, MAX_DATA_LEN - 4) + "...";
    }
    std::cout << color << BOLD << name << RESET << ":"
              << std::string(name_padding, ' ') << "  " << emoji_str << data << "\n";
  }
}

struct CPUInfo {
  std::string model;
  int cores_physical;
  int cores_logical;
  int sockets;
  double load_1;
  double load_5;
  double load_15;
};

struct MemInfo {
  uint64_t total;
  uint64_t used;
  double percent;
};

struct DiskInfo {
  uint64_t total;
  uint64_t used;
  double percent;
};

struct LoginInfo {
  std::string time;
  std::string ip;
  bool ip_present;
  std::string uptime;
};

inline std::string getOSName() {
  std::string product_name = execCommand("sw_vers -productName");
  std::string product_version = execCommand("sw_vers -productVersion");
  if (!product_name.empty() && !product_version.empty()) {
    return product_name + " " + product_version;
  }

  size_t size = 0;
  sysctlbyname("kern.ostype", nullptr, &size, nullptr, 0);
  std::string os_type = "macOS";
  if (size > 1) {
    std::vector<char> buffer(size);
    sysctlbyname("kern.ostype", buffer.data(), &size, nullptr, 0);
    if (size > 0 && buffer[size - 1] == '\0') {
      os_type = std::string(buffer.data());
    }
  }

  size = 0;
  sysctlbyname("kern.osrelease", nullptr, &size, nullptr, 0);
  std::string os_release = "";
  if (size > 1) {
    std::vector<char> buffer(size);
    sysctlbyname("kern.osrelease", buffer.data(), &size, nullptr, 0);
    if (size > 0 && buffer[size - 1] == '\0') {
      os_release = std::string(buffer.data());
    }
  }
  return os_type + " " + os_release;
}

inline std::string getKernelVersion() {
  size_t size = 0;
  sysctlbyname("kern.version", nullptr, &size, nullptr, 0);
  if (size > 1) {
    std::vector<char> buffer(size);
    sysctlbyname("kern.version", buffer.data(), &size, nullptr, 0);
    if (size > 0) {
      std::string version(buffer.data());
      size_t newline = version.find('\n');
      if (newline != std::string::npos) {
        version = version.substr(0, newline);
      }
      return version;
    }
  }
  return "unknown";
}

inline std::string getHostname() {
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) == 0) {
    return std::string(hostname);
  }
  return "unknown";
}

inline std::string getMachineIP() {
  struct ifaddrs *ifaddrs_ptr;
  if (getifaddrs(&ifaddrs_ptr) != 0) {
    return "unknown";
  }

  std::string ip;
  for (struct ifaddrs *ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr) continue;
    if (ifa->ifa_addr->sa_family != AF_INET) continue;
    if (std::string(ifa->ifa_name).find("lo") == 0) continue;

    struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &sin->sin_addr, ip_str, INET_ADDRSTRLEN) != nullptr) {
      ip = std::string(ip_str);
      break;
    }
  }
  freeifaddrs(ifaddrs_ptr);
  return ip.empty() ? "unknown" : ip;
}

inline std::string getClientIP() {
  const char *ssh_client = getenv("SSH_CLIENT");
  if (ssh_client != nullptr) {
    std::string client(ssh_client);
    size_t space = client.find(' ');
    if (space != std::string::npos) {
      return client.substr(0, space);
    }
    return client;
  }
  return "N/A";
}

inline std::vector<std::string> getDNS() {
  std::vector<std::string> dns_servers;
  std::string output = execCommand("scutil --dns | grep 'nameserver\\[0\\]' | head -3");
  if (!output.empty()) {
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
      size_t colon = line.find(':');
      if (colon != std::string::npos) {
        std::string ip = line.substr(colon + 1);
        while (!ip.empty() && ip[0] == ' ') ip.erase(0, 1);
        if (!ip.empty()) {
          dns_servers.push_back(ip);
        }
      }
    }
  }
  if (dns_servers.empty()) {
    dns_servers.push_back("N/A");
  }
  return dns_servers;
}

inline std::string getCurrentUser() {
  struct passwd *pw = getpwuid(getuid());
  if (pw != nullptr && pw->pw_name != nullptr) {
    return std::string(pw->pw_name);
  }
  return "unknown";
}

inline CPUInfo getCPUInfo() {
  CPUInfo info;

  size_t size = 0;
  sysctlbyname("machdep.cpu.brand_string", nullptr, &size, nullptr, 0);
  if (size > 1) {
    std::vector<char> buffer(size);
    sysctlbyname("machdep.cpu.brand_string", buffer.data(), &size, nullptr, 0);
    if (size > 0) {
      info.model = std::string(buffer.data());
    } else {
      info.model = "Unknown CPU";
    }
  } else {
    info.model = "Unknown CPU";
  }

  size = sizeof(int);
  sysctlbyname("hw.physicalcpu", &info.cores_physical, &size, nullptr, 0);
  sysctlbyname("hw.logicalcpu", &info.cores_logical, &size, nullptr, 0);
  sysctlbyname("hw.packages", &info.sockets, &size, nullptr, 0);

  struct loadavg load;
  size = sizeof(load);
  if (sysctlbyname("vm.loadavg", &load, &size, nullptr, 0) == 0) {
    info.load_1 = static_cast<double>(load.ldavg[0]) / static_cast<double>(load.fscale);
    info.load_5 = static_cast<double>(load.ldavg[1]) / static_cast<double>(load.fscale);
    info.load_15 = static_cast<double>(load.ldavg[2]) / static_cast<double>(load.fscale);
  } else {
    info.load_1 = info.load_5 = info.load_15 = 0.0;
  }

  return info;
}

inline MemInfo getMemInfo() {
  MemInfo info;
  vm_size_t page_size;
  vm_statistics64_data_t vm_stat;
  mach_msg_type_number_t host_size = sizeof(vm_statistics64_data_t) / sizeof(natural_t);

  host_page_size(mach_host_self(), &page_size);

  if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &host_size) == KERN_SUCCESS) {
    uint64_t total_mem;
    size_t size = sizeof(total_mem);
    sysctlbyname("hw.memsize", &total_mem, &size, nullptr, 0);

    uint64_t used_mem = (vm_stat.active_count + vm_stat.wire_count) * page_size;
    info.total = total_mem;
    info.used = used_mem;
    info.percent = (static_cast<double>(used_mem) / static_cast<double>(total_mem)) * 100.0;
  } else {
    info.total = info.used = 0;
    info.percent = 0.0;
  }

  return info;
}

inline DiskInfo getDiskInfo() {
  DiskInfo info;
  struct statfs fs;
  if (statfs("/", &fs) == 0) {
    uint64_t total_bytes = fs.f_blocks * fs.f_bsize;
    uint64_t free_bytes = fs.f_bavail * fs.f_bsize;
    uint64_t used_bytes = total_bytes - free_bytes;

    info.total = total_bytes;
    info.used = used_bytes;
    info.percent = (static_cast<double>(used_bytes) / static_cast<double>(total_bytes)) * 100.0;
  } else {
    info.total = info.used = 0;
    info.percent = 0.0;
  }

  return info;
}

inline LoginInfo getLastLogin() {
  LoginInfo info;
  std::string last_cmd = execCommand("last -1 -t console | head -1");
  if (!last_cmd.empty()) {
    std::istringstream iss(last_cmd);
    std::string token;
    std::vector<std::string> tokens;
    while (iss >> token) {
      tokens.push_back(token);
    }
    if (tokens.size() >= 6) {
      info.time = tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + tokens[5];
      info.ip_present = false;
    } else if (tokens.size() >= 4) {
      info.time = tokens[2] + " " + tokens[3];
      if (tokens.size() >= 5) {
        info.time += " " + tokens[4];
      }
      info.ip_present = false;
    } else {
      info.time = "N/A";
      info.ip_present = false;
    }
  } else {
    info.time = "N/A";
    info.ip_present = false;
  }

  std::string uptime_cmd = execCommand("uptime | sed 's/.*up \\([^,]*\\).*/\\1/'");
  if (!uptime_cmd.empty()) {
    info.uptime = uptime_cmd;
  } else {
    info.uptime = "N/A";
  }

  return info;
}

int main() {
  auto future_dns = std::async(std::launch::async, getDNS);
  auto future_client_ip = std::async(std::launch::async, getClientIP);
  auto future_login = std::async(std::launch::async, getLastLogin);

  const std::string os_name = getOSName();
  const std::string os_kernel = getKernelVersion();
  const std::string net_hostname = getHostname();
  const std::string net_machine_ip = getMachineIP();
  const std::string net_current_user = getCurrentUser();

  const CPUInfo cpu = getCPUInfo();
  const MemInfo mem = getMemInfo();
  const DiskInfo disk = getDiskInfo();

  const std::vector<std::string> net_dns_ip = future_dns.get();
  const std::string net_client_ip = future_client_ip.get();
  const LoginInfo login = future_login.get();

  const std::string cpu_cores_str = std::to_string(cpu.cores_physical) +
                                    " vCPU(s) / " +
                                    std::to_string(cpu.sockets) + " Socket(s)";

  const double usage_percent = (cpu.load_1 / cpu.cores_logical) * 100.0;
  std::stringstream usage_ss;
  usage_ss << std::fixed << std::setprecision(2) << usage_percent << "%";
  const std::string cpu_usage_str = usage_ss.str();

  std::stringstream mem_str_ss;
  mem_str_ss << formatGiB(mem.used) << "/" << formatGiB(mem.total) << " GiB ["
             << std::fixed << std::setprecision(2) << mem.percent << "%]";
  const std::string mem_usage_str = mem_str_ss.str();

  std::stringstream disk_str_ss;
  disk_str_ss << formatBytes(disk.used) << "/" << formatBytes(disk.total)
              << " GB [" << std::fixed << std::setprecision(2) << disk.percent
              << "%]";
  const std::string disk_usage_str = disk_str_ss.str();

  std::string cpu_model_with_kaomoji = std::string(KAWAII_CPU) + " " + cpu.model;
  std::string disk_usage_with_kaomoji = std::string(KAWAII_DISK) + " " + disk_usage_str;
  std::string mem_usage_with_kaomoji = std::string(KAWAII_MEM) + " " + mem_usage_str;
  std::string login_time_with_kaomoji = std::string(KAWAII_TIME) + " " + login.time;

  std::vector<std::string> all_strings = {
      REPORT_TITLE,   os_name,       os_kernel,        net_hostname,
      net_machine_ip, net_client_ip, net_current_user, cpu_model_with_kaomoji,
      cpu_cores_str,  "Bare Metal",  cpu_usage_str,    mem_usage_with_kaomoji,
      disk_usage_with_kaomoji, login_time_with_kaomoji, login.ip, login.uptime};

  const int current_len = maxLength(all_strings);

  int graph_width = current_len;
  if (graph_width > MAX_DATA_LEN - 3) {
    graph_width = MAX_DATA_LEN - 3;
  }

  const std::string cpu_1_graph =
      drawBarGraph((cpu.load_1 / cpu.cores_logical) * 100.0, graph_width);
  const std::string cpu_5_graph =
      drawBarGraph((cpu.load_5 / cpu.cores_logical) * 100.0, graph_width);
  const std::string cpu_15_graph =
      drawBarGraph((cpu.load_15 / cpu.cores_logical) * 100.0, graph_width);

  const std::string mem_graph = drawBarGraph(mem.percent, graph_width);
  const std::string disk_graph = drawBarGraph(disk.percent, graph_width);

  printHeader(current_len);
  printCenteredData("✧･ﾟ: *✧･ﾟ:* SYSTEM STATUS REPORT *:･ﾟ✧*:･ﾟ✧", current_len, PINK);
  printCenteredData("uwu TR-1000 Machine Report (◕‿◕✿)", current_len, CYAN);
  printDivider("top", current_len);

  printData("OS", os_name, current_len, CYAN, "");
  printData("KERNEL", os_kernel, current_len, CYAN, "");
  printDivider("", current_len);

  printData("HOSTNAME", net_hostname, current_len, BLUE, "");
  printData("MACHINE IP", net_machine_ip, current_len, BLUE, "");
  printData("CLIENT IP", net_client_ip, current_len, BLUE, "");
  for (size_t i = 0; i < net_dns_ip.size(); ++i) {
    printData("DNS IP " + std::to_string(i + 1), net_dns_ip[i], current_len, BLUE, "");
  }
  printData("USER", net_current_user, current_len, PURPLE, "");
  printDivider("", current_len);

  printData("PROCESSOR", cpu.model, current_len, YELLOW, KAWAII_CPU);
  printData("CORES", cpu_cores_str, current_len, YELLOW, "");
  printData("HYPERVISOR", "Bare Metal", current_len, YELLOW, "");
  printData("CPU USAGE", cpu_usage_str, current_len, YELLOW, "");
  printData("LOAD 1m", cpu_1_graph, current_len, GREEN, "");
  printData("LOAD 5m", cpu_5_graph, current_len, GREEN, "");
  printData("LOAD 15m", cpu_15_graph, current_len, GREEN, "");
  printDivider("", current_len);

  printData("VOLUME", disk_usage_str, current_len, PINK, KAWAII_DISK);
  printData("DISK USAGE", disk_graph, current_len, PINK, "");
  printDivider("", current_len);

  printData("MEMORY", mem_usage_str, current_len, PURPLE, KAWAII_MEM);
  printData("USAGE", mem_graph, current_len, PURPLE, "");
  printDivider("", current_len);

  printData("LAST LOGIN", login.time, current_len, CYAN, KAWAII_TIME);
  printData("UPTIME", login.uptime, current_len, GREEN, "");

  printDivider("bottom", current_len);

  return 0;
}

// Copy all unchanged helpers (getOSName, getDiskInfo, getDNS etc.) below this
