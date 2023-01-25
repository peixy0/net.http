#pragma once
#include <optional>
#include "network.hpp"

namespace application {

struct AppOptions {
  std::string wwwRoot;
};

class AppHttpProcessor : public network::HttpProcessor {
public:
  AppHttpProcessor(const AppOptions&, network::HttpSender&, network::ProtocolUpgrader&);
  void Process(network::HttpRequest&&) override;

private:
  void ServeWebsocket(const network::HttpRequest&);
  void ServeFile(const network::HttpRequest&);
  std::string MimeTypeOf(std::string_view);

  AppOptions options;
  network::HttpSender& sender;
  network::ProtocolUpgrader& upgrader;
};

class AppHttpProcessorFactory : public network::HttpProcessorFactory {
public:
  AppHttpProcessorFactory(const AppOptions&);
  std::unique_ptr<network::HttpProcessor> Create(network::HttpSender&, network::ProtocolUpgrader&) const override;

private:
  AppOptions options;
};

class AppWebsocketProcessor : public network::WebsocketProcessor {
public:
  explicit AppWebsocketProcessor(network::WebsocketFrameSender&);
  void Process(network::WebsocketFrame&&) override;

private:
  network::WebsocketFrameSender& sender;
};

class AppWebsocketProcessorFactory : public network::WebsocketProcessorFactory {
public:
  AppWebsocketProcessorFactory() = default;
  std::unique_ptr<network::WebsocketProcessor> Create(network::WebsocketFrameSender&) const override;
};

}  // namespace application
