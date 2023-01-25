#include <spdlog/spdlog.h>
#include <thread>
#include "app.hpp"
#include "http.hpp"
#include "network.hpp"
#include "protocol.hpp"
#include "tcp.hpp"
#include "websocket.hpp"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    return -1;
  }
  auto* host = argv[1];
  std::uint16_t port = std::atoi(argv[2]);
  spdlog::set_level(spdlog::level::debug);

  application::AppOptions appOptions;
  appOptions.wwwRoot = argv[3];

  std::vector<std::thread> workers;
  const int nWorkers = std::thread::hardware_concurrency();
  for (int i = 0; i < nWorkers; i++) {
    workers.emplace_back(std::thread([&host, &port, &appOptions] {
      auto appHttpProcessorFactory = std::make_unique<application::AppHttpProcessorFactory>(appOptions);
      auto httpLayerFactory = std::make_unique<network::ConcreteHttpLayerFactory>(std::move(appHttpProcessorFactory));
      auto protocolLayerFactory = std::make_unique<network::ProtocolLayerFactory>(std::move(httpLayerFactory));
      auto appWebsocketProcessorFactory = std::make_unique<application::AppWebsocketProcessorFactory>();
      auto websocketLayerFactory =
          std::make_unique<network::ConcreteWebsocketLayerFactory>(std::move(appWebsocketProcessorFactory));
      protocolLayerFactory->Add(std::move(websocketLayerFactory));
      network::Tcp4Layer tcp{host, port, std::move(protocolLayerFactory)};
      tcp.Start();
    }));
  }
  for (auto& worker : workers) {
    worker.join();
  }

  return 0;
}
