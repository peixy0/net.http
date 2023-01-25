#include "http.hpp"
#include <spdlog/spdlog.h>
#include <cctype>
#include <sstream>
#include "file.hpp"
#include "network.hpp"

namespace {

std::string to_string(network::HttpStatus status) {
  switch (status) {
    case network::HttpStatus::SwitchingProtocols:
      return "101 Switching Protocols";
    case network::HttpStatus::OK:
      return "200 OK";
    case network::HttpStatus::BadRequest:
      return "400 Bad Request";
    case network::HttpStatus::NotFound:
      return "404 Not Found";
  }
  return "";
}

}  // namespace

namespace network {

ConcreteHttpSender::ConcreteHttpSender(TcpSender& sender) : sender{sender} {
}

void ConcreteHttpSender::Send(HttpResponse&& response) {
  std::string respPayload = "HTTP/1.1 " + to_string(response.status) + "\r\n";
  response.headers.emplace("Content-Length", std::to_string(response.body.length()));
  for (const auto& [k, v] : response.headers) {
    respPayload += k + ": " + v + "\r\n";
  }
  respPayload += "\r\n";
  respPayload += std::move(response.body);
  sender.Send(std::move(respPayload));
}

void ConcreteHttpSender::Send(RawHttpResponse&& response) {
  sender.Send(std::move(response.body));
}

void ConcreteHttpSender::Send(FileHttpResponse&& response) {
  os::File file{response.path};
  if (not file.Ok()) {
    spdlog::error("http open(\"{}\"): {}", response.path, strerror(errno));
    HttpResponse resp;
    resp.status = HttpStatus::NotFound;
    resp.headers.emplace("Content-Type", "text/plain");
    resp.body = "Not Found";
    return Send(std::move(resp));
  }
  std::string respPayload = "HTTP/1.1 " + to_string(HttpStatus::OK) + "\r\n";
  response.headers.emplace("Content-Length", std::to_string(file.Size()));
  for (const auto& [k, v] : response.headers) {
    respPayload += k + ": " + v + "\r\n";
  }
  respPayload += "\r\n";
  sender.Send(std::move(respPayload));
  sender.Send(std::move(file));
}

void ConcreteHttpSender::Send(MixedReplaceHeaderHttpResponse&&) {
  std::string respPayload = "HTTP/1.1 " + to_string(HttpStatus::OK) +
                            "\r\n"
                            "Content-Type: multipart/x-mixed-replace; boundary=\"BND\"\r\n\r\n";
  sender.Send(std::move(respPayload));
}

void ConcreteHttpSender::Send(MixedReplaceDataHttpResponse&& response) {
  std::string respPayload = "--BND\r\n";
  response.headers.emplace("Content-Length", std::to_string(response.body.size()));
  for (const auto& [k, v] : response.headers) {
    respPayload += k + ": " + v + "\r\n";
  }
  respPayload += "\r\n";
  respPayload += std::move(response.body);
  respPayload += "\r\n";
  sender.Send(std::move(respPayload));
}

void ConcreteHttpSender::Send(ChunkedHeaderHttpResponse&& response) {
  std::string respPayload = "HTTP/1.1 " + to_string(HttpStatus::OK) +
                            "\r\n"
                            "Transfer-Encoding: chunked\r\n";
  for (const auto& [k, v] : response.headers) {
    respPayload += k + ": " + v + "\r\n";
  }
  respPayload += "\r\n";
  sender.Send(std::move(respPayload));
}

void ConcreteHttpSender::Send(ChunkedDataHttpResponse&& response) {
  std::stringstream ss;
  ss << std::hex << response.body.size();
  ss << "\r\n" << response.body << "\r\n";
  sender.Send(ss.str());
}

void ConcreteHttpSender::Close() {
  sender.Close();
}

HttpLayer::HttpLayer(const HttpOptions& options, std::unique_ptr<HttpParser> parser,
    std::unique_ptr<HttpSender> sender_, HttpProcessorFactory& processorFactory)
    : options{options},
      parser{std::move(parser)},
      sender{std::move(sender_)},
      processor{processorFactory.Create(*sender, *this)} {
}

HttpLayer::~HttpLayer() {
  processor.reset();
  parser.reset();
  sender.reset();
}

void HttpLayer::Process(std::string_view payload) {
  if (upgraded) {
    HttpRequest req;
    req.body = payload;
    processor->Process(std::move(req));
    return;
  }
  parser->Append(payload);
  size_t receivedPayloadSize = parser->GetLength();
  if (receivedPayloadSize > options.maxPayloadSize) {
    spdlog::error(
        "http receivedPayloadSize({}) > options.maxPayloadSize({})", receivedPayloadSize, options.maxPayloadSize);
    sender->Close();
    return;
  }
  spdlog::debug("http received payload: {}", payload);
  while (true) {
    auto request = parser->Parse();
    if (not request) {
      break;
    }
    processor->Process(std::move(*request));
  }
}

void HttpLayer::Upgrade() {
  upgraded = true;
}

HttpLayerFactory::HttpLayerFactory(const HttpOptions& options, HttpProcessorFactory& processorFactory)
    : options{options}, processorFactory{processorFactory} {
}

std::unique_ptr<network::TcpProcessor> HttpLayerFactory::Create(TcpSender& tcpSender) const {
  auto parser = std::make_unique<ConcreteHttpParser>();
  auto sender = std::make_unique<ConcreteHttpSender>(tcpSender);
  return std::make_unique<HttpLayer>(options, std::move(parser), std::move(sender), processorFactory);
}

}  // namespace network
