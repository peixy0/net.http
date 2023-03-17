#include "probe.hpp"
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <optional>

namespace {

std::optional<std::string> ReadFile(const char* filename) {
  int fd = open(filename, O_RDONLY);
  char buf[512];
  ssize_t n = 0;
  std::string s;
  while ((n = read(fd, buf, sizeof buf)) != 0) {
    if (n < 0) {
      spdlog::error("error reading {}: {}", filename, strerror(errno));
      close(fd);
      return std::nullopt;
    }
    s.append({buf, buf + n});
  }
  close(fd);
  return s;
}

void SkipWhitespaces(std::string& buffer) {
  const auto i = buffer.find_first_not_of(" \n");
  buffer.erase(0, i);
}

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

}  // namespace

namespace application {

CpuProbe::CpuProbe() {
  auto stat = ReadFile("/proc/stat");
  if (not stat) {
    return;
  }
  CpuProbe::ParseProcStat(*stat);
}

void CpuProbe::ParseProcStat(std::string& buffer) {
  std::optional<std::string> token;
  while ((token = ParseToken(buffer)) != std::nullopt) {
    if (token == "cpu") {
      ParseCpu(buffer);
    }
  }
}

void CpuProbe::ParseCpu(std::string& buffer) {
  int count = 0;
  sum = 0;
  idle = 0;
  while (not buffer.empty() and not buffer.starts_with('\n')) {
    const auto token = ParseToken(buffer);
    if (not token) {
      return;
    }
    const auto n = strtoll(token->c_str(), nullptr, 10);
    sum += n;
    if (count == 3) {
      idle += n;
    }
    ++count;
  }
}

MemProbe::MemProbe() {
  auto meminfo = ReadFile("/proc/meminfo");
  if (not meminfo) {
    return;
  }
  ParseProcMemInfo(*meminfo);
}

void MemProbe::ParseProcMemInfo(std::string& buffer) {
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

void MemProbe::ParseMemTotal(std::string& buffer) {
  const auto token = ParseToken(buffer);
  if (not token) {
    return;
  }
  total = strtoll(token->c_str(), nullptr, 10);
  while (not buffer.empty() and not buffer.starts_with('\n')) {
    ParseToken(buffer);
  }
}

void MemProbe::ParseMemFree(std::string& buffer) {
  const auto token = ParseToken(buffer);
  if (not token) {
    return;
  }
  free = strtoll(token->c_str(), nullptr, 10);
  while (not buffer.empty() and not buffer.starts_with('\n')) {
    ParseToken(buffer);
  }
}

void MemProbe::ParseMemAvail(std::string& buffer) {
  const auto token = ParseToken(buffer);
  if (not token) {
    return;
  }
  avail = strtoll(token->c_str(), nullptr, 10);
  while (not buffer.empty() and not buffer.starts_with('\n')) {
    ParseToken(buffer);
  }
}

NetProbe::NetProbe(std::string_view netInterface) {
  std::string netStatPath = "/sys/class/net/";
  netStatPath += netInterface;
  netStatPath += "/statistics";
  const std::string txStatPath = netStatPath + "/tx_bytes";
  const std::string rxStatPath = netStatPath + "/rx_bytes";
  const auto txbuf = ReadFile(txStatPath.c_str());
  if (txbuf) {
    tx = strtoll(txbuf->c_str(), nullptr, 10);
  }
  const auto rxbuf = ReadFile(rxStatPath.c_str());
  if (rxbuf) {
    rx = strtoll(rxbuf->c_str(), nullptr, 10);
  }
}

}  // namespace application
