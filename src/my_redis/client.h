#ifndef _CLIENT_H
#define _CLIENT_H

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include <iostream>
#include <string>

#include "utils.h"

#define K_MAX_MSG 4096


class Client {
  public:
    void request();

  private:
    int32_t send_req(int fd, const char* text);
    int32_t read_res(int fd);
};

#endif
