#include "app.hpp"
#include <spdlog/spdlog.h>
#include "websocket.hpp"

namespace application {

AppHttpProcessor::AppHttpProcessor(
    const AppOptions& options, network::HttpSender& sender, network::ProtocolUpgrader& upgrader)
    : options{options}, sender{sender}, upgrader{upgrader} {
}

void AppHttpProcessor::Process(network::HttpRequest&& req) {
  if (req.uri == "/ws") {
    ServeWebsocket(req);
    return;
  }
  ServeFile(req);
}

void AppHttpProcessor::ServeWebsocket(const network::HttpRequest& req) {
  network::WebsocketHandshakeBuilder handshakeBuilder{req};
  auto resp = handshakeBuilder.Build();
  if (not resp) {
    network::HttpResponse notFoundResp;
    notFoundResp.status = network::HttpStatus::NotFound;
    notFoundResp.headers.emplace("Content-Type", "text/plain");
    notFoundResp.body = "Not Found";
    sender.Send(std::move(notFoundResp));
    return;
  }
  sender.Send(std::move(*resp));
  upgrader.UpgradeToWebsocket();
}

void AppHttpProcessor::ServeFile(const network::HttpRequest& req) {
  std::string uri = req.uri;
  if (uri.ends_with("/")) {
    uri += "index.html";
  }
  if (uri.starts_with("/")) {
    uri.erase(0, 1);
  }
  char localPathStr[PATH_MAX];
  realpath(uri.c_str(), localPathStr);
  std::string localPath{localPathStr};
  spdlog::debug("request local path is {}", localPath);
  if (not localPath.starts_with(options.wwwRoot)) {
    network::HttpResponse resp;
    resp.status = network::HttpStatus::NotFound;
    resp.headers.emplace("Content-Type", "text/plain");
    resp.body = "Not Found";
    return sender.Send(std::move(resp));
  }
  std::string mimeType = MimeTypeOf(localPath);
  network::FileHttpResponse resp;
  resp.path = std::move(localPath);
  resp.headers.emplace("Content-type", std::move(mimeType));
  return sender.Send(std::move(resp));
}

std::string AppHttpProcessor::MimeTypeOf(std::string_view path) {
  if (path.ends_with(".html")) {
    return "text/html";
  }
  return "text/plain";
}

AppHttpProcessorFactory::AppHttpProcessorFactory(const AppOptions& options_) : options{options_} {
  const std::string s{options.wwwRoot};
  char resolvedPath[PATH_MAX];
  realpath(s.c_str(), resolvedPath);
  chdir(resolvedPath);
  options.wwwRoot = resolvedPath;
}

std::unique_ptr<network::HttpProcessor> AppHttpProcessorFactory::Create(
    network::HttpSender& sender, network::ProtocolUpgrader& upgrader) const {
  return std::make_unique<AppHttpProcessor>(options, sender, upgrader);
}

AppWebsocketProcessor::AppWebsocketProcessor(network::WebsocketFrameSender& sender) : sender{sender} {
}

void AppWebsocketProcessor::Process(network::WebsocketFrame&& frame) {
  sender.Send(std::move(frame));
}

std::unique_ptr<network::WebsocketProcessor> AppWebsocketProcessorFactory::Create(
    network::WebsocketFrameSender& sender) const {
  return std::make_unique<AppWebsocketProcessor>(sender);
}

}  // namespace application
