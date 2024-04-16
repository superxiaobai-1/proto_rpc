#include <iostream>
#include <thread>

class EventLoop {
 public:
  EventLoop() : threadId(std::this_thread::get_id()) {}

  void printThreadId() {
    std::cout << "Thread ID: " << threadId << std::endl;
    std::cout << " this " << this << std::endl;
  }

  void printMemberFunctionAddress() {
    std::cout << "Member Function Address: " << &EventLoop::printThreadId
              << std::endl;
  }

  void printMemberVariableAddress() {
    std::cout << "Member Variable Address: " << &threadId << std::endl;
  }

 private:
  std::thread::id threadId;
};

static thread_local EventLoop *t_loopInThisThread = nullptr;

void threadFunction() {
  if (!t_loopInThisThread) {
    t_loopInThisThread = new EventLoop();
  }

  t_loopInThisThread->printThreadId();

  t_loopInThisThread->printMemberFunctionAddress();

  t_loopInThisThread->printMemberVariableAddress();
}

int main() {
  std::thread t1(threadFunction);
  std::thread t2(threadFunction);

  t1.join();
  t2.join();

  delete t_loopInThisThread;

  return 0;
}
