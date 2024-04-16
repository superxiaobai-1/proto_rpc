#pragma once

#include <netinet/in.h>

#include <string>
namespace network {
namespace sockets {
const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
}

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress {
 public:
  /// Constructs an endpoint with given port number.
  /// Mostly used in TcpServer listening.
  explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false,
                       bool ipv6 = false);

  /// Constructs an endpoint with given ip and port.
  /// @c ip should be "1.2.3.4"
  InetAddress(const std::string &ip, uint16_t port, bool ipv6 = false);

  /// Constructs an endpoint with given struct @c sockaddr_in
  /// Mostly used when accepting new connections
  explicit InetAddress(const struct sockaddr_in &addr) : addr_(addr) {}

  explicit InetAddress(const struct sockaddr_in6 &addr) : addr6_(addr) {}

  sa_family_t family() const { return addr_.sin_family; }
  std::string toIp() const;
  std::string toIpPort() const;
  uint16_t port() const;

  // default copy/assignment are Okay

  const struct sockaddr *getSockAddr() const {
    return sockets::sockaddr_cast(&addr6_);
  }
  void setSockAddrInet6(const struct sockaddr_in6 &addr6) { addr6_ = addr6; }

  uint32_t ipv4NetEndian() const;
  uint16_t portNetEndian() const { return addr_.sin_port; }

  // resolve hostname to IP address, not changing port or sin_family
  // return true on success.
  // thread safe
  static bool resolve(const std::string &hostname, InetAddress *result);
  // static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t
  // port = 0);

  // set IPv6 ScopeID
  void setScopeId(uint32_t scope_id);

 private:
  union {
    struct sockaddr_in addr_;
    struct sockaddr_in6 addr6_;
  };
};

}  // namespace network
