#include "websocket.hpp"
#include "common.hpp"

namespace network {

ConcreteWebsocketSender::ConcreteWebsocketSender(HttpSender& sender) : sender{sender} {
}

void ConcreteWebsocketSender::Send(WebsocketFrame&& frame) {
  RawHttpResponse resp;
  resp.body += static_cast<unsigned char>((frame.fin << 7) | (frame.opcode));
  std::uint64_t payloadLen = frame.payload.length();
  if (payloadLen < 126) {
    resp.body += static_cast<unsigned char>(payloadLen);
  } else if (payloadLen < (1 << 16)) {
    resp.body += static_cast<unsigned char>(126);
    resp.body += static_cast<unsigned char>((payloadLen >> 8) & 0xff);
    resp.body += static_cast<unsigned char>(payloadLen & 0xff);
  } else {
    resp.body += static_cast<unsigned char>(127);
    resp.body += static_cast<unsigned char>((payloadLen >> 24) & 0xff);
    resp.body += static_cast<unsigned char>((payloadLen >> 16) & 0xff);
    resp.body += static_cast<unsigned char>((payloadLen >> 8) & 0xff);
    resp.body += static_cast<unsigned char>(payloadLen & 0xff);
  }
  resp.body += frame.payload;
  sender.Send(std::move(resp));
}

void ConcreteWebsocketSender::Close() {
  sender.Close();
}

WebsocketHandshakeBuilder::WebsocketHandshakeBuilder(const HttpRequest& request) : request{request} {
}

std::optional<HttpResponse> WebsocketHandshakeBuilder::Build() const {
  auto upgradeIt = request.headers.find("upgrade");
  if (upgradeIt == request.headers.end()) {
    return std::nullopt;
  }
  auto upgrade = upgradeIt->second;
  common::ToLower(upgrade);
  if (upgrade != "websocket") {
    return std::nullopt;
  }
  auto keyIt = request.headers.find("sec-websocket-key");
  if (keyIt == request.headers.end()) {
    return std::nullopt;
  }
  auto accept = common::Base64(common::SHA1(keyIt->second + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"));
  network::HttpResponse resp;
  resp.status = network::HttpStatus::SwitchingProtocols;
  resp.headers.emplace("Upgrade", "websocket");
  resp.headers.emplace("Connection", "Upgrade");
  resp.headers.emplace("Sec-WebSocket-Accept", std::move(accept));
  return resp;
}

}  // namespace network
