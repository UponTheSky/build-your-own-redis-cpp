#include "server.h"
#include <stdexcept>

int32_t Server::one_request(int connfd) {
  char rbuf[4 + K_MAX_MSG + 1];
  errno = 0;

  // read the first header
  int32_t err = Utils::read_full(connfd, rbuf, 4);
  if (err) {
    errno == 0 ? Utils::msg("EOF") : Utils::msg("read() error"); // see read_full - returns 0 when EOF
    return err;
  }

  // check the length of the first chunk through the header
  uint32_t len = 0;
  memcpy(&len, rbuf, 4);
  if (len > (uint32_t)K_MAX_MSG) { // len => 4 bytes header
    Utils::msg("too long");
    return -1;
  }

  // read the first body chunk
  err = Utils::read_full(connfd, &rbuf[4], len);
  if (err) {
    Utils::msg("read() error");
    return err;
  }

  // log
  rbuf[4 + len] = '\0';
  std::cout << "client says: " << &rbuf[4] << "\n" << std::endl;

  // reply
  const char reply[] = "world";
  char wbuf[4 + sizeof(reply)];
  len = (uint32_t)strlen(reply);

  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], reply, len); // after the header
  return Utils::write_all(connfd, wbuf, 4 + len);
}

void Server::run() {
  // step 1: generate a socket(ipv4, tcp)
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    Utils::die("socket ()");
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
    Utils::die("bind ()");
  }

  // step 4: listen to the incoming connections
  rv = listen(sockfd, SOMAXCONN);
  if (rv < 0) {
    Utils::die("listen ()");
  }

  // infinite loop that runs until the user connection is remained
  while (true) {
    // step 5: accept the request
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &socklen);
    if (connfd < 0) {
      continue;
    }

    // step 6: parse the request one by one
    while (true) {
      int32_t err = one_request(connfd);
      if (err) {
        break;
      }
    }
    close(connfd);
  }
}
