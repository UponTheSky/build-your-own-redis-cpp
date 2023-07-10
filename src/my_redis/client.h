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


class Client {
  public:
    void request();

  private:
    void die(const std::string msg) const;

};

#endif
