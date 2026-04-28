#include "common.h"
#include "minheap.h"
#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define EXPECTED_ARGS 3

enum {
  PROGRAM_NAME,
  HOST_NAME,
  SERV_NAME,
};

int init_client(char *ip, char *port);

/* Client component for CSE 4202 Lab 3
 *
 * This purpose of this client is to sort data received from an orchestrating
 * server. The client will sort said data and promptly provide it back to the
 * server.
 *
 */

int main(int argc, char *argv[]) {
  int fd;
  FILE *r_fp;
  line_vec_t lines = {0};

  ssize_t last_pos = -1;
  lines.vec = NULL;

  if (argc != EXPECTED_ARGS) {
    return usage();
  }

  fd = init_client(argv[HOST_NAME], argv[SERV_NAME]);
  if (-1 == fd) {
    err(EXIT_FAILURE, "init_client");
  }

  // create FILE stream
  r_fp = fdopen(fd, "r");
  setbuf(r_fp, NULL);
  if (NULL == r_fp) {
    err(EXIT_FAILURE, "fdopen");
  }

  // client will receive INVALID_FILE_POS on end of transmission.
  fscanf(r_fp, "%zd", &last_pos);
  while (last_pos != INVALID_FILE_POS) {
    line_t new_line = {.file_pos = last_pos, .line = NULL, .len = 0};

    size_t cap = 0;
    if (-1 == (new_line.len = getline(&new_line.line, &cap, r_fp))) {
      printf("Could not get fragment line from socket\n");
      err(EXIT_FAILURE, "getline");
    }

    if (NULL == insert_line(&lines, new_line)) {
      err(EXIT_FAILURE, "add_line");
    };

    fscanf(r_fp, "%zd", &last_pos);
  }
  printf("RECEIVED\n");

  line_t curr_line;
  while ((curr_line = get_min(&lines)).file_pos != INVALID_FILE_POS) {
    char line_pos[INT_STR_SIZE];
    ssize_t len = snprintf(line_pos, INT_STR_SIZE, "%zd", curr_line.file_pos);
    if (0 > len) {
      err(EXIT_FAILURE, "snprintf");
    }

    safe_send(fd, line_pos, len);
    safe_send(fd, curr_line.line, curr_line.len);
    printf("Sent line: file_pos=%s, line=%s\n", line_pos, curr_line.line);
    free(curr_line.line);
  }
  // delimit end of message
  safe_send(fd, EOF_STR, strlen(EOF_STR));
  printf("SENT\n");

  /* Clean up */
  // free lines array
  free(lines.vec);

  // close socket
  fclose(r_fp);

  return EXIT_SUCCESS;
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
  return sfd;
}

int usage(void) {
  printf("./client <server_ip> <server_port>\n"
         "server_ip: the numeric internet address that reaches the server.\n"
         "server_port: the numeric port that the server is running on \n");
  return EXIT_FAILURE;
}
