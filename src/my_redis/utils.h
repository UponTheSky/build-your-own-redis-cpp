#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <cstdint>
#include <iostream>
#include <string>

namespace Utils {
  void msg(const std::string msg);
  void die(const std::string msg);
  int32_t read_full(int fd, char* buf, size_t buf_size);
  int32_t write_all(int fd, const char* buf, size_t buf_size);
  bool cmd_is(const std::string& word, const std::string& cmd);
};

#endif
