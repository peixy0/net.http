#include "app.hpp"
#include <spdlog/spdlog.h>
#include "common.hpp"
#include "parser.hpp"
#include "websocket.hpp"

namespace application {

WebsocketLayer::WebsocketLayer(
    std::unique_ptr<network::WebsocketFrameParser> parser, std::unique_ptr<network::WebsocketSender> sender)
    : parser{std::move(parser)}, sender{std::move(sender)} {
}

void WebsocketLayer::Process(network::HttpRequest&& req) {
  parser->Append(req.body);
  auto frame = parser->Parse();
  if (not frame) {
    return;
  }
  spdlog::debug("app received websocket frame: fin = {}, opcode = {}", frame->fin, frame->opcode);
  if (frame->opcode == opClose) {
    sender->Close();
    return;
  }
  spdlog::debug("app received websocket message: {}", frame->payload);
  sender->Send(std::move(*frame));
}

AppLayer::AppLayer(const AppOptions& options, network::HttpSender& sender) : options{options}, sender{sender} {
}

void AppLayer::Process(network::HttpRequest&& req) {
  spdlog::debug("app received request {} {} {}", req.method, req.uri, req.version);
  if (websocketLayer) {
    websocketLayer->Process(std::move(req));
    return;
  }
  if (req.uri == "/ws") {
    ServeWebsocket(req);
    return;
  }
  ServeFile(req);
}

void AppLayer::ServeWebsocket(const network::HttpRequest& req) {
  network::HttpResponse notFoundResp;
  notFoundResp.status = network::HttpStatus::NotFound;
  notFoundResp.headers.emplace("Content-Type", "text/plain");
  notFoundResp.body = "Not Found";

  auto upgradeIt = req.headers.find("upgrade");
  if (upgradeIt == req.headers.end()) {
    sender.Send(std::move(notFoundResp));
    return;
  }
  auto upgrade = upgradeIt->second;
  common::ToLower(upgrade);
  if (upgrade != "websocket") {
    sender.Send(std::move(notFoundResp));
    return;
  }
  auto keyIt = req.headers.find("sec-websocket-key");
  if (keyIt == req.headers.end()) {
    sender.Send(std::move(notFoundResp));
    return;
  }
  auto accept = common::Base64(common::SHA1(keyIt->second + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
  network::HttpResponse resp;
  resp.status = network::HttpStatus::SwitchingProtocols;
  resp.headers.emplace("Upgrade", "websocket");
  resp.headers.emplace("Connection", "Upgrade");
  resp.headers.emplace("Sec-WebSocket-Accept", std::move(accept));
  sender.Send(std::move(resp));
  auto websocketParser = std::make_unique<network::ConcreteWebsocketFrameParser>();
  auto websocketSender = std::make_unique<network::ConcreteWebsocketSender>(sender);
  websocketLayer = std::make_unique<WebsocketLayer>(std::move(websocketParser), std::move(websocketSender));
}

void AppLayer::ServeFile(const network::HttpRequest& req) {
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

std::string AppLayer::MimeTypeOf(std::string_view path) {
  if (path.ends_with(".html")) {
    return "text/html";
  }
  return "text/plain";
}

AppLayerFactory::AppLayerFactory(const AppOptions& options_) : options{options_} {
  const std::string s{options.wwwRoot};
  char resolvedPath[PATH_MAX];
  realpath(s.c_str(), resolvedPath);
  chdir(resolvedPath);
  options.wwwRoot = resolvedPath;
}

std::unique_ptr<network::HttpProcessor> AppLayerFactory::Create(network::HttpSender& sender) const {
  return std::make_unique<AppLayer>(options, sender);
}

}  // namespace application
