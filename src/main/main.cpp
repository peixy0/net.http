#include <spdlog/spdlog.h>
#include <thread>
#include "app.hpp"
#include "server.hpp"

int main(int argc, char* argv[]) {
  if (argc < 3) {
    return -1;
  }
  auto* host = argv[1];
  std::uint16_t port = std::atoi(argv[2]);
  spdlog::set_level(spdlog::level::off);

  application::AppBtmpContentLoader btmpLoader;
  btmpLoader.Run();

  std::vector<std::thread> workers;
  const int nWorkers = std::thread::hardware_concurrency();
  for (int i = 0; i < nWorkers; i++) {
    workers.emplace_back(std::thread([&host, &port, &btmpLoader] {
      application::AppLayer app{btmpLoader};
      network::Server server;
      server.Add(network::HttpMethod::GET, "^/api/system/lastb$",
          [&app](network::HttpRequest&& req, network::HttpSender& sender) {
            app.RequestLastbLog(std::move(req), sender);
          });
      server.Add(network::HttpMethod::GET, "^/api/system/nginx$",
          [&app](network::HttpRequest&& req, network::HttpSender& sender) {
            app.RequestNginxLog(std::move(req), sender);
          });
      server.Start(host, port);
    }));
  }
  for (auto& worker : workers) {
    worker.join();
  }

  return 0;
}
