#include "network/EventLoopThread.h"

#include "network/EventLoop.h"

namespace network {
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(NULL), exiting_(false), mutex_(), callback_(cb) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ !=
      NULL)  // not 100% race-free, eg. threadFunc could be running callback_.
  {
    // still a tiny chance to call destructed object, if threadFunc exits just
    // now. but when EventLoopThread destructs, usually programming is exiting
    // anyway.
    loop_->quit();
    thread_->join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  // assert(!thread_.started());
  // thread_.start();
  thread_ = std::make_unique<std::thread>(
      std::bind(&EventLoopThread::threadFunc, this));
  EventLoop *loop = NULL;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == NULL) {
      cv_.wait(lock);
    }
    loop = loop_;
  }

  return loop;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;

  if (callback_) {
    callback_(&loop);
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;
    cv_.notify_all();
  }

  loop.loop();
  // assert(exiting_);
  std::unique_lock<std::mutex> lock(mutex_);
  loop_ = NULL;
}
}  // namespace network