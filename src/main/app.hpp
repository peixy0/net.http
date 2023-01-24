#pragma once
#include <optional>
#include "network.hpp"

namespace application {

struct WebsocketFrame {
  bool fin;
  std::uint8_t opcode;
  std::string payload;
};

class WebsocketLayer {
public:
  explicit WebsocketLayer(network::HttpSender&);
  void Process(network::HttpRequest&&);

private:
  std::optional<WebsocketFrame> ExtractFrame(std::string&);
  void ProcessMessage(std::string&);

  network::HttpSender& sender;
  std::string received;
  std::string message;
  static constexpr std::uint8_t headerLen = 2;
  static constexpr std::uint8_t maskLen = 4;
  static constexpr std::uint8_t ext1Len = 2;
  static constexpr std::uint8_t ext2Len = 8;
  static constexpr std::uint8_t opClose = 8;
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
