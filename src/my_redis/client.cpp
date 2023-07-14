#include "client.h"

void Client::request(int argc, char** argv) {
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

    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i) {
        cmd.push_back(argv[i]);
    }
    int32_t err = send_req(sockfd, cmd);
    if (err) {
        goto L_DONE;
    }
    err = read_res(sockfd);
    if (err) {
        goto L_DONE;
    }


  L_DONE:
    close(sockfd);
}

int32_t Client::send_req(int fd, const std::vector<std::string>& cmd) {
  uint32_t len = 4;
  for (const auto& s: cmd) {
    len += 4 + s.size();
  }

  if (len > (uint32_t)K_MAX_MSG) {
    return -1;
  }

  // write the request
  char wbuf[4 + K_MAX_MSG];
  memcpy(wbuf, &len, 4);
  uint32_t n = cmd.size();
  memcpy(wbuf + 4, &n, 4);

  size_t cur = 8;
  for (const auto& s : cmd) {
    uint32_t p = (uint32_t)s.size();
    memcpy(&wbuf[cur], &p, 4);
    memcpy(&wbuf[cur + 4], s.data(), s.size());
    cur += 4 + s.size();
  }

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

  if (len < 4) {
    Utils::msg("bad response");
    return -1;
  }

  // read the body
  err = Utils::read_full(fd, &rbuf[4], len);
  if (err) {
    Utils::msg("read() error");
    return err;
  }

  uint32_t rescode = 0;
  memcpy(&rescode, &rbuf[4], 4);
  printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
  return 0;
}
