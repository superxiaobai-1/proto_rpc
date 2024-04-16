#include "RpcChannel.h"

#include <cassert>

#include <glog/logging.h>
#include <google/protobuf/descriptor.h>

#include "rpc.pb.h"
using namespace network;
RpcChannel::RpcChannel()
    : codec_(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1,
                       std::placeholders::_2)),
      services_(NULL) {
  LOG(INFO) << "RpcChannel::ctor - " << this;
}

RpcChannel::RpcChannel(const TcpConnectionPtr &conn)
    : codec_(std::bind(&RpcChannel::onRpcMessage, this, std::placeholders::_1,
                       std::placeholders::_2)),
      conn_(conn),
      services_(NULL) {
  LOG(INFO) << "RpcChannel::ctor - " << this;
}

RpcChannel::~RpcChannel() {
  LOG(INFO) << "RpcChannel::dtor - " << this;
  for (const auto &outstanding : outstandings_) {
    OutstandingCall out = outstanding.second;
    delete out.response;
    delete out.done;
  }
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller,
                            const ::google::protobuf::Message *request,
                            ::google::protobuf::Message *response,
                            ::google::protobuf::Closure *done) {
  RpcMessage message;
  message.set_type(REQUEST);
  int64_t id = id_.fetch_add(1) + 1;
  message.set_id(id);
  message.set_service(method->service()->full_name());
  message.set_method(method->name());
  message.set_request(request->SerializeAsString());

  OutstandingCall out = {response, done};
  {
    std::unique_lock<std::mutex> lock(mutex_);
    outstandings_[id] = out;
  }
  codec_.send(conn_, message);
}

void RpcChannel::onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
  codec_.onMessage(conn, buf);
}

void RpcChannel::onRpcMessage(const TcpConnectionPtr &conn,
                              const RpcMessagePtr &messagePtr) {
  assert(conn == conn_);
  RpcMessage &message = *messagePtr;
  if (message.type() == RESPONSE) {
    handle_response_msg(messagePtr);
  } else if (message.type() == REQUEST) {
    handle_request_msg(conn, messagePtr);
  }
}

void RpcChannel::handle_response_msg(const RpcMessagePtr &messagePtr) {
  RpcMessage &message = *messagePtr;
  int64_t id = message.id();

  OutstandingCall out = {NULL, NULL};

  {
    std::unique_lock<std::mutex> lock(mutex_);
    auto it = outstandings_.find(id);
    if (it != outstandings_.end()) {
      out = it->second;
      outstandings_.erase(it);
    }
  }

  if (out.response) {
    std::unique_ptr<google::protobuf::Message> d(out.response);
    if (!message.response().empty()) {
      out.response->ParseFromString(message.response());
    }
    if (out.done) {
      out.done->Run();
    }
  }
}

void RpcChannel::handle_request_msg(const TcpConnectionPtr &conn,
                                    const RpcMessagePtr &messagePtr) {
  RpcMessage &message = *messagePtr;
  ErrorCode error = WRONG_PROTO;
  if (services_) {
    std::map<std::string, google::protobuf::Service *>::const_iterator it =
        services_->find(message.service());
    if (it != services_->end()) {
      google::protobuf::Service *service = it->second;
      assert(service != NULL);
      const google::protobuf::ServiceDescriptor *desc =
          service->GetDescriptor();
      const google::protobuf::MethodDescriptor *method =
          desc->FindMethodByName(message.method());
      if (method) {
        std::unique_ptr<google::protobuf::Message> request(
            service->GetRequestPrototype(method).New());
        if (request->ParseFromString(message.request())) {
          google::protobuf::Message *response =
              service->GetResponsePrototype(method).New();
          // response is deleted in doneCallback
          int64_t id = message.id();
          service->CallMethod(
              method, NULL, request.get(), response,
              NewCallback(this, &RpcChannel::doneCallback, response, id));
          error = NO_ERROR;
        } else {
          error = INVALID_REQUEST;
        }
      } else {
        error = NO_METHOD;
      }
    } else {
      error = NO_SERVICE;
    }
  } else {
    error = NO_SERVICE;
  }
  if (error != NO_ERROR) {
    RpcMessage response;
    response.set_type(RESPONSE);
    response.set_id(message.id());
    response.set_error(error);
    codec_.send(conn_, response);
  }
}

void RpcChannel::doneCallback(::google::protobuf::Message *response,
                              int64_t id) {
  std::unique_ptr<google::protobuf::Message> d(response);
  RpcMessage message;
  message.set_type(RESPONSE);
  message.set_id(id);
  message.set_response(response->SerializeAsString());  // FIXME: error check
  codec_.send(conn_, message);
}
