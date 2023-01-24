#pragma once
#include <map>
#include <optional>
#include "network.hpp"

namespace network {

class ConcreteHttpParser : public HttpParser {
public:
  ConcreteHttpParser() = default;
  ConcreteHttpParser(const ConcreteHttpParser&) = delete;
  ConcreteHttpParser(ConcreteHttpParser&&) = delete;
  ConcreteHttpParser& operator=(const ConcreteHttpParser&) = delete;
  ConcreteHttpParser& operator=(ConcreteHttpParser&&) = delete;
  ~ConcreteHttpParser() override = default;

  std::optional<HttpRequest> Parse() override;
  void Append(std::string_view) override;
  size_t GetLength() const override;

private:
  void Reset();
  void SkipWhiteSpaces(std::string&) const;
  bool Consume(std::string&, std::string_view) const;
  std::optional<std::string> ParseToken(std::string&) const;
  bool ParseHeaders(std::string&, HttpHeaders&) const;
  std::optional<HttpHeader> ParseHeader(std::string&) const;
  std::optional<std::string> ParseHeaderField(std::string&) const;
  std::optional<std::string> ParseLine(std::string&) const;
  size_t FindContentLength(const HttpHeaders&) const;
  std::string ParseUriBase(std::string&) const;
  std::string ParseQueryKey(std::string&) const;
  std::string ParseQueryValue(std::string&) const;
  HttpQuery ParseQueryString(std::string&) const;

  std::string payload;
  size_t receivedLength{0};
  std::optional<std::string> method;
  std::optional<std::string> uri;
  std::string uriBase;
  HttpQuery query;
  std::optional<std::string> version;
  bool requestLineEndingParsed{false};
  HttpHeaders headers;
  bool headersParsed{false};
  bool headersEndingParsed{false};
  bool upgraded{false};
  size_t bodyRemaining{0};
};

class ConcreteWebsocketFrameParser : public WebsocketFrameParser {
public:
  ConcreteWebsocketFrameParser() = default;
  ConcreteWebsocketFrameParser(const ConcreteWebsocketFrameParser&) = delete;
  ConcreteWebsocketFrameParser(ConcreteWebsocketFrameParser&&) = delete;
  ConcreteWebsocketFrameParser& operator=(const ConcreteWebsocketFrameParser&) = delete;
  ConcreteWebsocketFrameParser& operator=(ConcreteWebsocketFrameParser&&) = delete;
  ~ConcreteWebsocketFrameParser() override = default;

  std::optional<WebsocketFrame> Parse() override;
  void Append(std::string_view) override;

private:
  std::string payload;
  static constexpr std::uint8_t headerLen = 2;
  static constexpr std::uint8_t maskLen = 4;
  static constexpr std::uint8_t ext1Len = 2;
  static constexpr std::uint8_t ext2Len = 8;
};

}  // namespace network
