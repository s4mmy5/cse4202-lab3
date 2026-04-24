#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>

void print_file_pos(lines_vec_t *lines) {
  printf("Lines count=%zd\n", lines->len);
  for (ssize_t i = 0; i < lines->len; ++i) {
    printf("Idx=%02zd, File_pos=%02zd\n", i, lines->vec[i].file_pos);
    printf("Contents=%s\n", lines->vec[i].line);
  }
}

void set_non_blocking_io(int fd) {
  int flags;

  flags = fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
}

int safe_send(int fd, char *buf, ssize_t len) {
  ssize_t left = len;
  while (left > 0) {
    ssize_t written = send(fd, buf, left, 0);
    if (written < 0) {
      return -EBADF;
    }
    buf += written;
    left -= written;
  }
  return 0;
}
