#include "client.h"

void Client::request() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    Utils::die("socket ()");
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1

  int rv = connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv < 0) {
    Utils::die("connect");
  }

  const char *query_list[3] = {"hello1", "hello2", "hello3"};

  for (size_t i = 0; i < 3; ++i) {
    int32_t err = send_req(sockfd, query_list[i]);
    if (err) {
        goto L_DONE;
    }
  }
  for (size_t i = 0; i < 3; ++i) {
    int32_t err = read_res(sockfd);
    if (err) {
        goto L_DONE;
    }
  }

  L_DONE:
    close(sockfd);
}

int32_t Client::send_req(int fd, const char* text) {
  uint32_t len = (uint32_t)strlen(text);
  if (len > (uint32_t)K_MAX_MSG) {
    return -1;
  }

  // write the request
  char wbuf[4 + K_MAX_MSG];
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], text, len);
  return Utils::write_all(fd, wbuf, 4 + len);
}

int32_t Client::read_res(int fd) {
  // read the response
  char rbuf[4 + K_MAX_MSG + 1];
  errno = 0;
  int32_t err = Utils::read_full(fd, rbuf, 4);
  if (err) {
    errno == 0 ? Utils::msg("EOF") : Utils::msg("read() error");
    return err;
  }

  // read the header
  uint32_t len = 0;
  memcpy(&len, rbuf, 4);
  if (len > (uint32_t)K_MAX_MSG) {
    Utils::msg("too long");
    return -1;
  }

  // read the body
  err = Utils::read_full(fd, &rbuf[4], len);
  if (err) {
    Utils::msg("read() error");
    return err;
  }

  rbuf[4 + len] = '\0';
  std::cout << "server says: " << &rbuf[4] << std::endl;

  return 0;
}
