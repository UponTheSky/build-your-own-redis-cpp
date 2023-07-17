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

void Server::fd_set_nb(int fd) {
  errno = 0;
  int flags = fcntl(fd, F_GETFL, 0);
  if (errno) {
    Utils::die("fcntl error");
    return;
  }

  flags |= O_NONBLOCK;

  errno = 0;
  fcntl(fd, F_SETFL, flags);
  if (errno) {
    Utils::die("fcntl error");
  }
}

void Server::conn_put(std::map<int, Conn*>& fd2conn, struct Conn* conn) {
  fd2conn[conn->fd] = conn;
}

int32_t Server::accept_new_conn(std::map<int, Conn*>& fd2conn, int fd) {
  // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*)&client_addr, &socklen);
    if (connfd < 0) {
      Utils::msg("accept() error");
      return -1;
    }

    // set the new connfd as nonblocking
    fd_set_nb(connfd);

    // create a struct Conn
    struct Conn* conn = new Conn;

    conn->fd = connfd;
    conn->state = State::STATE_REQ;
    conn_put(fd2conn, conn);

    return 0;
}

void Server::connection_io(Conn* conn) {
  switch (conn->state) {
    case State::STATE_REQ:
      state_req(conn);
      break;
    case State::STATE_RES:
      state_res(conn);
      break;
    default:
      assert(0);
  }
}

void Server::state_req(Conn* conn) {
  while (try_fill_buffer(conn)) {}
}

bool Server::try_fill_buffer(Conn* conn) {
  assert(conn->rbuf_size < sizeof(conn->rbuf));
  ssize_t rv = 0;

  do {
    size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
    rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
  } while (rv < 0 && errno == EINTR); // even after interruption retries are required

  // the connection is not ready yet
  // EAGAIN: resource temporarily unavailable
  if (rv < 0 && errno == EAGAIN) {
    return false;
  }

  if (rv < 0) {
    Utils::msg("read() error");
    conn->state = State::STATE_END;
    return false;
  }

  if (rv == 0) {
    conn->rbuf_size > 0 ? Utils::msg("unexpected EOF") : Utils::msg("EOF");
    conn->state = State::STATE_END;
    return false;
  }

  conn->rbuf_size += (size_t)rv;
  assert(conn->rbuf_size <= sizeof(conn->rbuf));

  while (try_one_request(conn)) {};
  return (conn->state == State::STATE_REQ);
}

bool Server::try_one_request(Conn* conn) {
  // guard clauses
  if (conn->rbuf_size < 4) {
    return false;
  }

  uint32_t len = 0;
  memcpy(&len, &conn->rbuf[0], 4);
  if (len > (uint32_t)K_MAX_MSG) {
    Utils::msg("too long");
    conn->state = State::STATE_END;
    return false;
  }

  if (conn->rbuf_size < 4 + len) {
    return false;
  }

  // getting a request and generate a response
  uint32_t rescode = 0;
  uint32_t wlen = 0;
  int32_t err = do_request(&conn->rbuf[4], len, rescode, &conn->wbuf[4 + 4], wlen);

  if (err) {
    conn->state = State::STATE_END;
    return false;
  }

  wlen += 4;
  memcpy(&conn->wbuf[0], &wlen, 4);
  memcpy(&conn->wbuf[4], &rescode, 4);
  conn->wbuf_size = 4 + wlen;

  // remove the request from the buffer
  size_t remain = conn->rbuf_size - 4 - len;
  if (remain) {
    memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
  }
  conn->rbuf_size = remain;

  conn->state = State::STATE_RES;
  state_res(conn);

  return (conn->state == State::STATE_REQ);
}

int32_t Server::do_request(
  const uint8_t* req,
  uint32_t reqlen,
  uint32_t& rescode,
  uint8_t* res,
  uint32_t& reslen
) {
  std::vector<std::string> cmd;
  if (0 != parse_req(req, reqlen, cmd)) {
    Utils::msg("bad req");
    return -1;
  }

  if (cmd.size() == 2 && Utils::cmd_is(cmd[0], "get")) {
    rescode = do_get(cmd, res, reslen);
  } else if (cmd.size() == 3 && Utils::cmd_is(cmd[0], "set")) {
    rescode = do_set(cmd, res, reslen);
  } else if (cmd.size() == 2 && Utils::cmd_is(cmd[0], "del")) {
    rescode = do_del(cmd, res, reslen);
  } else {
    rescode = RES_ERR;
    const char* msg = "Unknown cmd";
    strcpy((char*)res, msg);
    reslen = strlen(msg);
    return 0;
  }

  return 0;
}

int32_t Server::parse_req(
  const uint8_t* data,
  size_t len,
  std::vector<std::string>& out
) {
  if (len < 4) {
    return -1;
  }

  uint32_t n = 0;
  memcpy(&n, &data[0], 4);
  if (n > (uint32_t)K_MAX_MSG) {
    return -1;
  }

  size_t pos = 4;
  while (n--) {
    if (pos + 4 > len) {
      return -1;
    }

    uint32_t sz = 0;
    memcpy(&sz, &data[pos], 4);
    if (pos + 4 + sz > len) {
      return -1;
    }

    out.push_back(std::string((char*)&data[pos + 4], sz));
    pos += 4 + sz;
  }

  if (pos != len) {
    return -1;
  }

  return 0;
}

