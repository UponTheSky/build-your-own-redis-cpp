#include "utils.h"

void Utils::msg(const std::string msg) {
  std::cerr << msg << "\n" << std::endl;
}

void Utils::die(const std::string msg) {
  std::cerr << "[" << errno << "]" << msg << "\n" << std::endl;
}

int32_t Utils::read_full(int fd, char* buf, size_t buf_size) {
  while (buf_size > 0) {
    ssize_t rv = read(fd, buf, buf_size);
    if (rv < 0) {
      return -1;
    }

    assert((size_t)rv <= buf_size);
    buf_size -= (size_t)rv;
    buf += rv;
  }

  return 0;
}

int32_t Utils::write_all(int fd, const char* buf, size_t buf_size) {
  while (buf_size > 0) {
    ssize_t rv = write(fd, buf, buf_size);
    if (rv < 0) {
      return -1;
    }

    assert((size_t)rv <= buf_size);
    buf_size -= rv;
    buf += rv;
  }

  return 0;
}
