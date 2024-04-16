#include <unistd.h>

#include <cassert>

#include <glog/logging.h>

#include "network/EventLoop.h"
#include "rpc_framework/RpcServer.h"

#include "monitor.pb.h"

using namespace network;

namespace monitor {

class TestServiceImpl : public TestService {
 public:
  virtual void MonitorInfo(::google::protobuf::RpcController *controller,
                           const ::monitor::TestRequest *request,
                           ::monitor::TestResponse *response,
                           ::google::protobuf::Closure *done) {
    LOG(INFO) << " req:\n" << request->DebugString();
    response->set_status(true);
    std::string c = "hight_" + std::to_string(request->count());
    response->set_cpu_info(c);
    done->Run();
  }
};

}  // namespace monitor

int main(int argc, char *argv[]) {
  // google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(9981);
  monitor::TestServiceImpl impl;
  RpcServer server(&loop, listenAddr);
  server.registerService(&impl);
  server.start();
  loop.loop();
  google::protobuf::ShutdownProtobufLibrary();
}
