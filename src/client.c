#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct fragment_line {
  size_t file_pos;
  size_t line_len;
  char *line;
} fragment_line_t;

typedef struct lines_vec {
  size_t len;
  fragment_line_t *vec;
} lines_vec_t;

enum {
  PROGRAM_NAME,
  HOST_NAME,
  SERV_NAME,
};

int init_client(char *ip, char *port);
int usage(void);
void set_non_blocking_io(int fd);

int main(int argc, char *argv[]) {
  int sfd;
  FILE *s_fp;
  lines_vec_t lines = {0};

  size_t last_pos = -1;
  lines.vec = NULL;

  if (argc != 3) {
    return usage();
  }

  sfd = init_client(argv[HOST_NAME], argv[SERV_NAME]);

  // create FILE stream
  s_fp = fdopen(sfd, "w");
  if (NULL == s_fp) {
    err(EXIT_FAILURE, "fdopen");
  }

  // client will receive EOF on end of transmission.
  fscanf(s_fp, "%zu", &last_pos);
  while (last_pos != EOF) {
    // increase lines capacity
    lines.vec = realloc(lines.vec, sizeof(fragment_line_t) * ++lines.len);
    fragment_line_t *last_line = &lines.vec[lines.len - 1];

    // init line
    last_line->file_pos = last_pos;
    last_line->line = NULL;
    last_line->line_len = 0;
    if (-1 == getline(&last_line->line, &last_line->line_len, s_fp)) {
      printf("Could not get fragment line from socket\n");
      err(EXIT_FAILURE, "getline");
    }

    fscanf(s_fp, "%zu", &last_pos);
  }

  // sort lines
  insertion_sort(&lines);
}

void set_non_blocking_io(int fd) {
  int flags;

  flags = fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
}

int usage(void) {
  printf("./server <server_ip> <server_port>\n"
         "server_ip: the numeric internet address that reaches the server.\n"
         "server_port: the numeric port that the server is running on \n");
  return EXIT_FAILURE;
}

int init_client(char *ip, char *port) {
  int sfd;
  struct addrinfo *info = NULL;
  struct addrinfo hints = {0};

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

  if (0 != getaddrinfo(ip, port, &hints, &info)) {
    err(EXIT_FAILURE, "getaddrinfo");
  }

  sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (-1 == sfd) {
    printf("Could not parse IP and port number\n");
    usage();
    err(EXIT_FAILURE, "socket");
  }

  if (-1 == connect(sfd, info->ai_addr, info->ai_addrlen))
    err(EXIT_FAILURE, "connect");

  // prevent blocking on input output
  set_non_blocking_io(sfd);
  return sfd;
}

int insertion_sort(lines_vec_t *lines) {
  for (int i = 1; i < lines->len; ++i) {
    fragment_line_t anker = lines->vec[i];
    int j = i - 1;
    while (j > 0 && lines->vec[j].file_pos > anker.file_pos) {
      // TODO finish implementing insertion sort
      j--;
    }
  }
}
