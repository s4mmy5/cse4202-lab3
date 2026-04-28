#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

void print_file_pos(line_vec_t *lines) {
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

void insertion_sort(line_vec_t *lines) {
  for (ssize_t i = 1; i < lines->size; ++i) {
    line_t anker = lines->vec[i];
    ssize_t j = i - 1;
    while (j >= 0 && lines->vec[j].file_pos > anker.file_pos) {
      lines->vec[j + 1] = lines->vec[j];
      --j;
    }
    lines->vec[j + 1] = anker;
  }
}

void print_arr(line_vec_t *lines) {
  for (int i = 0; i < lines->size; ++i) {
    line_t curr_line = lines->vec[i];
    printf("i=%d, file_pos=%zd, line=%s\n", i, curr_line.file_pos,
           curr_line.line);
  }
}
