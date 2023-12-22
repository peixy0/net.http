#include "tcp.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <spdlog/spdlog.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

struct TrySendOperation {
  auto operator()(auto& op) {
    op.Send();
    return op.Done();
  }
};

}  // namespace

namespace network {

TcpSendBuffer::TcpSendBuffer(int fd, std::string_view buffer) : fd{fd}, buffer{buffer}, size{buffer.size()} {
}

void TcpSendBuffer::Send() {
  while (size > 0) {
    ssize_t n = send(fd, buffer.c_str(), size, 0);
    if (n == -1) {
      if (errno == EAGAIN or errno == EWOULDBLOCK) {
        return;
      }
      spdlog::error("tcp send(): {}", strerror(errno));
      return;
    }
    if (n == 0) {
      return;
    }
    size -= n;
    buffer.erase(0, n);
  }
}

bool TcpSendBuffer::Done() const {
  return size == 0;
}

std::string&& TcpSendBuffer::TakeBuffer() {
  return std::move(buffer);
}

TcpSendFile::TcpSendFile(int fd, os::File file_) : fd{fd}, file{std::move(file_)} {
  if (not file.Ok()) {
    size = 0;
    return;
  }
  size = file.Size();
}

void TcpSendFile::Send() {
  while (size > 0) {
    ssize_t n = sendfile(fd, file.Fd(), nullptr, size);
    if (n < 0) {
      if (errno == EAGAIN or errno == EWOULDBLOCK) {
        return;
      }
      spdlog::error("tcp sendfile(): {}", strerror(errno));
      return;
    }
    if (n == 0) {
      return;
    }
    size -= n;
  }
}

bool TcpSendFile::Done() const {
  return size == 0;
}

ConcreteTcpSender::ConcreteTcpSender(int fd, TcpSenderSupervisor& supervisor) : fd{fd}, supervisor{supervisor} {
  int flag = 0;
  int r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof flag);
  if (r < 0) {
    spdlog::error("tcp setsockopt(): {}", strerror(errno));
  }
}

ConcreteTcpSender::~ConcreteTcpSender() {
  ConcreteTcpSender::Close();
}

void ConcreteTcpSender::SendBuffered() {
  std::lock_guard lock{senderMut};
  while (not buffered.empty()) {
    auto& op = buffered.front();
    if (not std::visit(TrySendOperation{}, op)) {
      return;
    }
    buffered.pop_front();
  }
  UnmarkPending();
}

void ConcreteTcpSender::Send(std::string_view buf) {
  std::lock_guard lock{senderMut};
  std::string accumulated{buf};
  while (not buffered.empty()) {
    auto& lastOp = buffered.back();
    if (not std::holds_alternative<TcpSendBuffer>(lastOp)) {
      break;
    }
    auto& lastWriteBuffer = std::get<TcpSendBuffer>(lastOp);
    std::string s = lastWriteBuffer.TakeBuffer();
    s += accumulated;
    accumulated = std::move(s);
    buffered.pop_back();
  }
  TcpSendBuffer op{fd, std::move(accumulated)};
  buffered.emplace_back(std::move(op));
  MarkPending();
}

void ConcreteTcpSender::Send(os::File file) {
  std::lock_guard lock{senderMut};
  TcpSendFile op{fd, std::move(file)};
  buffered.emplace_back(std::move(op));
  MarkPending();
}

void ConcreteTcpSender::Close() {
  std::lock_guard lock{senderMut};
  CloseImpl();
}

void ConcreteTcpSender::CloseImpl() {
  if (fd != -1) {
    shutdown(fd, SHUT_RDWR);
    fd = -1;
  }
}

void ConcreteTcpSender::MarkPending() {
  if (pending) {
    return;
  }
  pending = true;
  supervisor.MarkSenderPending(fd);
}

void ConcreteTcpSender::UnmarkPending() {
  if (not pending) {
    return;
  }
  pending = false;
  supervisor.UnmarkSenderPending(fd);
}

TcpLayer::TcpLayer(TcpProcessorFactory& processorFactory) : processorFactory{processorFactory} {
}

TcpLayer::~TcpLayer() {
  if (localFd != -1) {
    close(localFd);
    localFd = -1;
  }
  if (epollFd != -1) {
    close(epollFd);
    epollFd = -1;
  }
}

void TcpLayer::Start() {
  epollFd = epoll_create1(0);
  if (epollFd < 0) {
    spdlog::error("tcp epoll_create1(): {}", strerror(errno));
    return;
  }
  localFd = CreateSocket();
  if (localFd < 0) {
    return;
  }
  MarkReceiverPending(localFd);
  StartLoop();
}

void TcpLayer::MarkReceiverPending(int peer) const {
  epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = peer;
  int r = epoll_ctl(epollFd, EPOLL_CTL_ADD, peer, &event);
  if (r < 0) {
    spdlog::error("tcp epoll_ctl(): {}", strerror(errno));
    return;
  }
}

