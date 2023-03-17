#pragma once
#include <deque>
#include <mutex>
#include <thread>
#include "network.hpp"
#include "probe.hpp"

namespace application {

struct ProbeInfo {
  std::int64_t timestamp;
  std::int64_t cpuSum;
  std::int64_t cpuIdle;
  std::int64_t memTotal;
  std::int64_t memFree;
  std::int64_t memAvail;
  std::int64_t netTx;
  std::int64_t netRx;
};

class AppLayer {
public:
  AppLayer();
  void GetProbe(network::HttpSender&);

private:
  std::thread t;
  std::deque<ProbeInfo> probes;
  int count{0};
  static const int maxCount{289};
  std::mutex mutex;
};

}  // namespace application
