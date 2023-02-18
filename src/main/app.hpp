#pragma once
#include <deque>
#include <mutex>
#include <thread>
#include "network.hpp"
#include "probe.hpp"

namespace application {

class AppLayer {
public:
  AppLayer();
  void GetProbe(network::HttpSender&);

private:
  std::thread t;
  std::deque<Probe> probes;
  int count{0};
  static const int maxCount{289};
  std::mutex mutex;
};

}  // namespace application