void TcpLayer::MarkSenderPending(int peer) const {
  spdlog::debug("tcp mark sender pending: {}", peer);
  epoll_event event;
  event.events = EPOLLIN | EPOLLOUT;
  event.data.fd = peer;
  int r = epoll_ctl(epollFd, EPOLL_CTL_MOD, peer, &event);
  if (r < 0) {
    spdlog::error("tcp epoll_ctl(): {}", strerror(errno));
    return;
  }
}

void TcpLayer::UnmarkSenderPending(int peer) const {
  spdlog::debug("tcp unmark sender pending: {}", peer);
  epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = peer;
  int r = epoll_ctl(epollFd, EPOLL_CTL_MOD, peer, &event);
  if (r < 0) {
    spdlog::error("tcp epoll_ctl(): {}", strerror(errno));
    return;
  }
}

void TcpLayer::SetNonBlocking(int peer) const {
  int flags = fcntl(peer, F_GETFL);
  if (flags < 0) {
    spdlog::error("tcp fcntl(): {}", strerror(errno));
    return;
  }
  flags = fcntl(peer, F_SETFL, flags | O_NONBLOCK);
  if (flags < 0) {
    spdlog::error("tcp fcntl(): {}", strerror(errno));
    return;
  }
}

void TcpLayer::StartLoop() {
  constexpr int maxEvents = 32;
  epoll_event events[maxEvents];
  while (true) {
    int n = epoll_wait(epollFd, events, maxEvents, -1);
    for (int i = 0; i < n; i++) {
      if (events[i].data.fd == localFd) {
        SetupPeer();
        continue;
      }
      if (events[i].events & EPOLLERR) {
        ClosePeer(events[i].data.fd);
        continue;
      }
      if (events[i].events & EPOLLIN) {
        ReadFromPeer(events[i].data.fd);
        continue;
      }
      if (events[i].events & EPOLLOUT) {
        SendToPeer(events[i].data.fd);
        continue;
      }
    }
  }
}

void TcpLayer::SetupPeer() {
  int s = Accept(localFd);
  if (s < 0) {
    return;
  }
  MarkReceiverPending(s);

  auto sender = std::make_unique<ConcreteTcpSender>(s, *this);
  auto processor = processorFactory.Create(*sender);
  connections.try_emplace(s, std::move(processor), std::move(sender));
}

void TcpLayer::ClosePeer(int peer) {
  connections.erase(peer);
  epoll_ctl(epollFd, EPOLL_CTL_DEL, peer, nullptr);
  close(peer);
}

void TcpLayer::ReadFromPeer(int peer) {
  auto it = connections.find(peer);
  if (it == connections.end()) {
    spdlog::error("tcp read from unexpected peer: {}", peer);
    return;
  }
  char buf[512];
  size_t size = sizeof buf;
  ssize_t r = recv(peer, buf, size, 0);
  if (r < 0) {
    if (errno == EAGAIN or errno == EWOULDBLOCK) {
      return;
    }
    ClosePeer(peer);
    return;
  }
  if (r == 0) {
    ClosePeer(peer);
    return;
  }
  auto& context = std::get<TcpConnectionContext>(*it);
  context.processor->Process({buf, buf + r});
}

void TcpLayer::SendToPeer(int peer) const {
  auto it = connections.find(peer);
  if (it == connections.end()) {
    spdlog::error("tcp send to unexpected peer: {}", peer);
    return;
  }

  const auto& context = std::get<TcpConnectionContext>(*it);
  context.sender->SendBuffered();
}

Tcp4Layer::Tcp4Layer(std::string_view host, std::uint16_t port, TcpProcessorFactory& processorFactory)
    : TcpLayer{processorFactory}, host{host}, port{port} {
}

int Tcp4Layer::CreateSocket() const {
  const int one = 1;
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    spdlog::error("tcp socket(): {}", strerror(errno));
    goto out;
  }
  if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof one) < 0) {
    spdlog::error("tcp setsockopt(SO_REUSEADDR): {}", strerror(errno));
    goto out;
  }
  SetNonBlocking(s);

  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof localAddr);
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = inet_addr(host.c_str());
  localAddr.sin_port = htons(port);
  if (bind(s, reinterpret_cast<sockaddr*>(&localAddr), sizeof localAddr) < 0) {
    spdlog::error("tcp bind(): {}", strerror(errno));
    goto out;
  }

  if (listen(s, 8) < 0) {
    spdlog::error("tcp listen(): {}", strerror(errno));
    goto out;
  }
  spdlog::info("tcp listening on {}:{}", host, port);
  return s;

out:
  if (s != -1) {
    close(s);
  }
  return -1;
}

int Tcp4Layer::Accept(int fd) const {
  sockaddr_in peerAddr;
  socklen_t n = sizeof peerAddr;
  memset(&peerAddr, 0, n);
  int s = accept(fd, reinterpret_cast<sockaddr*>(&peerAddr), &n);
  if (s < 0) {
    spdlog::error("tcp accept(): {}", strerror(errno));
    return -1;
  }
  SetNonBlocking(s);
  return s;
}

}  // namespace network
