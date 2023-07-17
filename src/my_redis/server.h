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
#include "hash_table.h"

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

enum Rescode: uint32_t {
  RES_OK = 0,
  RES_ERR = 1,
  RES_NX = 2,
};

struct Entry {
  HashTable::HNode node;
  std::string key;
  std::string val;
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

    int32_t parse_req(const uint8_t* data, size_t len, std::vector<std::string>& out);
    int32_t do_request(const uint8_t* req, uint32_t reqlen, uint32_t& rescode, uint8_t* res, uint32_t& reslen);
    Rescode do_get(std::vector<std::string>& cmd, uint8_t* res, uint32_t& reslen);
    Rescode do_set(std::vector<std::string>& cmd, uint8_t* res, uint32_t& reslen);
    Rescode do_del(std::vector<std::string>& cmd, uint8_t* res, uint32_t& reslen);

    std::map<std::string, std::string> g_map;

    struct {
      HashTable::HMap db;
    } g_data;
};

bool entry_eq(const HashTable::HNode* lhs, const HashTable::HNode* rhs);

#endif
