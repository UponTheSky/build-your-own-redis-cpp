#include "client.h"

void Client::die(const std::string msg) const {
  std::cerr << "[" << errno << "] " << msg << "\n" << std::endl;
  abort();
}

void Client::request() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    die("socket ()");
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1

  int rv = connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv < 0) {
    die("connect");
  }

  char msg[] = "hello";
  write(sockfd, msg, strlen(msg));

  char rbuf[64] = {};
  ssize_t n = read(sockfd, rbuf, sizeof(rbuf) - 1);
  if (n < 0) {
    die("read");
  }

  std::cout << "server says: " << rbuf << std::endl;

  close(sockfd);
}
