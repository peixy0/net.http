#include "app.hpp"
#include <spdlog/spdlog.h>

namespace application {

AppLayer::AppLayer() {
  t = std::thread([this] {
    while (true) {
      const Probe p;
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
      std::int64_t d1 = probes[i].GetCpuSum() - probes[i - 1].GetCpuSum();
      std::int64_t d2 = probes[i].GetCpuIdle() - probes[i - 1].GetCpuIdle();
      float f = 1 - d2 / static_cast<float>(d1);
      resp.body += "{\"timestamp\": " + std::to_string(probes[i].GetTimestamp()) + ", \"cpu\": " + std::to_string(f) +
                   ", \"memtotal\": " + std::to_string(probes[i].GetMemTotal()) +
                   ", \"memfree\": " + std::to_string(probes[i].GetMemFree()) +
                   ", \"memavail\": " + std::to_string(probes[i].GetMemAvail()) + "}";
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
