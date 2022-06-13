#include "app.hpp"
#include <spdlog/spdlog.h>
#include <utmp.h>
#include <chrono>
#include <ctime>
#include <thread>
#include <vector>
#include "network.hpp"

namespace application {

void AppBtmpContentLoader::Run() {
  daemon = std::thread([this] {
    while (true) {
      LoadContent();
    }
  });
}

void AppBtmpContentLoader::LoadContent() {
  std::vector<utmp> entries;
  utmpname("/var/log/btmp");
  setutent();
  for (;;) {
    auto ent = getutent();
    if (not ent) {
      break;
    }
    entries.emplace_back(*ent);
  }
  endutent();

  std::string result;
  int n = 0;
  for (auto it = entries.rbegin(); it != entries.rend() and n < 50; it++) {
    time_t t = it->ut_tv.tv_sec;
    char timebuf[50];
    std::strftime(timebuf, sizeof timebuf, "%c %Z", std::gmtime(&t));
    result += timebuf;
    result += " ";
    result += it->ut_line;
    result += " ";
    result += it->ut_host;
    result += "\t";
    result += it->ut_user;
    result += "\n";
    n++;
  }

  {
    std::unique_lock l{btmpMutex};
    btmpContent = result;
  }
  {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(10min);
  }
}

std::string AppBtmpContentLoader::GetContent() const {
  std::unique_lock l{btmpMutex};
  return btmpContent;
}

AppLayer::AppLayer(AppBtmpContentLoader& btmpLoader) : btmpLoader{btmpLoader} {
}

void AppLayer::RequestLastbLog(network::HttpRequest&&, network::HttpSender& sender) {
  network::HttpResponse resp;
  resp.status = network::HttpStatus::OK;
  resp.headers.emplace("Content-Type", "text/plain");
  resp.body = btmpLoader.GetContent();
  return sender.Send(std::move(resp));
}

void AppLayer::RequestNginxLog(network::HttpRequest&&, network::HttpSender& sender) {
  network::FileHttpResponse resp;
  resp.path = "/var/log/nginx/access.log";
  resp.headers.emplace("Content-Type", "text/plain");
  return sender.Send(std::move(resp));
}

}  // namespace application
