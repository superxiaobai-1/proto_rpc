#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

#include "rpc.pb.h"

namespace network {

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

class RpcMessage;
typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
extern const char rpctag[];  // = "RPC0";

// wire format
//
// Field     Length  Content
//
// size      4-byte  N+8
// "RPC0"    4-byte
// payload   N-byte
// checksum  4-byte  adler32 of "RPC0"+payload
//

class ProtoRpcCodec {
 public:
  const static int kHeaderLen = sizeof(int32_t);
  const static int kChecksumLen = sizeof(int32_t);
  const static int kMaxMessageLen =
      64 * 1024 * 1024;  // same as codec_stream.h kDefaultTotalBytesLimit
  enum ErrorCode {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError,
  };
  typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
  typedef std::function<void(const TcpConnectionPtr &, const RpcMessagePtr &)>
      ProtobufMessageCallback;

  typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

  explicit ProtoRpcCodec(const ProtobufMessageCallback &messageCb)
      : messageCallback_(messageCb) {}
  ~ProtoRpcCodec() {}
  void send(const TcpConnectionPtr &conn,
            const ::google::protobuf::Message &message);

  void onMessage(const TcpConnectionPtr &conn, Buffer *buf);
  bool parseFromBuffer(const void *buf, int len,
                       google::protobuf::Message *message);
  int serializeToBuffer(const google::protobuf::Message &message, Buffer *buf);
  ErrorCode parse(const char *buf, int len,
                  ::google::protobuf::Message *message);
  void fillEmptyBuffer(Buffer *buf, const google::protobuf::Message &message);

  static int32_t checksum(const void *buf, int len);
  static bool validateChecksum(const char *buf, int len);
  static int32_t asInt32(const char *buf);

 private:
  ProtobufMessageCallback messageCallback_;
  int kMinMessageLen = 4;
  const std::string tag_ = "RPC0";
};

}  // namespace network
