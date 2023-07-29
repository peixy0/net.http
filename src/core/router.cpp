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
  httpSender.Send(std::move(*resp));
  httpProcessor.reset();
  websocketProcessor = entry->Create(websocketSender);
  protocolProcessorDelegate = &websocketLayer;
  return true;
}

void ConcreteRouter::Process(HttpRequest&& req) {
  if (TryUpgradeToWebsocket(req)) {
    return;
  }
  auto entry = httpMapping.Get(req.method, req.uri);
  if (entry) {
    websocketProcessor.reset();
    httpProcessor = entry->Create(httpSender);
    httpProcessor->Process(std::move(req));
    return;
  }
  HttpResponse resp;
  resp.status = HttpStatus::NotFound;
  httpSender.Send(std::move(resp));
}

void ConcreteRouter::Process(WebsocketFrame&& req) {
  if (not websocketProcessor) {
    websocketSender.Close();
  }
  websocketProcessor->Process(std::move(req));
}

}  // namespace network
