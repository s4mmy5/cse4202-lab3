#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 50

typedef struct clients_list {
  ssize_t size;
  int *fds;
} clients_list_t;

typedef struct epoll_list {
  ssize_t size;
  struct epoll_event *evs;
} event_list_t;

typedef struct fragments_list {
  ssize_t size;
  FILE **list;
} fragments_list_t;

int init_socket(void);
int usage(void);
void set_non_blocking_io(int fd);
int get_fragments(fragments_list_t *fragments, FILE *in_file);

int main(int argc, char *argv[]) {
  // 1. Open all required files
  FILE *in_file, *out_file;
  int sfd, cfd, epoll_fd, ret, ready;
  char *line = NULL;
  size_t line_len = 0;
  struct sockaddr_in peer_addr = {0};
  socklen_t peer_addr_size = 0;
  clients_list_t clients = {0};
  struct epoll_event ev = {0};
  event_list_t revents = {0};
  clients.fds = NULL;
  revents.evs = NULL;
  fragments_list_t fragments;

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

  // drop newline
  line[strcspn(line, "\n")] = '\0';
  out_file = fopen(line, "w");
  if (NULL == out_file) // open output file and truncate old contents
  {
    printf("Could not open output file");
    err(EXIT_FAILURE, "fopen");
  }

  if (0 != get_fragments(&fragments, in_file)) {
    err(EXIT_FAILURE, "get_fragments");
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

  revents.evs =
      realloc(revents.evs, sizeof(struct epoll_event) * ++revents.size);

  // set non blocking socket
  set_non_blocking_io(sfd);

  // wait for connections
  while ((ready = epoll_wait(epoll_fd, revents.evs, revents.size, -1)) != -1) {
    for (int i = 0; i < ready; i++) {
      if (revents.evs[i].data.fd == sfd) {
        // accept connections on server socket
        cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
        if (cfd == -1)
          err(EXIT_FAILURE, "accept");

        // add to client list
        clients.fds =
            realloc(clients.fds, sizeof(clients.fds) * ++clients.size);
        clients.fds[clients.size - 1] = cfd;

        // initialize socket pollfd
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.fd = cfd;
        ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev);
        if (-1 == ret) {
          err(EXIT_FAILURE, "epoll_ctl");
        }
        revents.evs =
            realloc(revents.evs, sizeof(struct epoll_event) * ++revents.size);

        set_non_blocking_io(cfd);
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

int get_fragments(fragments_list_t *fragments, FILE *in_file) {
  char *line = NULL;
  size_t line_len = 0;
  fragments->list = NULL;

  while (-1 != getline(&line, &line_len, in_file)) {
    fragments->list =
        realloc(fragments->list, sizeof(FILE *) * (++fragments->size));

    if (NULL == fragments->list) {
      printf("Could not allocate fragments array\n");
      return EAGAIN;
    }

    line[strcspn(line, "\n")] = '\0';
    fragments->list[fragments->size - 1] = fopen(line, "r");

    if (NULL == fragments->list[fragments->size - 1]) {
      printf("Could not open fragment file %zd\n", fragments->size);
      return EBADF;
    }
  }

  return 0;
}
