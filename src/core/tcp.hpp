#pragma once
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <variant>
#include "network.hpp"

namespace network {

class TcpSendBuffer {
public:
  TcpSendBuffer(int, std::string_view);
  TcpSendBuffer(TcpSendBuffer&) = delete;
  TcpSendBuffer(TcpSendBuffer&&) = default;
  TcpSendBuffer& operator=(TcpSendBuffer&) = delete;
  TcpSendBuffer& operator=(TcpSendBuffer&&) = default;
  ~TcpSendBuffer() = default;
  void Send();
  bool Done() const;
  std::string Buffer() const;

private:
  int fd;
  std::string buffer;
  size_t size{0};
};

class TcpSendFile {
public:
  TcpSendFile(int, os::File);
  TcpSendFile(TcpSendFile&) = delete;
  TcpSendFile(TcpSendFile&&) = default;
  TcpSendFile& operator=(TcpSendFile&) = delete;
  TcpSendFile& operator=(TcpSendFile&&) = default;
  ~TcpSendFile() = default;
  void Send();
  bool Done() const;

private:
  int fd;
  os::File file;
  size_t size{0};
};

using TcpSendOperation = std::variant<TcpSendBuffer, TcpSendFile>;

class ConcreteTcpSender final : public TcpSender {
public:
  ConcreteTcpSender(int, TcpSenderSupervisor&);
  ConcreteTcpSender(const ConcreteTcpSender&) = delete;
  ConcreteTcpSender(ConcreteTcpSender&&) = delete;
  ConcreteTcpSender& operator=(const ConcreteTcpSender&) = delete;
  ConcreteTcpSender& operator=(ConcreteTcpSender&&) = delete;
  ~ConcreteTcpSender() override;

  void Send(std::string_view) override;
  void Send(os::File) override;
  void SendBuffered() override;
  void Close() override;

private:
  void CloseImpl();
  void MarkPending();
  void UnmarkPending();

  int fd;
  TcpSenderSupervisor& supervisor;
  std::deque<TcpSendOperation> buffered;
  bool pending{false};
  std::mutex senderMut;
};

struct TcpConnectionContext {
  ~TcpConnectionContext() {
    processor.reset();
    sender.reset();
  }
  std::unique_ptr<TcpProcessor> processor;
  std::unique_ptr<TcpSender> sender;
};

class TcpLayer : public TcpSenderSupervisor {
public:
  explicit TcpLayer(TcpProcessorFactory&);
  TcpLayer(const TcpLayer&) = delete;
  TcpLayer(TcpLayer&&) = delete;
  TcpLayer& operator=(const TcpLayer&) = delete;
  TcpLayer& operator=(TcpLayer&&) = delete;
  ~TcpLayer() override;

  void Start();
  void MarkSenderPending(int) const override;
  void UnmarkSenderPending(int) const override;

protected:
  virtual int CreateSocket() const = 0;
  virtual int Accept(int) const = 0;
  void SetNonBlocking(int) const;

private:
  void StartLoop();
  void SetupPeer();
  void ClosePeer(int);
  void ReadFromPeer(int);
  void SendToPeer(int) const;
  void MarkReceiverPending(int) const;

  TcpProcessorFactory& processorFactory;
  int localFd{-1};
  int epollFd{-1};
  std::unordered_map<int, TcpConnectionContext> connections;
};

class Tcp4Layer final : public TcpLayer {
public:
  Tcp4Layer(std::string_view, std::uint16_t, TcpProcessorFactory&);

protected:
  int CreateSocket() const override;
  int Accept(int) const override;

private:
  std::string host;
  std::uint16_t port;
};

}  // namespace network
