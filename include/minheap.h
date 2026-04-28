#ifndef MINHEAP_H
#define MINHEAP_H
#include "common.h"

#define INVALID_FILE_POS -1

static inline size_t get_left_child(ssize_t idx) { return 2 * idx + 1; }
static inline size_t get_right_child(ssize_t idx) { return 2 * idx + 2; }
static inline size_t get_parent(ssize_t idx) { return (idx - 1) / 2; }

void bubbleDown(line_vec_t *lines, size_t start_idx);
void bubbleUp(line_vec_t *lines, size_t start_idx);
void swap(line_vec_t *lines, size_t i, size_t j);
line_t *insert_line(line_vec_t *lines, line_t new_line);
line_t get_min(line_vec_t *lines);
ssize_t get_priority(line_vec_t *lines, ssize_t idx);

#endif /* MINHEAP_H */
