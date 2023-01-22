#include <spdlog/spdlog.h>
#include <thread>
#include "app.hpp"
#include "http.hpp"
#include "network.hpp"
#include "tcp.hpp"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    return -1;
  }
  auto* host = argv[1];
  std::uint16_t port = std::atoi(argv[2]);
  spdlog::set_level(spdlog::level::debug);

  application::AppOptions appOptions;
  appOptions.wwwRoot = argv[3];
  application::AppLayerFactory appFactory{appOptions};

  network::HttpOptions httpOptions;
  httpOptions.maxPayloadSize = 1 << 20;
  network::HttpLayerFactory httpLayerFactory{httpOptions, appFactory};

  std::vector<std::thread> workers;
  const int nWorkers = std::thread::hardware_concurrency();
  for (int i = 0; i < nWorkers; i++) {
    workers.emplace_back(std::thread([&host, &port, &httpLayerFactory] {
      network::Tcp4Layer tcp{host, port, httpLayerFactory};
      tcp.Start();
    }));
  }
  for (auto& worker : workers) {
    worker.join();
  }

  return 0;
}
