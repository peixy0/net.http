#pragma once
#include "network.hpp"

namespace application {

class WebsocketLayer {
public:
  explicit WebsocketLayer(network::HttpSender&);
  void Process(network::HttpRequest&&);

private:
  network::HttpSender& sender;
};

struct AppOptions {
  std::string wwwRoot;
};

class AppLayer : public network::HttpProcessor {
public:
  explicit AppLayer(const AppOptions&, network::HttpSender&);
  ~AppLayer() override = default;
  void Process(network::HttpRequest&&) override;

private:
  void ServeWebsocket(const network::HttpRequest&);
  void ServeFile(const network::HttpRequest&);
  std::string MimeTypeOf(std::string_view);

  AppOptions options;
  network::HttpSender& sender;
  std::unique_ptr<WebsocketLayer> websocketLayer{nullptr};
};

class AppLayerFactory : public network::HttpProcessorFactory {
public:
  AppLayerFactory(const AppOptions&);
  std::unique_ptr<network::HttpProcessor> Create(network::HttpSender&) const override;

private:
  AppOptions options;
};

}  // namespace application
