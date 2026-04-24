#ifndef HEADER_H_
#define HEADER_H_

#include <sys/types.h>

typedef struct fragment_line {
  ssize_t file_pos;
  ssize_t len;
  char *line;
} fragment_line_t;

typedef struct lines_vec {
  ssize_t len;
  fragment_line_t *vec;
} lines_vec_t;

int usage(void);
void set_non_blocking_io(int fd);
void print_file_pos(lines_vec_t *lines);
int safe_send(int fd, char *buf, ssize_t len);

#endif // HEADER_H_
