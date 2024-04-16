#pragma once

#include <sys/types.h>
#include <unistd.h>

namespace network {
pid_t getPid();

pid_t getThreadId();

int64_t getNowMs();

int32_t getInt32FromNetByte(const char *buf);

}  // namespace network
