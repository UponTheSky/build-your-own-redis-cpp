#ifndef _SERVER_H
#define _SERVER_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <string>


class Server {
  public:
    void run();

  private:
    void do_something(int connfd);
    void msg(const std::string msg) const;
    void die(const std::string msg) const;
};

#endif
