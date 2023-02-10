#pragma once
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "network.hpp"

namespace network {

class ProtocolDispatcherMock : public ProtocolDispatcher {
public:
  MOCK_METHOD(void, SetProcessor, (ProtocolProcessor*), (override));
};

class HttpSenderMock : public HttpSender {
public:
  MOCK_METHOD(void, Send, (HttpResponse &&), (const, override));
  MOCK_METHOD(void, Send, (FileHttpResponse &&), (const, override));
  MOCK_METHOD(void, Send, (MixedReplaceHeaderHttpResponse &&), (const, override));
  MOCK_METHOD(void, Send, (MixedReplaceDataHttpResponse &&), (const, override));
  MOCK_METHOD(void, Send, (ChunkedHeaderHttpResponse &&), (const, override));
  MOCK_METHOD(void, Send, (ChunkedDataHttpResponse &&), (const, override));
  MOCK_METHOD(void, Close, (), (const, override));
};

class WebsocketSenderMock : public WebsocketSender {
public:
  MOCK_METHOD(void, Send, (WebsocketFrame &&), (const, override));
  MOCK_METHOD(void, Close, (), (const, override));
};

}  // namespace network
