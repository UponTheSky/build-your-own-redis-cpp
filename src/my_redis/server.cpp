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

void conn_put(std::map<int, Conn*>& fd2conn, struct Conn* conn) {
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
  } while (rv < 0 && errno == EINTR);

  if (rv < 0 && errno == EAGAIN) {
    return false;
  }

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

    // step 8: if the listening sockfd is active, try to accept a new connection
    if (poll_args[0].revents) {
      accept_new_conn(fd2conn, sockfd);
    }

  //   // step 6: parse the request one by one
  //   while (true) {
  //     int32_t err = one_request(connfd);
  //     if (err) {
  //       break;
  //     }
  //   }
  //   close(connfd);
  // }
  }
}
