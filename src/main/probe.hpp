#pragma once
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

namespace application {

class Probe {
public:
  Probe() {
    timestamp = std::time(nullptr);
    ProbeStat();
    ProbeMemInfo();
  }

  void ProbeStat() {
    int fd = open("/proc/stat", O_RDONLY);
    char buf[512];
    ssize_t n = 0;
    std::string stat;
    while ((n = read(fd, buf, sizeof buf)) != 0) {
      if (n < 0) {
        spdlog::error("error reading /proc/stat: {}", strerror(errno));
        close(fd);
        return;
      }
      stat.append({buf, buf + n});
    }
    close(fd);
    ParseStat(stat);
  }

  void ProbeMemInfo() {
    int fd = open("/proc/meminfo", O_RDONLY);
    char buf[512];
    ssize_t n = 0;
    std::string meminfo;
    while ((n = read(fd, buf, sizeof buf)) != 0) {
      if (n < 0) {
        spdlog::error("error reading /proc/meminfo: {}", strerror(errno));
        close(fd);
        return;
      }
      meminfo.append({buf, buf + n});
    }
    close(fd);
    ParseMemInfo(meminfo);
  }

  std::int64_t GetTimestamp() const {
    return timestamp;
  }

  std::int64_t GetCpuSum() const {
    return cpuSum;
  }

  std::int64_t GetCpuIdle() const {
    return cpuIdle;
  }

  std::int64_t GetMemTotal() const {
    return memTotal;
  }

  std::int64_t GetMemFree() const {
    return memFree;
  }

  std::int64_t GetMemAvail() const {
    return memAvail;
  }

private:
  std::optional<std::string> ParseToken(std::string& buffer) {
    SkipWhitespaces(buffer);
    const auto i = buffer.find_first_of(" \n");
    if (i == buffer.npos) {
      return std::nullopt;
    }
    const auto token = buffer.substr(0, i);
    buffer.erase(0, i);
    return token;
  }

  void SkipWhitespaces(std::string& buffer) {
    const auto i = buffer.find_first_not_of(" \n");
    buffer.erase(0, i);
  }

  void ParseCpu(std::string& buffer) {
    int count = 0;
    cpuSum = 0;
    cpuIdle = 0;
    while (not buffer.empty() and not buffer.starts_with('\n')) {
      const auto token = ParseToken(buffer);
      if (not token) {
        return;
      }
      const auto n = strtoll(token->c_str(), nullptr, 10);
      cpuSum += n;
      if (count == 3) {
        cpuIdle += n;
      }
      ++count;
    }
  }

  void ParseStat(std::string& buffer) {
    std::optional<std::string> token;
    while ((token = ParseToken(buffer)) != std::nullopt) {
      if (token == "cpu") {
        ParseCpu(buffer);
      }
    }
  }

  void ParseMemTotal(std::string& buffer) {
    const auto token = ParseToken(buffer);
    if (not token) {
      return;
    }
    memTotal = strtoll(token->c_str(), nullptr, 10);
    while (not buffer.empty() and not buffer.starts_with('\n')) {
      ParseToken(buffer);
    }
  }

  void ParseMemFree(std::string& buffer) {
    const auto token = ParseToken(buffer);
    if (not token) {
      return;
    }
    memFree = strtoll(token->c_str(), nullptr, 10);
    while (not buffer.empty() and not buffer.starts_with('\n')) {
      ParseToken(buffer);
    }
  }

  void ParseMemAvail(std::string& buffer) {
    const auto token = ParseToken(buffer);
    if (not token) {
      return;
    }
    memAvail = strtoll(token->c_str(), nullptr, 10);
    while (not buffer.empty() and not buffer.starts_with('\n')) {
      ParseToken(buffer);
    }
  }

  void ParseMemInfo(std::string& buffer) {
    std::optional<std::string> token;
    while ((token = ParseToken(buffer)) != std::nullopt) {
      if (token == "MemTotal:") {
        ParseMemTotal(buffer);
      }
      if (token == "MemFree:") {
        ParseMemFree(buffer);
      }
      if (token == "MemAvailable:") {
        ParseMemAvail(buffer);
      }
    }
  }

  std::int64_t timestamp{0};
  std::int64_t cpuSum{0};
  std::int64_t cpuIdle{0};
  std::int64_t memTotal{0};
  std::int64_t memFree{0};
  std::int64_t memAvail{0};
};

}  // namespace application
