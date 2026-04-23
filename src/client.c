#include <arpa/inet.h>
#include <err.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

int usage(void);

int main(int argc, char *argv[]) {
  struct sockaddr_in sfd;
  if (argc != 3) {
    return usage();
  }

  inet_pton(AF_INET, argv[1]);

  // create server socket
}

int usage(void) {
  printf("./server <server_ip> <server_port>\n"
         "server_ip: the numeric internet address that reaches the server.\n"
         "server_port: the numeric port that the server is running on \n");
  return EXIT_FAILURE;
}
