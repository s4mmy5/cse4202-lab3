#include <cstdlib>
#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 50

int init_socket(void);
void usage(void);

int main(int argc, char *argv[]) {
  // 1. Open all required files
  FILE *in_file, *out_file;
  FILE **fragments;
  int fragments_cnt;
  char *line;
  size_t line_len;
  int sfd;

  if (argc != 2)
    usage();

  if (NULL == (in_file = fopen(argv[1], "r"))) {
    printf("Could not open input file\n");
    usage();
    err(EXIT_FAILURE, "fopen");
  }

  if (-1 == getline(&line, &line_len, in_file)) // get output file name
    {
      printf("Input file is emtpy\n");
      err(EXIT_FAILURE, "getline");
  }

  if (NULL == (out_file = fopen(line, "w"))) // open output file and truncate old contents
  {
    printf("Could not open output file");
    err(EXIT_FAILURE, "fopen");
  }

  while (-1 != getline(&line, &line_len, in_file)) {
    fragments = realloc(fragments,
  }

  // 2. Connect with all clients
  sfd = init_socket();
}

int init_socket(void) {
  int sfd;
  char server_host[NI_MAXHOST], server_serv[NI_MAXSERV];
  struct sockaddr_in my_addr = {0};

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    err(EXIT_FAILURE, "socket");

  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = INADDR_ANY;

  if (-1 == bind(sfd, (struct sockaddr *)&my_addr, sizeof(my_addr)))
    err(EXIT_FAILURE, "bind");

  if (-1 == listen(sfd, LISTEN_BACKLOG))
    err(EXIT_FAILURE, "listen");

  socklen_t sfd_len = sizeof(my_addr);
  getsockname(sfd, (struct sockaddr *)&my_addr, &sfd_len);

  getnameinfo((struct sockaddr *)&my_addr, sfd_len, server_host, NI_MAXHOST,
              server_serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

  printf("Server Host: %s\n"
         "Server Port: %s\n",
         server_host, server_serv);

  return sfd;
}

void usage(void) {
  printf("./server <input_file>"
         "input_file: Provide a file containing fragments.");
}
