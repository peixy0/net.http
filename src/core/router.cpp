#include "router.hpp"

namespace network {

bool ConcreteRouter::TryUpgradeToWebsocket(const HttpRequest& req) {
  auto* entry = websocketMapping.Get(req.uri);
  if (not entry) {
    return false;
  }
  WebsocketHandshakeBuilder handshake{req};
  auto resp = handshake.Build();
  if (not resp) {
    return false;
  }
  httpAggregation.httpSender.Send(std::move(*resp));
  httpAggregation.httpProcessor.reset();
  websocketAggregation.emplace(tcpSender, *this);
  websocketAggregation->websocketProcessor = entry->Create(websocketAggregation->websocketSender);
  protocolProcessorDelegate = &websocketAggregation->websocketLayer;
  return true;
}

void ConcreteRouter::Process(HttpRequest&& req) {
  if (TryUpgradeToWebsocket(req)) {
    return;
  }
  auto entry = httpMapping.Get(req.method, req.uri);
  if (entry) {
    websocketAggregation.reset();
    httpAggregation.httpProcessor = entry->Create(httpAggregation.httpSender);
    httpAggregation.httpProcessor->Process(std::move(req));
    return;
  }
  HttpResponse resp;
  resp.status = HttpStatus::NotFound;
  httpAggregation.httpSender.Send(std::move(resp));
}

void ConcreteRouter::Process(WebsocketFrame&& req) {
  if (not websocketAggregation) {
    return;
  }
  if (not websocketAggregation->websocketProcessor) {
    websocketAggregation->websocketSender.Close();
    return;
  }
  websocketAggregation->websocketProcessor->Process(std::move(req));
}

}  // namespace network
