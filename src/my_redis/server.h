#ifndef _SERVER_H
#define _SERVER_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "utils.h"

#define K_MAX_MSG 4096

// the state of an ongoing connection
enum State: uint32_t {
  STATE_REQ = 0, // reading requests
  STATE_RES = 1, // sending responses
  STATE_END = 2
};

struct Conn {
  int fd = -1;
  State state = State::STATE_REQ;
  size_t rbuf_size = 0;
  uint8_t rbuf[4 + K_MAX_MSG];
  size_t wbuf_size = 0;
  size_t wbuf_sent = 0;
  uint8_t wbuf[4 + K_MAX_MSG];
};

class Server {
  public:
    void run();

  private:
    int32_t one_request(int connfd);
    void fd_set_nb(int fd);
    void conn_put(std::map<int, Conn*>& fd2conn, struct Conn* conn);
    int32_t accept_new_conn(std::map<int, Conn*>& fd2conn, int fd);
    void connection_io(Conn* conn);
    void state_req(Conn* conn);
    void state_res(Conn* conn);
    bool try_fill_buffer(Conn* conn);
    bool try_one_request(Conn* conn);
    bool try_flush_buffer(Conn* conn);
};


#endif
