#include <spdlog/spdlog.h>
#include <cstring>
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
  network::HttpLayerFactory factory{httpOptions, appFactory};

  network::TcpOptions tcpOptions;
  tcpOptions.maxBufferedSize = 30;
  network::Tcp4Layer tcp{host, port, tcpOptions, factory};
  tcp.Start();

  return 0;
}
