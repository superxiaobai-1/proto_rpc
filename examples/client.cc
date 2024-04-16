#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <cassert>
#include <thread>

#include <glog/logging.h>

#include "network/EventLoop.h"
#include "network/InetAddress.h"
#include "network/TcpClient.h"
#include "network/TcpConnection.h"
#include "rpc_framework/RpcChannel.h"

#include "monitor.pb.h"

using namespace network;

class RpcClient {
 public:
  RpcClient(EventLoop *loop, const InetAddress &serverAddr)
      : loop_(loop),
        client_(loop, serverAddr, "RpcClient"),
        channel_(new RpcChannel),
        stub_(get_pointer(channel_)) {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2));
  }

  void SetMonitorInfo(const monitor::TestRequest &request) {
    monitor::TestResponse *response = new monitor::TestResponse();

    stub_.MonitorInfo(NULL, &request, response,
                      NewCallback(this, &RpcClient::closure, response));
  }

  void connect() { client_.connect(); }

 private:
  void onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
      channel_->setConnection(conn);
    } else {
      RpcClient::connect();
    }
  }

  void closure(monitor::TestResponse *resp) {
    LOG(INFO) << "resq:\n" << resp->DebugString();
  }

  EventLoop *loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  monitor::TestService::Stub stub_;
};

int main(int argc, char *argv[]) {
  // google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "pid = " << getpid();
  if (argc > 1) {
    EventLoop loop;
    InetAddress serverAddr(argv[1], 9981);

    RpcClient rpcClient(&loop, serverAddr);
    rpcClient.connect();
    std::unique_ptr<std::thread> thread_ = nullptr;
    int count = 0;
    thread_ = std::make_unique<std::thread>([&]() {
      while (true) {
        count++;

        monitor::TestRequest request;
        request.set_name("cpu0");
        request.set_count(count);

        rpcClient.SetMonitorInfo(request);
        std::this_thread::sleep_for(std::chrono::seconds(3));
      }
    });
    thread_->detach();
    loop.loop();

  } else {
    printf("Usage: %s host_ip\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}
