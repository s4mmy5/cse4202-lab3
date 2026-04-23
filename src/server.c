#include <err.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 50

int init_socket(void);
int usage(void);
void set_non_blocking_io(int fd);

int main(int argc, char *argv[]) {
  // 1. Open all required files
  FILE *in_file, *out_file;
  int sfd, epoll_fd, ret, ready;
  FILE **fragments = NULL;
  int fragments_cnt = 0;
  char *line = NULL;
  char *fragment_filename = NULL;
  size_t line_len = 0;
  struct epoll_event ev = NULL;
  struct epoll_event *revents = NULL;
  int epollfd_count = 0;

  if (argc != 2)
    return usage();

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

  if (NULL == (out_file = fopen(
                   line, "w"))) // open output file and truncate old contents
  {
    printf("Could not open output file");
    err(EXIT_FAILURE, "fopen");
  }

  while (-1 != getline(&line, &line_len, in_file)) {
    fragments = realloc(fragments, (++fragments_cnt) * sizeof(FILE *));

    if (NULL == fragments) {
      printf("Could not allocate fragments array\n");
      err(EXIT_FAILURE, "realloc");
    }

    line[strcspn(line, "\n")] = '\0';
    fragments[fragments_cnt - 1] = fopen(line, "r");

    if (NULL == fragments[fragments_cnt - 1]) {
      printf("Could not open fragment file %d\n", fragments_cnt);
      err(EXIT_FAILURE, "fopen");
    }
  }

  // initialize epoll fd
  epoll_fd = epoll_create1(0);
  if (-1 == epoll_fd) {
    err(EXIT_FAILURE, "epoll_create1");
  }

  // 2. Connect with all clients
  sfd = init_socket();

  // initialize socket epoll_event
  ev.events = EPOLLIN;
  ev.data.fd = sfd;
  if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev)) {
    err(EXIT_FAILURE, "epoll_ctl");
  }

  revents = realloc(revents, sizeof(struct epoll_event) * ++epollfd_count);

  // set non blocking readers
  set_non_blocking_io(sfd);

  // wait for connections
  while ((ready = epoll_wait(epoll_fd, revents, epollfd_count, -1)) != -1) {
    for (int i = 0; i < ready; i++) {
      if (revents[i].data.fd == sfd) {
        // accept connections on server socket
      } else {
        // interact with client
      }
    }
  }
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
              NULL, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);

  printf("Server Host: %s\n"
         "Server Port: %s\n",
         server_host, server_serv);

  return sfd;
}

int usage(void) {
  printf("./server <input_file>"
         "input_file: Provide a file containing fragments.");
  return EXIT_FAILURE;
}

void set_non_blocking_io(int fd) {
  int flags;

  flags = fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
}
