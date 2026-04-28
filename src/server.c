#include "common.h"
#include "minheap.h"
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

enum {
  ENOFRAGS = 1,
  EGETLINE = 1,
};

typedef struct event_pair {
  int fd;
  FILE *frag_file;
} event_pair_t;

typedef struct event_list {
  ssize_t size;
  struct epoll_event *vec;
} event_list_t;

int init_socket(void);
int count_lines(FILE *in);
int grow_events(event_list_t *events);
FILE *get_file(FILE *in_file, char *mode);
void skip_lines(FILE *in_file, int n);

int main(int argc, char *argv[]) {
  ssize_t registered_epollfds = 0;
  ssize_t registered_clients = 0;
  line_vec_t sorted_lines = {.vec = NULL, .size = 0, .capacity = 0};

  if (argc != 2)
    return usage();

  FILE *in_file;
  if (NULL == (in_file = fopen(argv[1], "r"))) {
    printf("Could not open input file\n");
    usage();
    err(EXIT_FAILURE, "fopen");
  }

  int frag_count = count_lines(in_file) - 1;

  rewind(in_file);        // prepare for getting fragment files later
  skip_lines(in_file, 1); // skip output file line

  // initialize epoll fd
  int ret, epoll_fd;
  epoll_fd = epoll_create1(0);
  if (-1 == epoll_fd) {
    err(EXIT_FAILURE, "epoll_create1");
  }

  // 2. Connect with all clients
  int sfd = init_socket();
  // set non blocking socket
  set_non_blocking_io(sfd);

  // initialize socket epoll_event
  struct epoll_event ev = {0};
  ev.events = EPOLLIN;
  ev.data.ptr = &(event_pair_t){.fd = sfd, .frag_file = NULL};
  if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sfd, &ev)) {
    err(EXIT_FAILURE, "epoll_ctl");
  }
  registered_epollfds++;

  // for receiving event notifications
  event_list_t revents = {.vec = NULL, .size = 0};
  grow_events(&revents);

  // wait for connections
  int ready = 0;
  while (-1 != (ready = epoll_wait(epoll_fd, revents.vec, revents.size, -1))) {
    for (int i = 0; i < ready; i++) {
      if (((event_pair_t *)(revents.vec[i].data.ptr))->fd == sfd) {
        // accept connections on server socket
        struct sockaddr_in peer_addr = {0};
        socklen_t peer_addr_size = 0;
        int cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_size);
        if (cfd == -1)
          err(EXIT_FAILURE, "accept");

        /* set_non_blocking_io(cfd); */

        printf("Registered client %zd\n", ++registered_clients);

        // initialize client pollfd
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
        ev.data.ptr = &(event_pair_t){.fd = cfd, .frag_file = NULL};
        ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &ev);
        registered_epollfds++;
        if (-1 == ret) {
          err(EXIT_FAILURE, "epoll_ctl");
        }

        grow_events(&revents);

        if (registered_clients == frag_count) {
          ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sfd, NULL);
          registered_epollfds--;
          if (-1 == ret) {
            err(EXIT_FAILURE, "epoll_ctl");
          }
        }
      } else {
        // work on client
        event_pair_t *client_pair = (event_pair_t *)ev.data.ptr;
        if (revents.vec[i].events & EPOLLOUT) {
          // get a fragment_file
          if (NULL == client_pair->frag_file) {
            client_pair->frag_file = get_file(in_file, "r");
          }

          char *line = NULL;
          size_t cap = 0;
          ssize_t len = 0;
          // read line from file and send
          while (-1 != (len = getline(&line, &cap, client_pair->frag_file))) {
            safe_send(client_pair->fd, line, len);
          }

          // delimit end of message
          safe_send(client_pair->fd, EOF_STR, strlen(EOF_STR));

          // clean up
          free(line);
          fclose(client_pair->frag_file);

          // remove EPOLLOUT from watched events
          ev.events = EPOLLIN | EPOLLRDHUP;
          ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_pair->fd, &ev);
          if (-1 == ret) {
            err(EXIT_FAILURE, "epoll_ctl");
          }
        } else if (revents.vec[i].events & EPOLLIN) {
          // duplicate file descriptor to use streams and be able to close them
          // later.
          int dup_client = dup(client_pair->fd);
          FILE *reader_fp = fdopen(dup_client, "r");

          ssize_t last_pos = -1;
          // client will receive INVALID_FILE_POS on end of transmission.
          fscanf(reader_fp, "%zd", &last_pos);
          while (last_pos != INVALID_FILE_POS) {
            line_t last_line = {.file_pos = last_pos, .line = NULL, .len = 0};

            size_t cap = 0;
            if (-1 ==
                (last_line.len = getline(&last_line.line, &cap, reader_fp))) {
              printf("Could not get fragment line from socket\n");
              err(EXIT_FAILURE, "getline");
            }

            if (NULL == insert_line(&sorted_lines, last_line)) {
              err(EXIT_FAILURE, "insert_line");
            };

            fscanf(reader_fp, "%zd", &last_pos);
          }

          // remove EPOLLIN from watched events
          ev.events = EPOLLRDHUP;
          ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_pair->fd, &ev);
          if (-1 == ret) {
            err(EXIT_FAILURE, "epoll_ctl");
          }

          /* Cleanup */
          fclose(reader_fp);
        } else if (revents.vec[i].events & EPOLLRDHUP) {
          ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_pair->fd, NULL);
          registered_epollfds--;
          if (-1 == ret) {
            err(EXIT_FAILURE, "epoll_ctl");
          }

          close(client_pair->fd);
        }
      }
    }
    if (registered_epollfds == 0)
      break;
  }

  rewind(in_file);
  FILE *out_file =
      get_file(in_file, "w"); // first line should be the output file
  // write to output file and clean up lines
  line_t curr_line = {0};
  while ((curr_line = get_min(&sorted_lines)).file_pos != INVALID_FILE_POS) {
    fprintf(out_file, "%zd%s", curr_line.file_pos, curr_line.line);
    free(curr_line.line);
  }

  /* Cleanup */
  fclose(in_file);
  fclose(out_file);
  free(sorted_lines.vec);
  free(revents.vec);
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

// Reads the next line in in_file, interprets it as a file path then creates and
// returns the associated FILE *
FILE *get_file(FILE *in_file, char *mode) {
  char *line = NULL;
  size_t line_len = 0;
  size_t cap = 0;

  line_len = getline(&line, &cap, in_file);
  if ((size_t)(-1) == line_len) {
    printf("Failed to get new fragment\n");
    return NULL;
  }

  // drop newline
  line[line_len - 1] = '\0';
  FILE *fd = fopen(line, mode);
  free(line);

  return fd;
}

int count_lines(FILE *in) {
  char c;
  int count = 0;
  while ((c = fgetc(in)) != (char)EOF) {
    if (c == '\n')
      ++count;
  }
  return count;
}

int usage(void) {
  printf("./server <input_file> \n"
         "input_file: a file containing fragment file names\n");
  return EXIT_FAILURE;
}

int grow_events(event_list_t *events) {
  if (NULL == (events->vec = realloc(events->vec, sizeof(struct epoll_event) *
                                                      ++events->size)))
    return -1;

  return 0;
}

void skip_lines(FILE *in_file, int n) {
  char *buf = NULL;
  size_t len = 0;

  for (int i = 0; i < n; i++) {
    if (-1 == getline(&buf, &len, in_file)) {
      break;
    }
  }

  free(buf);
}
