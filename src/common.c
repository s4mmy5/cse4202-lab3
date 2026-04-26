#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

void print_file_pos(lines_vec_t *lines) {
  printf("Lines count=%zd\n", lines->size);
  for (ssize_t i = 0; i < lines->size; ++i) {
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

fragment_line_t *add_line(lines_vec_t *lines, fragment_line_t new_line) {
  if (lines->size == lines->capacity) {
    size_t new_capacity = GROWTH_FACTOR * lines->capacity;
    lines->vec = realloc(lines->vec, new_capacity);
    if (NULL == lines->vec) {
      printf("Failed to grow lines_vec_t\n");
      return NULL;
    }
    lines->capacity = new_capacity;
  }
  lines->vec[lines->size++] = new_line;
  return &lines->vec[lines->size - 1];
}
