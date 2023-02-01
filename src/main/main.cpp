#include <spdlog/spdlog.h>
#include <thread>
#include "app.hpp"
#include "network.hpp"
#include "server.hpp"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    return -1;
  }
  auto* host = argv[1];
  std::uint16_t port = std::atoi(argv[2]);
  spdlog::set_level(spdlog::level::off);

  application::AppOptions appOptions;
  appOptions.wwwRoot = argv[3];
  application::AppLayer appLayer{appOptions};

  std::vector<std::thread> workers;
  const std::size_t nWorkers = std::thread::hardware_concurrency();
  for (std::size_t i = 0; i < nWorkers; i++) {
    workers.emplace_back(std::thread([&host, &port, &appLayer] {
      network::Server server;
      server.Add(network::HttpMethod::GET, "^/$", [&appLayer](network::HttpRequest&& req, network::HttpSender& sender) {
        appLayer.Process(std::move(req), sender);
      });
      server.Add(network::HttpMethod::GET, "^/static/.*$",
          [&appLayer](
              network::HttpRequest&& req, network::HttpSender& sender) { appLayer.Process(std::move(req), sender); });
      server.Add("^/ws$", [&appLayer](network::WebsocketFrame&& req, network::WebsocketSender& sender) {
        appLayer.Process(std::move(req), sender);
      });
      server.Start(host, port);
    }));
  }
  for (auto& worker : workers) {
    worker.join();
  }

  return 0;
}
