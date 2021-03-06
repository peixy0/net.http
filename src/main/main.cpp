#include <spdlog/spdlog.h>
#include <cstring>
#include "app.hpp"
#include "http.hpp"
#include "tcp.hpp"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    return -1;
  }
  auto* host = argv[1];
  std::uint16_t port = std::atoi(argv[2]);
  spdlog::set_level(spdlog::level::debug);
  application::AppLayer app{argv[3]};
  network::HttpOptions options;
  options.maxPayloadSize = 1 << 20;
  network::HttpLayerFactory factory{options, app};
  network::Tcp4Layer tcp{host, port, factory};
  tcp.Start();
  return 0;
}
