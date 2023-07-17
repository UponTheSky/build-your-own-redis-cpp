#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <cstdint>
#include <iostream>
#include <string>

#define container_of(ptr, type, member) ({ \
  const typeof(((type*)0)->member)* __mem_ptr = (ptr); \
  (type*)((char*)__mem_ptr - offsetof(type, member)); })

namespace Utils {
  void msg(const std::string msg);
  void die(const std::string msg);
  int32_t read_full(int fd, char* buf, size_t buf_size);
  int32_t write_all(int fd, const char* buf, size_t buf_size);
  bool cmd_is(const std::string& word, const std::string& cmd);
  uint64_t str_hash(const uint8_t* data, size_t len);
};

#endif
