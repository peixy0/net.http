#include "app.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>

namespace application {

AppLayer::AppLayer(const AppOptions& options) : options{options} {
}

void AppLayer::Process(network::HttpRequest&& req, network::HttpSender& sender) {
  std::string uri = req.uri;
  if (uri.ends_with("/")) {
    uri += "index.html";
  }
  if (uri.starts_with("/")) {
    uri.erase(0, 1);
  }
  const auto path = std::filesystem::path{options.wwwRoot} / std::filesystem::path{uri};
  const auto pathStr = path.string();
  spdlog::debug("request local path is {}", pathStr);
  if (not pathStr.starts_with(options.wwwRoot)) {
    network::HttpResponse resp;
    resp.status = network::HttpStatus::NotFound;
    resp.headers.emplace("Content-Type", "text/plain");
    resp.body = "No such file";
    return sender.Send(std::move(resp));
  }
  std::string mimeType = MimeTypeOf(pathStr);
  network::FileHttpResponse resp;
  resp.path = std::move(pathStr);
  resp.headers.emplace("Content-type", std::move(mimeType));
  return sender.Send(std::move(resp));
}

std::string AppLayer::MimeTypeOf(std::string_view path) const {
  if (path.ends_with(".html")) {
    return "text/html";
  }
  return "text/plain";
}

void AppLayer::Process(network::WebsocketFrame&& frame, network::WebsocketSender& sender) {
  sender.Send(std::move(frame));
}

}  // namespace application
