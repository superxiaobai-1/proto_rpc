#include "network/Socket.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>  // snprintf

#include <cassert>

#include <glog/logging.h>

#include "network/InetAddress.h"
#include "network/SocketsOps.h"

using namespace network;
namespace network {
Socket::~Socket() { sockets::close(sockfd_); }

bool Socket::getTcpInfo(struct tcp_info *tcpi) const {
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0, len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char *buf, int len) const {
  struct tcp_info tcpi;
  bool ok = getTcpInfo(&tcpi);
  if (ok) {
    snprintf(
        buf, len,
        "unrecovered=%u "
        "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
        "lost=%u retrans=%u rtt=%u rttvar=%u "
        "sshthresh=%u cwnd=%u total_retrans=%u",
        tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
        tcpi.tcpi_rto,          // Retransmit timeout in usec
        tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
        tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss,
        tcpi.tcpi_lost,     // Lost packets
        tcpi.tcpi_retrans,  // Retransmitted packets out
        tcpi.tcpi_rtt,      // Smoothed round trip time in usec
        tcpi.tcpi_rttvar,   // Medium deviation
        tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
        tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
  }
  return ok;
}

void Socket::bindAddress(const InetAddress &addr) {
  int ret = ::bind(sockfd_, addr.getSockAddr(),
                   static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG(FATAL) << "sockets::bindOrDie";
  }
}

void Socket::listen() {
  int ret = ::listen(sockfd_, SOMAXCONN);
  if (ret < 0) {
    LOG(FATAL) << "sockets::listenOrDie";
  }
}

int Socket::accept(InetAddress *peeraddr) {
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof(addr));

  socklen_t addrlen = static_cast<socklen_t>(sizeof(addr));
  int client_fd =
      ::accept(sockfd_, reinterpret_cast<sockaddr *>(&addr), &addrlen);
  if (client_fd < 0) {
    LOG(ERROR) << "Socket::accept error";
  }
  if (client_fd >= 0) {
    peeraddr->setSockAddrInet6(addr);
  }
  return client_fd;
}

void Socket::shutdownWrite() {
  // sockets::shutdownWrite(sockfd_);
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
    LOG(ERROR) << "sockets::shutdownWrite";
  }
}

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,
               static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
               static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                         static_cast<socklen_t>(sizeof optval));
  if (ret < 0 && on) {
    LOG(ERROR) << "SO_REUSEPORT failed.";
  }
#else
  if (on) {
    LOG_ERROR << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
               static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}
}  // namespace network