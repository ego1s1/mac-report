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
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sstream>
#include <string>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

constexpr int MIN_NAME_LEN = 5;
constexpr int MAX_NAME_LEN = 13;
constexpr int MIN_DATA_LEN = 20;
constexpr int MAX_DATA_LEN = 32;
constexpr int BORDERS_AND_PADDING = 7;
constexpr const char *REPORT_TITLE = "SYSTEM STATUS REPORT";

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

inline size_t maxLength(const std::vector<std::string> &strings) {
  size_t max_len = 0;
  for (const auto &str : strings) {
    const size_t len = str.length();
    if (len > max_len) {
      max_len = len;
    }
  }
  return (max_len < MAX_DATA_LEN) ? max_len : MAX_DATA_LEN;
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

inline std::string drawBarGraph(double percent, int width) {
  const int num_blocks = static_cast<int>((percent / 100.0) * width);
  std::string graph;
  graph.reserve(width * 3);
  for (int i = 0; i < num_blocks; ++i)
    graph += "█";
  for (int i = num_blocks; i < width; ++i)
    graph += "░";
  return graph;
}

inline void printHeader(int current_len) {
  const int length = current_len + MAX_NAME_LEN + BORDERS_AND_PADDING;
  std::string top = "┌";
  std::string bottom = "├";
  top.reserve(length);
  bottom.reserve(length);
  for (int i = 0; i < length - 2; ++i) {
    top += "┬";
    bottom += "┴";
  }
  top += "┐";
  bottom += "┤";
  std::cout << top << '\n' << bottom << '\n';
}

inline void printCenteredData(const std::string &text, int current_len) {
  const int max_len = current_len + MAX_NAME_LEN - BORDERS_AND_PADDING;
  const int total_width = max_len + 12;
  const int padding_left = (total_width - text.length()) / 2;
  const int padding_right = total_width - text.length() - padding_left;

  std::cout << "│" << std::string(padding_left, ' ') << text
            << std::string(padding_right, ' ') << "│\n";
}

inline void printDivider(const std::string &side, int current_len) {
  const char *left_symbol, *middle_symbol, *right_symbol;
  if (side == "top") {
    left_symbol = "├";
    middle_symbol = "┬";
    right_symbol = "┤";
  } else if (side == "bottom") {
    left_symbol = "└";
    middle_symbol = "┴";
    right_symbol = "┘";
  } else {
    left_symbol = "├";
    middle_symbol = "┼";
    right_symbol = "┤";
  }

  const int length = current_len + MAX_NAME_LEN + BORDERS_AND_PADDING;
  std::string divider = left_symbol;
  divider.reserve(length);
  for (int i = 0; i < length - 3; ++i) {
    divider += "─";
    if (i == 14)
      divider += middle_symbol;
  }
  divider += right_symbol;
  std::cout << divider << '\n';
}

inline void printData(std::string name, std::string data, int current_len) {
  if (name.length() > MAX_NAME_LEN) {
    name = name.substr(0, MAX_NAME_LEN - 3) + "...";
  }

  if (name.length() < MAX_NAME_LEN) {
    name.resize(MAX_NAME_LEN, ' ');
  }

  const bool is_graph = (data.find("█") != std::string::npos ||
                         data.find("░") != std::string::npos);

  if (is_graph) {
    const size_t num_blocks = data.length() / 3;
    if (num_blocks < static_cast<size_t>(current_len)) {
      data += std::string(current_len - num_blocks, ' ');
    }
  } else {
    if (data.length() >= MAX_DATA_LEN) {
      data = data.substr(0, MAX_DATA_LEN - 4) + "...";
    } else if (data.length() < static_cast<size_t>(current_len)) {
      data.resize(current_len, ' ');
    }
  }

  std::cout << "│ " << name << " │ " << data << " │\n";
}

inline std::string getOSName() {
  static std::string cached_os_name;
  if (cached_os_name.empty()) {
    const std::string name = execCommand("sw_vers -productName");
    const std::string version = execCommand("sw_vers -productVersion");
    cached_os_name = name + " " + version;
  }
  return cached_os_name;
}

inline std::string getKernelVersion() {
  static std::string cached_kernel;
  if (cached_kernel.empty()) {
    char str[256];
    size_t size = sizeof(str);
    cached_kernel = "Darwin";
    if (sysctlbyname("kern.osrelease", str, &size, NULL, 0) == 0) {
      cached_kernel += " ";
      cached_kernel += str;
    }
  }
  return cached_kernel;
}

inline std::string getHostname() {
  static std::string cached_hostname;
  if (cached_hostname.empty()) {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
      cached_hostname = hostname;
    } else {
      cached_hostname = "Not Defined";
    }
  }
  return cached_hostname;
}

