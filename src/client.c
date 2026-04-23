#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

// TODO Implement better sorting than insertion sort

typedef struct fragment_line {
  ssize_t file_pos;
  size_t line_len;
  char *line;
} fragment_line_t;

typedef struct lines_vec {
  ssize_t len;
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
void insertion_sort(lines_vec_t *lines);
void print_file_pos(lines_vec_t *lines);

int main(int argc, char *argv[]) {
  int sfd;
  FILE *s_fp;
  lines_vec_t lines = {0};

  ssize_t last_pos = -1;
  lines.vec = NULL;

  if (argc != 3) {
    return usage();
  }

  sfd = init_client(argv[HOST_NAME], argv[SERV_NAME]);
  if (-1 == sfd) {
    err(EXIT_FAILURE, "init_client");
  }

  // create FILE stream
  s_fp = fdopen(sfd, "r+");
  if (NULL == s_fp) {
    err(EXIT_FAILURE, "fdopen");
  }

  // client will receive EOF on end of transmission.
  fscanf(s_fp, "%zd", &last_pos);
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

    fscanf(s_fp, "%zd", &last_pos);
  }

  print_file_pos(&lines);
  // sort lines
  insertion_sort(&lines);
  print_file_pos(&lines);

  /* Cleanup */
  // free lines
  for (int i = 0; i < lines.len; ++i) {
    free(lines.vec[i].line);
  }
  // free lines array
  free(lines.vec);

  // close socket
  fclose(s_fp);

  return EXIT_SUCCESS;
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
    printf("Could not parse IP and port number\n");
    usage();
    return -1;
  }

  sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (-1 == sfd) {
    printf("Could not parse IP and port number\n");
    usage();
    return -1;
  }

  if (-1 == connect(sfd, info->ai_addr, info->ai_addrlen)) {
    printf("Could not parse IP and port number\n");
    usage();
    return -1;
  }

  // free addr_info struct
  freeaddrinfo(info);
  // prevent blocking on input output
  /* set_non_blocking_io(sfd); */
  return sfd;
}

void insertion_sort(lines_vec_t *lines) {
  for (ssize_t i = 1; i < lines->len; ++i) {
    fragment_line_t anker = lines->vec[i];
    ssize_t j = i - 1;
    while (j >= 0 && lines->vec[j].file_pos > anker.file_pos) {
      lines->vec[j + 1] = lines->vec[j];
      --j;
    }
    lines->vec[j + 1] = anker;
  }
}

void print_file_pos(lines_vec_t *lines) {
  printf("Lines count=%zd\n", lines->len);
  for (ssize_t i = 0; i < lines->len; ++i) {
    printf("Idx=%02zd, File_pos=%02zd\n", i, lines->vec[i].file_pos);
    printf("Contents=%s\n", lines->vec[i].line);
  }
}
