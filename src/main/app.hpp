#pragma once
#include "network.hpp"

namespace application {

struct AppOptions {
  std::string wwwRoot;
};

class AppLayer : public network::HttpProcessor {
public:
  explicit AppLayer(const AppOptions&, network::HttpSender&);
  ~AppLayer() override = default;
  void Process(network::HttpRequest&&) override;

private:
  std::string MimeTypeOf(std::string_view path);

  AppOptions options;
  network::HttpSender& sender;
};

class AppLayerFactory : public network::HttpProcessorFactory {
public:
  AppLayerFactory(const AppOptions&);
  std::unique_ptr<network::HttpProcessor> Create(network::HttpSender&) const override;

private:
  AppOptions options;
};

}  // namespace application