inline std::string getMachineIP() {
  struct ifaddrs *ifaddr, *ifa;
  std::string ip = "No IP found";

  if (getifaddrs(&ifaddr) == -1)
    return ip;

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    const char *iface = ifa->ifa_name;
    if (iface[0] == 'l' && iface[1] == 'o' && iface[2] == '0')
      continue;
    if (iface[0] == 'd' && iface[1] == 'o' && iface[2] == 'c')
      continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      char host[NI_MAXHOST];
      if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                      NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) {
        if (host[0] != '1' || host[1] != '2' || host[2] != '7') {
          ip = host;
          break;
        }
      }
    }
  }
  freeifaddrs(ifaddr);
  return ip;
}

inline std::string getClientIP() {
  const char *ssh_client = getenv("SSH_CLIENT");
  if (ssh_client) {
    std::stringstream ss(ssh_client);
    std::string ip;
    ss >> ip;
    return ip;
  }

  const std::string output = execCommand("who am i");
  if (output.empty())
    return "Not connected";

  const size_t start = output.find('(');
  const size_t end = output.find(')');
  if (start != std::string::npos && end != std::string::npos && end > start) {
    return output.substr(start + 1, end - start - 1);
  }

  return "Local Session";
}

inline std::vector<std::string> getDNS() {
  static std::vector<std::string> cached_dns;
  if (cached_dns.empty()) {
    cached_dns.reserve(3);
    const std::string output =
        execCommand("scutil --dns | grep 'nameserver\\[0\\]' | sed "
                    "'s/.*nameserver\\[0\\] : \\([0-9.]*\\).*/\\1/' | head -3");
    std::stringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
      if (!line.empty())
        cached_dns.push_back(line);
    }
  }
  return cached_dns;
}

struct CPUInfo {
  std::string model;
  int cores_physical;
  int sockets;
  double freq_ghz;
  double load_1, load_5, load_15;
  int cores_logical;
};

struct CPUStaticInfo {
  std::string model;
  int cores_physical;
  int sockets;
  double freq_ghz;
  int cores_logical;
};

inline CPUStaticInfo getCPUStaticInfo() {
  static CPUStaticInfo cached_info{};
  static bool initialized = false;

  if (!initialized) {
    char str[256];
    size_t size = sizeof(str);
    int val;
    size_t int_size = sizeof(val);
    int64_t freq = 0;
    size_t freq_size = sizeof(freq);

    if (sysctlbyname("machdep.cpu.brand_string", str, &size, NULL, 0) == 0)
      cached_info.model = str;
    if (sysctlbyname("hw.physicalcpu", &val, &int_size, NULL, 0) == 0)
      cached_info.cores_physical = val;
    if (sysctlbyname("hw.packages", &val, &int_size, NULL, 0) == 0)
      cached_info.sockets = val;

    if (sysctlbyname("hw.cpufrequency", &freq, &freq_size, NULL, 0) != 0 ||
        freq == 0) {
      if (sysctlbyname("hw.cpufrequency_max", &freq, &freq_size, NULL, 0) !=
              0 ||
          freq == 0) {
        freq = 0;
      }
    }
    cached_info.freq_ghz = static_cast<double>(freq) / 1000000000.0;

    if (sysctlbyname("hw.ncpu", &val, &int_size, NULL, 0) == 0)
      cached_info.cores_logical = val;

    initialized = true;
  }

  return cached_info;
}

