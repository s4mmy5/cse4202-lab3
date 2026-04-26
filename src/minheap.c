#include "minheap.h"
#include "common.h"

// FIXME: finish implementing minheap for using line_vec_t
// https://github.com/wustl-cse247-sp25/m7-lab-JobaHD/blob/master/src/labcode/heaps/MinHeap.java#L328

size_t get_left_child(size_t idx) { return 2 * idx + 1; }

size_t get_right_child(size_t idx) { return 2 * idx + 2; }

void swap(line_vec_t *lines, size_t i, size_t j) {
  line_t tmp = lines->vec[j];

  lines->vec[j] = lines->vec[i];
  lines->vec[i] = tmp;
}

// does insertion and enforces ordering
line_t insert_line(line_vec_t *lines, line_t new_line) {
  line_t *new_line_ptr = add_line(lines, new_line);
  bubbleUp(lines, lines->size - 1); // bubble up starting at the end.
}

line_t get_min(line_vec_t *lines) {
  if (lines->size == 0) {
    return (line_t){0};
  }

  line_t min = lines->vec[lines->size];
  if (lines->size == 1) {
    lines->size--;
    return min;
  }

  swap(lines, 0, lines->size - 1);
  lines->size--;
  bubbleDown(lines, 0);

  return min;
}

void bubbleUp(line_vec_t *lines, size_t start_idx) {
  size_t node_idx = start_idx;
  while (node_idx > 0) {
    size_t parent_idx = get_parent_idx(node_idx);
    if (lines->vec[node_idx].file_pos >= lines->vec[parent_idx].file_pos) {
      return;
    } else {
      swap(lines, node_idx, parent_idx);
      node_idx = parent_idx;
    }
  }
}
