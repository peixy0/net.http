#include "app.hpp"
#include <spdlog/spdlog.h>
#include <ctime>

namespace application {

AppLayer::AppLayer() {
  const auto netInterface = std::getenv("PROBE_NET_INTERFACE");
  t = std::thread([this, netInterface] {
    while (true) {
      const CpuProbe cpu;
      const MemProbe mem;
      const NetProbe net{netInterface};
      ProbeInfo p;
      p.timestamp = time(nullptr);
      p.cpuSum = cpu.GetSum();
      p.cpuIdle = cpu.GetIdle();
      p.memTotal = mem.GetTotal();
      p.memFree = mem.GetFree();
      p.memAvail = mem.GetAvail();
      p.netTx = net.GetTx();
      p.netRx = net.GetRx();
      {
        std::lock_guard lock{mutex};
        probes.emplace_back(std::move(p));
        ++count;
        while (count > maxCount) {
          probes.pop_front();
          --count;
        }
      }
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(5min);
    }
  });
}

void AppLayer::GetProbe(network::HttpSender& sender) {
  network::HttpResponse resp;
  resp.body = "[\n";
  {
    std::lock_guard lock{mutex};
    for (int i = 1; i < count; i++) {
      std::int64_t dCpuSum = probes[i].cpuSum - probes[i - 1].cpuSum;
      std::int64_t dCpuIdle = probes[i].cpuIdle - probes[i - 1].cpuIdle;
      float cpuBusyRatio = 1 - dCpuIdle / static_cast<float>(dCpuSum);
      std::int64_t dTx = probes[i].netTx - probes[i - 1].netTx;
      std::int64_t dRx = probes[i].netRx - probes[i - 1].netRx;
      // clang-format off
      resp.body += "{"
        "\"timestamp\": " + std::to_string(probes[i].timestamp) +
        ", \"cpu_usage\": " + std::to_string(cpuBusyRatio) +
        ", \"mem_total\": " + std::to_string(probes[i].memTotal) +
        ", \"mem_free\": " + std::to_string(probes[i].memFree) +
        ", \"mem_avail\": " + std::to_string(probes[i].memAvail) +
        ", \"net_tx\": " + std::to_string(dTx) +
        ", \"net_rx\": " + std::to_string(dRx) +
      "}";
      // clang-format on
      if (i < count - 1) {
        resp.body += ", ";
      }
      resp.body += '\n';
    }
  }
  resp.body += "]";
  resp.status = network::HttpStatus::OK;
  resp.headers.emplace("Content-Type", "application/json");
  sender.Send(std::move(resp));
}

}  // namespace application