inline CPUInfo getCPUInfo() {
  const CPUStaticInfo static_info = getCPUStaticInfo();

  CPUInfo info{};
  info.model = static_info.model;
  info.cores_physical = static_info.cores_physical;
  info.sockets = static_info.sockets;
  info.freq_ghz = static_info.freq_ghz;
  info.cores_logical = static_info.cores_logical;

  double load[3];
  if (getloadavg(load, 3) != -1) {
    info.load_1 = load[0];
    info.load_5 = load[1];
    info.load_15 = load[2];
  }

  return info;
}

struct MemInfo {
  uint64_t total;
  uint64_t used;
  double percent;
};

inline MemInfo getMemInfo() {
  MemInfo info{};

  static int64_t cached_total_mem = 0;
  if (cached_total_mem == 0) {
    int64_t mem_size;
    size_t size = sizeof(mem_size);
    if (sysctlbyname("hw.memsize", &mem_size, &size, NULL, 0) == 0)
      cached_total_mem = mem_size;
  }
  info.total = cached_total_mem;

  static const long long page_size =
      static_cast<int64_t>(sysconf(_SC_PAGESIZE));

  mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
  vm_statistics_data_t vm_stat;
  if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vm_stat,
                      &count) == KERN_SUCCESS) {
    const long long active_mem =
        static_cast<int64_t>(vm_stat.active_count) * page_size;
    const long long wired_mem =
        static_cast<int64_t>(vm_stat.wire_count) * page_size;
    info.used = active_mem + wired_mem;
  }

  if (info.total > 0) {
    info.percent = (static_cast<double>(info.used) / info.total) * 100.0;
  }
  return info;
}

struct DiskInfo {
  uint64_t total;
  uint64_t used;
  double percent;
};

inline DiskInfo getDiskInfo() {
  DiskInfo info{};
  struct statfs stats;
  if (statfs("/", &stats) == 0) {
    info.total = static_cast<uint64_t>(stats.f_blocks) * stats.f_bsize;
    const uint64_t free = static_cast<uint64_t>(stats.f_bfree) * stats.f_bsize;
    info.used = info.total - free;
    if (info.total > 0) {
      info.percent = (static_cast<double>(info.used) / info.total) * 100.0;
    }
  }
  return info;
}

struct LoginInfo {
  std::string time;
  std::string ip;
  bool ip_present;
  std::string uptime;
};

