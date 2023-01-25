#pragma once
#include <optional>
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

class WebsocketHandshakeBuilder {
public:
  explicit WebsocketHandshakeBuilder(const HttpRequest&);
  std::optional<HttpResponse> Build() const;

private:
  const HttpRequest& request;
};

}  // namespace network