void Server::state_res(Conn* conn) {
  while (try_flush_buffer(conn)) {}
}

bool Server::try_flush_buffer(Conn* conn) {
  ssize_t rv = 0;

  do {
    size_t remain = conn->wbuf_size - conn->wbuf_sent;
    rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
  } while (rv < 0 && errno == EINTR);

  if (rv < 0 && errno == EAGAIN) {
    return false;
  }

  if (rv < 0) {
    Utils::msg("write() error");
    conn->state = State::STATE_END;
    return false;
  }

  conn->wbuf_sent += (size_t)rv;
  assert (conn->wbuf_sent <= conn->wbuf_size);

  if (conn->wbuf_sent == conn->wbuf_size) {
    conn->state = State::STATE_REQ;
    conn->wbuf_sent = 0;
    conn->wbuf_size = 0;
    return false;
  }

  return true;
}

bool entry_eq(const HashTable::HNode* lhs, const HashTable::HNode* rhs) {
  Entry* le = container_of(lhs, Entry, node);
  Entry* re = container_of(rhs, Entry, node);
  return lhs->hcode == rhs->hcode && le->key == re->key;
}

Rescode Server::do_get(std::vector<std::string>& cmd, uint8_t* res, uint32_t& reslen) {
  Entry key;
  key.key.swap(cmd[1]);
  key.node.hcode = Utils::str_hash((uint8_t*)key.key.data(), key.key.size());

  HashTable::HNode* node = HashTable::hm_lookup(g_data.db, &key.node, &entry_eq);
  if (!node) {
    return Rescode::RES_NX;
  }

  std::string& val = container_of(node, Entry, node)->val; // the pointer to the Entry instance containing the node
  assert(val.size() < (size_t)K_MAX_MSG);
  memcpy(res, val.data(), val.size());
  reslen = (uint32_t)val.size();

  return Rescode::RES_OK;
}

Rescode Server::do_set(
  std::vector<std::string>& cmd,
  uint8_t* res,
  uint32_t& reslen
) {
  Entry key;
  key.key.swap(cmd[1]);
  key.node.hcode = Utils::str_hash((uint8_t*)key.key.data(), key.key.size());

  HashTable::HNode* node = HashTable::hm_lookup(g_data.db, &key.node, &entry_eq);

  if (node) {
    container_of(node, Entry, node)->val.swap(cmd[2]);
  } else {
    Entry* ent = new Entry();
    ent->key.swap(key.key);
    ent->node.hcode = key.node.hcode;
    ent->val.swap(cmd[2]);
    hm_insert(g_data.db, &ent->node);
  }

  return Rescode::RES_OK;
}

Rescode Server::do_del(
  std::vector<std::string>& cmd,
  uint8_t* res,
  uint32_t& reslen
) {
  Entry key;
  key.key.swap(cmd[1]);
  key.node.hcode = Utils::str_hash((uint8_t*)key.key.data(), key.key.size());

  HashTable::HNode* node = HashTable::hm_pop(g_data.db, &key.node, &entry_eq);
  if (node) {
    delete container_of(node, Entry, node);
  }

  return Rescode::RES_OK;
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

  // step 5: get ready for the event loop
  fd_set_nb(sockfd);
  std::map<int, Conn*> fd2conn;
  std::vector<struct pollfd> poll_args;

  // infinite loop that runs until the user connection is remained
  while (true) {
    // step 6: prepare the args of poll()
    poll_args.clear();
    struct pollfd pfd = { sockfd, POLLIN, 0 };
    poll_args.push_back(pfd);

    for (const auto& [connfd, conn] : fd2conn) {
      if (!conn) {
        continue;
      }

      struct pollfd pfd = {};
      pfd.fd = conn->fd,
      pfd.events = (conn->state == State::STATE_REQ ? POLLIN : POLLOUT) | POLL_ERR;

      poll_args.push_back(pfd);
    }

    // step 7: poll for the active fds
    int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000); // 1000 => timeout
    if (rv < 0) {
      Utils::die("poll");
    }

    // step 8: process those active conns
    for (size_t i = 1; i < poll_args.size(); i++) {
      if (poll_args[i].revents)  { // if event has returned
        Conn* conn = fd2conn[poll_args[i].fd];
        connection_io(conn);
        if (conn->state == State::STATE_END) {
          // destroy the connection
          fd2conn[conn->fd] = nullptr;
          close(conn->fd);
          delete conn;
        }
      }
    }

    // step 9: if the listening sockfd is active, try to accept a new connection
    if (poll_args[0].revents) {
      accept_new_conn(fd2conn, sockfd);
    }
  }
}
