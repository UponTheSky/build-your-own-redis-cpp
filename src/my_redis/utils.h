#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <cstdint>
#include <iostream>
#include <string>

class Utils {
  public:
    static void msg(const std::string msg);
    static void die(const std::string msg);
    static int32_t read_full(int fd, char* buf, size_t buf_size);
    static int32_t write_all(int fd, const char* buf, size_t buf_size);
};

#endif
