#include "RpcCodec.h"

#include <zlib.h>

#include <google/protobuf/message.h>

#include "network/Endian.h"
#include "network/TcpConnection.h"

using namespace network;

namespace network {
void ProtoRpcCodec::send(const TcpConnectionPtr &conn,
                         const ::google::protobuf::Message &message) {
  Buffer buf;
  fillEmptyBuffer(&buf, message);
  conn->send(&buf);
}

void ProtoRpcCodec::onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
  while (buf->readableBytes() >=
         static_cast<uint32_t>(kMinMessageLen + kHeaderLen)) {
    const int32_t len = buf->peekInt32();
    if (len > kMaxMessageLen || len < kMinMessageLen) {
      // errorCallback_(conn, buf, receiveTime, kInvalidLength);
      break;
    } else if (buf->readableBytes() >= size_t(kHeaderLen + len)) {
      // if (rawCb_ && !rawCb_(conn, StringPiece(buf->peek(), kHeaderLen + len),
      //                       receiveTime)) {
      //   buf->retrieve(kHeaderLen + len);
      //   continue;
      // }
      RpcMessagePtr message(new RpcMessage());
      // FIXME: can we move deserialization & callback to other thread?
      ErrorCode errorCode = parse(buf->peek() + kHeaderLen, len, message.get());
      if (errorCode == kNoError) {
        // FIXME: try { } catch (...) { }
        messageCallback_(conn, message);
        buf->retrieve(kHeaderLen + len);
      } else {
        // errorCallback_(conn, buf, receiveTime, errorCode);
        break;
      }
    } else {
      break;
    }
  }
}

bool ProtoRpcCodec::parseFromBuffer(const void *buf, int len,
                                    google::protobuf::Message *message) {
  return message->ParseFromArray(buf, len);
}

int ProtoRpcCodec::serializeToBuffer(const google::protobuf::Message &message,
                                     Buffer *buf) {
#if GOOGLE_PROTOBUF_VERSION > 3009002
  int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
  int byte_size = message.ByteSize();
#endif
  buf->ensureWritableBytes(byte_size + kChecksumLen);

  uint8_t *start = reinterpret_cast<uint8_t *>(buf->beginWrite());
  uint8_t *end = message.SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size) {
  }
  buf->hasWritten(byte_size);
  return byte_size;
}

ProtoRpcCodec::ErrorCode ProtoRpcCodec::parse(
    const char *buf, int len, ::google::protobuf::Message *message) {
  ErrorCode error = kNoError;

  if (validateChecksum(buf, len)) {
    if (memcmp(buf, tag_.data(), tag_.size()) == 0) {
      // parse from buffer
      const char *data = buf + tag_.size();
      int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
      if (parseFromBuffer(data, dataLen, message)) {
        error = kNoError;
      } else {
        error = kParseError;
      }
    } else {
      error = kUnknownMessageType;
    }
  } else {
    error = kCheckSumError;
  }

  return error;
}

void ProtoRpcCodec::fillEmptyBuffer(Buffer *buf,
                                    const google::protobuf::Message &message) {
  assert(buf->readableBytes() == 0);
  buf->append(tag_);

  int byte_size = serializeToBuffer(message, buf);

  int32_t checkSum =
      checksum(buf->peek(), static_cast<int>(buf->readableBytes()));
  buf->appendInt32(checkSum);
  assert(buf->readableBytes() == tag_.size() + byte_size + kChecksumLen);
  (void)byte_size;
  int32_t len =
      sockets::hostToNetwork32(static_cast<int32_t>(buf->readableBytes()));
  buf->prepend(&len, sizeof len);
}

int32_t ProtoRpcCodec::asInt32(const char *buf) {
  int32_t be32 = 0;
  ::memcpy(&be32, buf, sizeof(be32));
  return sockets::networkToHost32(be32);
}

int32_t ProtoRpcCodec::checksum(const void *buf, int len) {
  return static_cast<int32_t>(
      ::adler32(1, static_cast<const Bytef *>(buf), len));
}

bool ProtoRpcCodec::validateChecksum(const char *buf, int len) {
  // check sum
  int32_t expectedCheckSum = asInt32(buf + len - kChecksumLen);
  int32_t checkSum = checksum(buf, len - kChecksumLen);
  return checkSum == expectedCheckSum;
}

}  // namespace network
