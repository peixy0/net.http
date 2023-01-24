#include "app.hpp"
#include <spdlog/spdlog.h>
#include "common.hpp"
#include "network.hpp"

namespace application {

WebsocketLayer::WebsocketLayer(network::HttpSender& sender) : sender{sender} {
}

void WebsocketLayer::Process(network::HttpRequest&& req) {
  received += std::move(req.body);
  auto frame = ExtractFrame(received);
  if (not frame) {
    return;
  }
  spdlog::debug("app received websocket frame: fin = {}, opcode = {}", frame->fin, frame->opcode);
  if (frame->opcode == opClose) {
    sender.Close();
    return;
  }
  message += std::move(frame->payload);
  if (frame->fin) {
    ProcessMessage(message);
  }
}

std::optional<WebsocketFrame> WebsocketLayer::ExtractFrame(std::string& buffer) {
  int bufferLen = buffer.length();
  int requiredLen = headerLen;
  if (bufferLen < requiredLen) {
    return std::nullopt;
  }
  WebsocketFrame frame;
  const auto* p = reinterpret_cast<const unsigned char*>(buffer.data());
  frame.fin = (p[0] >> 7) & 0b1;
  frame.opcode = p[0] & 0b1111;
  bool mask = (p[1] >> 7) & 0b1;
  std::uint64_t len = p[1] & 0b1111111;
  p += headerLen;
  int payloadExtLen = 0;
  if (len == 126) {
    payloadExtLen = ext1Len;
  }
  if (len == 127) {
    payloadExtLen = ext2Len;
  }
  requiredLen += payloadExtLen;
  if (bufferLen < requiredLen) {
    return std::nullopt;
  }
  if (payloadExtLen > 0) {
    len = 0;
    for (int i = 0; i < payloadExtLen; i++) {
      len |= p[i] << (8 * (payloadExtLen - i - 1));
    }
    p += payloadExtLen;
  }
  unsigned char maskKey[maskLen] = {0};
  if (mask) {
    requiredLen += maskLen;
    if (bufferLen < requiredLen) {
      return std::nullopt;
    }
    for (int i = 0; i < maskLen; i++) {
      maskKey[i] = p[i];
    }
    p += maskLen;
  }
  requiredLen += len;
  if (bufferLen < requiredLen) {
    return std::nullopt;
  }
  std::string payload{p, p + len};
  for (std::uint64_t i = 0; i < len; i++) {
    payload[i] ^= reinterpret_cast<const std::uint8_t*>(&maskKey)[i % maskLen];
  }
  frame.payload = std::move(payload);
  buffer.erase(0, requiredLen);
  return frame;
}

void WebsocketLayer::ProcessMessage(std::string& message) {
  spdlog::debug("app received websocket message: {}", message);
  message.clear();
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
  websocketLayer = std::make_unique<WebsocketLayer>(sender);
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