inline LoginInfo getLastLogin() {
  LoginInfo info{};
  info.ip_present = false;

  const char *user = getenv("USER");
  std::string cmd = "last -1 ";
  cmd += user;
  cmd += " 2>/dev/null | head -1";
  const std::string last_login = execCommand(cmd.c_str());

  if (last_login.empty() ||
      last_login.find("never logged in") != std::string::npos) {
    info.time = "Never logged in";
  } else {
    std::stringstream ss(last_login);
    std::string segment;
    std::vector<std::string> parts;
    parts.reserve(10);
    while (ss >> segment)
      parts.push_back(segment);

    int date_start_idx = 2;
    if (parts.size() > 2) {
      if (std::isdigit(parts[2][0]) &&
          parts[2].find('.') != std::string::npos) {
        info.ip = parts[2];
        info.ip_present = true;
        date_start_idx = 3;
      }
    }

    if (parts.size() >= static_cast<size_t>(date_start_idx + 4)) {
      info.time = parts[date_start_idx] + " " + parts[date_start_idx + 1] +
                  " " + parts[date_start_idx + 2] + " " +
                  parts[date_start_idx + 3];
    } else {
      info.time = last_login;
    }
  }

  std::string uptime_raw = execCommand("uptime");
  const size_t up_pos = uptime_raw.find("up ");
  if (up_pos != std::string::npos) {
    std::string sub = uptime_raw.substr(up_pos + 3);
    const size_t comma_pos = sub.find(",");
    if (comma_pos != std::string::npos) {
      std::string part1 = sub.substr(0, comma_pos);
      std::string time_part = "";

      if (part1.find("day") != std::string::npos) {
        const size_t comma2_pos = sub.find(",", comma_pos + 1);
        if (comma2_pos != std::string::npos) {
          time_part = sub.substr(comma_pos + 1, comma2_pos - comma_pos - 1);
        }

        size_t p;
        while ((p = part1.find(" days")) != std::string::npos)
          part1.replace(p, 5, "d");
        while ((p = part1.find(" day")) != std::string::npos)
          part1.replace(p, 4, "d");
        part1.erase(std::remove(part1.begin(), part1.end(), ' '), part1.end());

        time_part.erase(0, time_part.find_first_not_of(" "));
        const size_t colon = time_part.find(":");
        if (colon != std::string::npos) {
          const std::string hours = time_part.substr(0, colon);
          const std::string mins = time_part.substr(colon + 1);
          info.uptime = part1 + " " + hours + "h " + mins + "m";
        } else {
          info.uptime = part1;
        }
      } else {
        part1.erase(0, part1.find_first_not_of(" "));
        const size_t colon = part1.find(":");
        if (colon != std::string::npos) {
          const std::string hours = part1.substr(0, colon);
          const std::string mins = part1.substr(colon + 1);
          info.uptime = hours + "h " + mins + "m";
        } else {
          info.uptime = part1;
        }
      }
    }
  }

  return info;
}

inline std::string getCurrentUser() {
  static std::string cached_user;
  if (cached_user.empty()) {
    const char *user_env = getenv("USER");
    cached_user = user_env ? user_env : "";
  }
  return cached_user;
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

  std::vector<std::string> all_strings = {
      REPORT_TITLE,   os_name,       os_kernel,        net_hostname,
      net_machine_ip, net_client_ip, net_current_user, cpu.model,
      cpu_cores_str,  "Bare Metal",  cpu_usage_str,    mem_usage_str,
      disk_usage_str, login.time,    login.ip,         login.uptime};

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
  printCenteredData(REPORT_TITLE, current_len);
  printCenteredData("TR-1000 MACHINE REPORT", current_len);
  printDivider("top", current_len);

  printData("OS", os_name, current_len);
  printData("KERNEL", os_kernel, current_len);
  printDivider("", current_len);

  printData("HOSTNAME", net_hostname, current_len);
  printData("MACHINE IP", net_machine_ip, current_len);
  printData("CLIENT  IP", net_client_ip, current_len);
  for (size_t i = 0; i < net_dns_ip.size(); ++i) {
    printData("DNS  IP " + std::to_string(i + 1), net_dns_ip[i], current_len);
  }
  printData("USER", net_current_user, current_len);
  printDivider("", current_len);

  printData("PROCESSOR", cpu.model, current_len);
  printData("CORES", cpu_cores_str, current_len);
  printData("HYPERVISOR", "Bare Metal", current_len);
  printData("CPU USAGE", cpu_usage_str, current_len);
  printData("LOAD  1m", cpu_1_graph, current_len);
  printData("LOAD  5m", cpu_5_graph, current_len);
  printData("LOAD 15m", cpu_15_graph, current_len);
  printDivider("", current_len);

  printData("VOLUME", disk_usage_str, current_len);
  printData("DISK USAGE", disk_graph, current_len);
  printDivider("", current_len);

  printData("MEMORY", mem_usage_str, current_len);
  printData("USAGE", mem_graph, current_len);
  printDivider("", current_len);

  printData("LAST LOGIN", login.time, current_len);
  if (login.ip_present) {
    printData("", login.ip, current_len);
  }
  printData("UPTIME", login.uptime, current_len);

  printDivider("bottom", current_len);

  return 0;
}
