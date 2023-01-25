#pragma once
#include <optional>
#include "network.hpp"

namespace application {

struct AppOptions {
  std::string wwwRoot;
};

class AppLayer : public network::HttpProcessor {
public:
  AppLayer(const AppOptions&, network::HttpSender&, network::ProtocolUpgrader&);
  ~AppLayer() override = default;
  void Process(network::HttpRequest&&) override;

private:
  void ServeWebsocket(const network::HttpRequest&);
  void ServeFile(const network::HttpRequest&);
  std::string MimeTypeOf(std::string_view);

  AppOptions options;
  network::HttpSender& sender;
  network::ProtocolUpgrader& upgrader;
};

class AppLayerFactory : public network::HttpProcessorFactory {
public:
  AppLayerFactory(const AppOptions&);
  std::unique_ptr<network::HttpProcessor> Create(network::HttpSender&, network::ProtocolUpgrader&) const override;

private:
  AppOptions options;
};

}  // namespace application
