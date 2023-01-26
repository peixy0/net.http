#pragma once
#include <optional>
#include "network.hpp"

namespace application {

struct AppOptions {
  std::string wwwRoot;
};

class AppLayer {
public:
  AppLayer(const AppOptions&);
  void Process(network::HttpRequest&&, network::HttpSender&);
  void Process(network::WebsocketFrame&&, network::WebsocketSender&);

private:
  std::string MimeTypeOf(std::string_view) const;

  const AppOptions options;
};

}  // namespace application
