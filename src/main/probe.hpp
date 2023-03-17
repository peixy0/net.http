#pragma once
#include <cstdint>
#include <string>

namespace application {

class CpuProbe {
public:
  CpuProbe();

  std::int64_t GetSum() const {
    return sum;
  }

  std::int64_t GetIdle() const {
    return idle;
  }

private:
  void ParseProcStat(std::string&);
  void ParseCpu(std::string&);

  std::int64_t sum{0};
  std::int64_t idle{0};
};

class MemProbe {
public:
  MemProbe();

  std::int64_t GetTotal() const {
    return total;
  }

  std::int64_t GetFree() const {
    return free;
  }

  std::int64_t GetAvail() const {
    return avail;
  }

private:
  void ParseProcMemInfo(std::string&);
  void ParseMemTotal(std::string&);
  void ParseMemFree(std::string&);
  void ParseMemAvail(std::string&);

  std::int64_t total{0};
  std::int64_t free{0};
  std::int64_t avail{0};
};

class NetProbe {
public:
  explicit NetProbe(std::string_view);

  std::int64_t GetTx() const {
    return tx;
  }

  std::int64_t GetRx() const {
    return rx;
  }

private:
  std::int64_t tx;
  std::int64_t rx;
};

}  // namespace application
