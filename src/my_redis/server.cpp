#include "server.h"
#include <stdexcept>

void Server::msg(const std::string msg) const {
  std::cerr << msg << "\n" << std::endl;
}

void Server::die(const std::string msg) const {
  std::cerr << "[" << errno << "]" << msg << "\n" << std::endl;
}

void Server::do_something(int connfd) {
  // using the C APIs
  char rbuf[64] = {};

  ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
  if (n < 0) {
    msg("read() error");
    return;
  }

  std::cout << "client says: " << rbuf << "\n" << std::endl;

  char wbuf[] = "world";
  write(connfd, wbuf, strlen(wbuf));
}

void Server::run() {
  // step 1: generate a socket(ipv4, tcp)
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    die("socket ()");
  }

  // step 2: set socket options: socket level control(SOL_SOCKET), reusing addr(SO_REUSEADDR)
  int val = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // step 3: bind the socket to the client address
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234); // port 1234
  addr.sin_addr.s_addr = ntohl(0); // host 0.0.0.0

  int rv = bind(sockfd, (const sockaddr *)&addr, sizeof(addr));
  if (rv < 0) {
    die("bind ()");
  }

  // step 4: listen to the incoming connections
  rv = listen(sockfd, SOMAXCONN);
  if (rv < 0) {
    die("listen ()");
  }

  while (true) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &socklen);
    if (connfd < 0) {
      continue;
    }

    do_something(connfd);
    close(connfd);
  }
}
