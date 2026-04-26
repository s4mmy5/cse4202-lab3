#include "common.h"
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

typedef struct epoll_list {
  ssize_t size;
  struct epoll_event *evs;
} event_list_t;

typedef struct client {
  int sorting;
  int remote_fd;
  FILE *fragments_file;
} client_t;

typedef struct clients_list {
  ssize_t size;
  client_t *vec;
} clients_list_t;

int init_socket(void);
int init_clients_list(clients_list_t *clients, FILE *in_file);

/* For now just use a burst model where all data from a fragment is sent at
 * once or not. FIXME: Next implement version where reads and writes are
 * interleaved. */

int main(int argc, char *argv[]) {
  // 1. Open all required files
  FILE *in_file, *out_file;
  int sfd, cfd, epoll_fd, ret, ready;
  char *line = NULL;
  size_t line_len = 0;
  size_t lines_sent = 0;
  ssize_t registered_clients = 0;
  struct sockaddr_in peer_addr = {0};
  socklen_t peer_addr_size = 0;
  clients_list_t clients = {0};
  struct epoll_event ev = {0};
  event_list_t revents = {0};
  lines_vec_t sorted_lines = {0};
  sorted_lines.vec = NULL;
  clients.vec = NULL;
  revents.evs = NULL;
  revents.size = 0;

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
  free(line);

  if (NULL == out_file) // open output file and truncate old contents
  {
    printf("Could not open output file");
    err(EXIT_FAILURE, "fopen");
  }

  if (0 != init_clients_list(&clients, in_file)) {
    err(EXIT_FAILURE, "init_clients_list");
  }

  // initialize epoll fd
  epoll_fd = epoll_create1(0);
  if (-1 == epoll_fd) {
    err(EXIT_FAILURE, "epoll_create1");
  }

  // 2. Connect with all clients
  sfd = init_socket();
  // set non blocking socket
  set_non_blocking_io(sfd);

  // initialize socket epoll_event
  ev.events = EPOLLIN;
  ev.data.fd = sfd;
  if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev)) {
    err(EXIT_FAILURE, "epoll_ctl");
  }

  revents.evs =
      realloc(revents.evs, sizeof(struct epoll_event) * ++revents.size);
  // wait for connections
  while (-1 != (ready = epoll_wait(epoll_fd, revents.evs, revents.size, -1))) {
    for (int i = 0; i < ready; i++) {
      if (revents.evs[i].data.fd == sfd) {
        // accept connections on server socket
        cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
        if (cfd == -1)
          err(EXIT_FAILURE, "accept");

        set_non_blocking_io(cfd);

        // add FILE* to client list
        clients.vec[registered_clients++].remote_fd = cfd;
        printf("Registered client %zd\n", registered_clients);

        // initialize client pollfd
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
        ev.data.fd = cfd;
        ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev);
        if (-1 == ret) {
          err(EXIT_FAILURE, "epoll_ctl");
        }

        revents.evs =
            realloc(revents.evs, sizeof(struct epoll_event) * ++revents.size);

        // if we are done registering clients remove sfd from epoll
        if (registered_clients == clients.size) {
          ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sfd, NULL);
          if (-1 == ret) {
            err(EXIT_FAILURE, "epoll_ctl");
          }
        }
      } else {
        // find client_idx. FIXME change to binary search if possible
        int client_idx;
        for (client_idx = 0; client_idx < clients.size; ++client_idx) {
          if (clients.vec[client_idx].remote_fd ==
              revents.evs[client_idx].data.fd)
            break;
        }
        client_t *curr_client = &clients.vec[client_idx];

        if (revents.evs[i].events & EPOLLOUT) {
          // send client unsorted fragments
          char *line = NULL;
          size_t cap = 0;
          ssize_t len = 0;

          // read line from file and send
          while (-1 !=
                 (len = getline(&line, &cap, curr_client->fragments_file))) {
            safe_send(curr_client->remote_fd, line, len);
            lines_sent++;
          }

          // delimit end of message
          safe_send(curr_client->remote_fd, "-1\n", 2);

          // detect reading errors
          if (ferror(curr_client->fragments_file)) {
            printf("Error reading from fragment file %d",
                   curr_client->remote_fd);
            err(EXIT_FAILURE, "ferror");
          }

          // clean up
          free(line);
          fclose(curr_client->fragments_file);

          // remove EPOLLOUT from watched events
          ev.events = EPOLLIN | EPOLLRDHUP;
          ev.data.fd = cfd;
          ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, curr_client->remote_fd, &ev);
          if (-1 == ret) {
            err(EXIT_FAILURE, "epoll_ctl");
          }
        }
        if (revents.evs[i].events & EPOLLIN) {
          // do the merge step when clients send data back
          // open FILE * to use getline()
          FILE *reader_fp = fdopen(curr_client->remote_fd, "r");

          char *line = NULL;
          size_t cap = 0;
          ssize_t len = 0;

          while (-1 != (len = getline(&line, &cap, reader_fp))) {
            // TODO: add line and keep sorted property. Needs new data structure
          }
        }
        if (revents.evs[i].events & EPOLLRDHUP) {
        }
      }
    }
  }

  close(sfd);
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

int init_clients_list(clients_list_t *clients, FILE *in_file) {
  char *line = NULL;
  size_t line_len = 0;
  clients->vec = NULL;

  while (-1 != getline(&line, &line_len, in_file)) {
    clients->vec = realloc(clients->vec, sizeof(client_t) * (++clients->size));

    if (NULL == clients->vec) {
      printf("Could not allocate clients array\n");
      return -EAGAIN;
    }

    line[strcspn(line, "\n")] = '\0';
    clients->vec[clients->size - 1].fragments_file = fopen(line, "r");

    if (NULL == clients->vec[clients->size - 1].fragments_file) {
      printf("Could not open fragment file %zd\n", clients->size);
      return -EBADF;
    }
  }
  free(line);

  if (ferror(in_file)) {
    return -EBADF;
  }
  return 0;
}

int usage(void) {
  printf("./server <input_file> \n"
         "input_file: a file containing fragment file names\n");
  return EXIT_FAILURE;
}
