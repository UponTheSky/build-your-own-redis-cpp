#include "client.h"

int main(int argc, char** argv) {
  Client client;

  client.request(argc, argv);

  return 0;
}
