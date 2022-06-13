#pragma once
#include <shared_mutex>
#include <thread>
#include "network.hpp"

namespace application {

class AppBtmpContentLoader {
public:
  void Run();
  void LoadContent();
  std::string GetContent() const;

private:
  std::thread daemon;
  std::string btmpContent;
  mutable std::shared_mutex btmpMutex;
};

class AppLayer {
public:
  AppLayer(AppBtmpContentLoader&);
  void RequestLastbLog(network::HttpRequest&&, network::HttpSender&);
  void RequestNginxLog(network::HttpRequest&&, network::HttpSender&);

private:
  AppBtmpContentLoader& btmpLoader;
};

}  // namespace application
