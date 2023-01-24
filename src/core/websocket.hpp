#pragma once
#include "network.hpp"

namespace network {

class ConcreteWebsocketSender : public WebsocketSender {
public:
  explicit ConcreteWebsocketSender(HttpSender&);
  ConcreteWebsocketSender(const ConcreteWebsocketSender&) = delete;
  ConcreteWebsocketSender(ConcreteWebsocketSender&&) = delete;
  ConcreteWebsocketSender& operator=(const ConcreteWebsocketSender&) = delete;
  ConcreteWebsocketSender& operator=(ConcreteWebsocketSender&&) = delete;
  ~ConcreteWebsocketSender() override = default;

  void Send(WebsocketFrame&&) override;
  void Close() override;

private:
  HttpSender& sender;
};

}  // namespace network
