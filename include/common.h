#ifndef HEADER_H_
#define HEADER_H_

#include <sys/types.h>

#define GROWTH_FACTOR 1.5

typedef struct fragment_line {
  ssize_t file_pos;
  ssize_t len;
  char *line;
} line_t;

typedef struct lines_vec {
  ssize_t size;
  ssize_t capacity;
  line_t *vec;
} line_vec_t;

int usage(void);
void set_non_blocking_io(int fd);
void print_file_pos(line_vec_t *lines);
int safe_send(int fd, char *buf, ssize_t len);
line_t *add_line(line_vec_t *lines, line_t new_line);

#endif // HEADER_H_
