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

bool Utils::cmd_is(const std::string& word, const std::string& cmd) {
  return 0 == strcasecmp(word.c_str(), cmd.c_str());
}

uint64_t Utils::str_hash(const uint8_t* data, size_t len) {
  uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++) {
      h = (h +data[i]) * 0x01000193;
    }

    return h;
}
