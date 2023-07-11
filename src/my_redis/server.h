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

#include "utils.h"

#define K_MAX_MSG 4096


class Server {
  public:
    void run();

  private:
    int32_t one_request(int connfd);
};

#endif
