#pragma once
#include <optional>
#include "network.hpp"

namespace application {

class WebsocketLayer {
public:
  WebsocketLayer(std::unique_ptr<network::WebsocketFrameParser>, std::unique_ptr<network::WebsocketSender>);
  void Process(network::HttpRequest&&);

private:
  static constexpr std::uint8_t opClose = 8;

  std::unique_ptr<network::WebsocketFrameParser> parser;
  std::unique_ptr<network::WebsocketSender> sender;
  std::string message;
};

struct AppOptions {
  std::string wwwRoot;
};

class AppLayer : public network::HttpProcessor {
public:
  AppLayer(const AppOptions&, network::HttpSender&, network::HttpSupervisor&);
  ~AppLayer() override = default;
  void Process(network::HttpRequest&&) override;

private:
  void ServeWebsocket(const network::HttpRequest&);
  void ServeFile(const network::HttpRequest&);
  std::string MimeTypeOf(std::string_view);

  AppOptions options;
  network::HttpSender& sender;
  network::HttpSupervisor& supervisor;
  std::unique_ptr<WebsocketLayer> websocketLayer{nullptr};
};

class AppLayerFactory : public network::HttpProcessorFactory {
public:
  AppLayerFactory(const AppOptions&);
  std::unique_ptr<network::HttpProcessor> Create(network::HttpSender&, network::HttpSupervisor&) const override;

private:
  AppOptions options;
};

}  // namespace application
