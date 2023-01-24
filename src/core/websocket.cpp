#include "websocket.hpp"

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

}  // namespace network
