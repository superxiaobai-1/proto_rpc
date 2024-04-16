#pragma once

#include <sys/epoll.h>

#include <atomic>
#include <functional>
#include <memory>

namespace network {

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel {
 public:
  enum EventType {
    NoneEvent = 0,
    ReadEvent = EPOLLIN,
    WriteEvent = EPOLLOUT,
  };

  typedef std::function<void()> EventCallback;
  // typedef std::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop *loop, int fd);
  ~Channel();

  void handleEvent();
  void setReadCallback(EventCallback cb) { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  int fd() const { return fd_; }
  int events() const { return events_; }
  // used by pollers
  void set_revents(int revt) { revents_ = revt; }
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == EventType::NoneEvent; }

  void enableReading() {
    events_ |= EventType::ReadEvent;
    update();
  }
  void disableReading() {
    events_ &= ~EventType::ReadEvent;
    update();
  }
  void enableWriting() {
    events_ |= EventType::WriteEvent;
    update();
  }
  void disableWriting() {
    events_ &= ~EventType::WriteEvent;
    update();
  }
  void disableAll() {
    events_ = EventType::NoneEvent;
    update();
  }
  bool isWriting() const { return events_ & EventType::WriteEvent; }
  bool isReading() const { return events_ & EventType::ReadEvent; }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  EventLoop *ownerLoop() { return loop_; }
  void remove();

 private:
  void update();

  // static const int kNoneEvent;
  // static const int kReadEvent;
  // static const int kWriteEvent;

  EventLoop *loop_;
  const int fd_;
  int events_;
  int revents_;  // it's the received event types of epoll or poll
  int index_;    // used by Poller.

  std::weak_ptr<void> tie_;
  std::atomic<bool> event_handling_;
  std::atomic<bool> addedToLoop_;
  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}  // namespace network
