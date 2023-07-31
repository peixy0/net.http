#pragma once
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include "http.hpp"
#include "websocket.hpp"

namespace network {

class HttpRouteMapping {
public:
  void Add(HttpMethod method, const std::string& uri, std::unique_ptr<HttpProcessorFactory> processorFactory) {
    mapping.emplace_back(std::make_tuple<HttpMethod, std::regex, std::unique_ptr<HttpProcessorFactory>>(
        std::move(method), std::regex{uri}, std::move(processorFactory)));
  }

  HttpProcessorFactory* Get(HttpMethod method, const std::string& uri) const {
    for (const auto& [m, k, v] : mapping) {
      if (m == method and std::regex_match(uri, k)) {
        return v.get();
      }
    }
    return nullptr;
  }

private:
  std::vector<std::tuple<HttpMethod, std::regex, std::unique_ptr<HttpProcessorFactory>>> mapping;
};

class WebsocketRouteMapping {
public:
  void Add(const std::string& uri, std::unique_ptr<WebsocketProcessorFactory> processorFactory) {
    mapping.emplace_back(std::make_tuple<std::regex, std::unique_ptr<WebsocketProcessorFactory>>(
        std::regex{uri}, std::move(processorFactory)));
  }

  WebsocketProcessorFactory* Get(const std::string& uri) const {
    for (const auto& [k, v] : mapping) {
      if (std::regex_match(uri, k)) {
        return v.get();
      }
    }
    return nullptr;
  }

private:
  std::vector<std::tuple<std::regex, std::unique_ptr<WebsocketProcessorFactory>>> mapping;
};

class ConcreteRouter final : public Router {
public:
  ConcreteRouter(HttpRouteMapping& httpMapping, WebsocketRouteMapping& websocketMapping, TcpSender& tcpSender)
      : tcpSender{tcpSender},
        httpMapping{httpMapping},
        websocketMapping{websocketMapping},
        httpAggregation{tcpSender, *this},
        protocolProcessorDelegate{&httpAggregation.httpLayer} {
  }

  bool TryProcess(std::string& buffer) override {
    return protocolProcessorDelegate->TryProcess(buffer);
  }

  void Process(HttpRequest&&) override;
  void Process(WebsocketFrame&&) override;

private:
  struct HttpAggregation {
    HttpAggregation(TcpSender& tcpSender, HttpProcessor& httpProcessor)
        : httpSender{tcpSender}, httpLayer{httpParser, httpSender, httpProcessor} {
    }
    ConcreteHttpSender httpSender;
    ConcreteHttpParser httpParser;
    HttpLayer httpLayer;
    std::unique_ptr<HttpProcessor> httpProcessor{nullptr};
  };

  struct WebsocketAggregation {
    WebsocketAggregation(TcpSender& tcpSender, WebsocketProcessor& websocketProcessor)
        : websocketSender{tcpSender}, websocketLayer{websocketParser, websocketSender, websocketProcessor} {
    }
    ConcreteWebsocketSender websocketSender;
    ConcreteWebsocketParser websocketParser;
    WebsocketLayer websocketLayer;
    std::unique_ptr<WebsocketProcessor> websocketProcessor{nullptr};
  };

  bool TryUpgradeToWebsocket(const HttpRequest& req);

  TcpSender& tcpSender;
  HttpRouteMapping& httpMapping;
  WebsocketRouteMapping& websocketMapping;
  HttpAggregation httpAggregation;
  std::optional<WebsocketAggregation> websocketAggregation{std::nullopt};
  ProtocolProcessor* protocolProcessorDelegate;
};

class ConcreteRouterFactory final : public RouterFactory {
public:
  ConcreteRouterFactory(HttpRouteMapping& httpMapping, WebsocketRouteMapping& websocketMapping)
      : httpMapping{httpMapping}, websocketMapping{websocketMapping} {
  }

  std::unique_ptr<Router> Create(TcpSender& sender) const override {
    return std::make_unique<ConcreteRouter>(httpMapping, websocketMapping, sender);
  }

private:
  HttpRouteMapping& httpMapping;
  WebsocketRouteMapping& websocketMapping;
};

}  // namespace network
