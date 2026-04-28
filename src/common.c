#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

// utility function for inspecting the state of line_vec_t
void print_file_pos(line_vec_t *lines) {
  printf("Lines count=%zd\n", lines->size);
  for (ssize_t i = 0; i < lines->size; ++i) {
    printf("Idx=%02zd, File_pos=%02zd\n", i, lines->vec[i].file_pos);
    printf("Contents=%s\n", lines->vec[i].line);
  }
}

// // sets the O_NONBLOCK flag on fd
// sets the O_NONBLOCK flag on fd
void set_non_blocking_io(int fd) {
  int flags;

  flags = fcntl(fd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
}

// send() but it keeps trying until all data is sent.
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

// grows the line_vec_t array. Also keeps a capacity variable for faster
// amortized memory allocations.
line_t *add_line(line_vec_t *lines, line_t new_line) {
  if (lines->capacity == 0) {
    lines->vec = malloc(sizeof(line_t) * MIN_CAPACITY);
    if (NULL == lines->vec) {
      printf("Failed to create lines_vec_t\n");
      return NULL;
    }
    lines->capacity = MIN_CAPACITY;
  } else if (lines->size == lines->capacity) {
    size_t new_capacity = GROWTH_FACTOR * lines->capacity;
    lines->vec = realloc(lines->vec, sizeof(line_t) * new_capacity);
    if (NULL == lines->vec) {
      printf("Failed to grow lines_vec_t\n");
      return NULL;
    }
    lines->capacity = new_capacity;
  }

  lines->vec[lines->size++] = new_line;
  return &lines->vec[lines->size - 1];
}
