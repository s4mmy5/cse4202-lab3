#include "minheap.h"
#include <stdio.h>

// Modified version of my CSE247 minheap implementation.
// https://github.com/wustl-cse247-sp25/m7-lab-JobaHD/blob/master/src/labcode/heaps/MinHeap.java#L328

void swap(line_vec_t *lines, size_t i, size_t j) {
  line_t tmp = lines->vec[j];

  lines->vec[j] = lines->vec[i];
  lines->vec[i] = tmp;
}

// does insertion and enforces ordering
line_t *insert_line(line_vec_t *lines, line_t new_line) {
  line_t *new_line_ptr = add_line(lines, new_line);
  if (NULL == new_line_ptr) {
    printf("Failed to insert line\n");
    return NULL;
  }

  bubbleUp(lines, lines->size - 1); // bubble up starting at the end.
  return new_line_ptr;
}

line_t get_min(line_vec_t *lines) {
  if (lines->size == 0) {
    return (line_t){.file_pos = -1};
  }

  line_t min = lines->vec[0];
  if (lines->size == 1) {
    lines->size--; // remove last element
    return min;
  }

  swap(lines, 0, lines->size - 1);
  lines->size--; // remove last element
  bubbleDown(lines, 0);

  return min;
}

void bubbleUp(line_vec_t *lines, size_t start_idx) {
  size_t node_idx = start_idx;
  while (node_idx > 0) {
    size_t parent_idx = get_parent(node_idx);
    if (get_priority(lines, node_idx) >= get_priority(lines, parent_idx)) {
      return;
    } else {
      swap(lines, node_idx, parent_idx);
      node_idx = parent_idx;
    }
  }
}

void bubbleDown(line_vec_t *lines, size_t start_idx) {
  ssize_t node_idx = start_idx;
  ssize_t left_child_idx = get_left_child(node_idx);
  ssize_t right_child_idx = get_right_child(node_idx);
  ssize_t node_priority = get_priority(lines, node_idx);

  while (left_child_idx < lines->size) {
    ssize_t min_priority = node_priority;
    ssize_t min_idx = -1;

    if (get_priority(lines, left_child_idx) < min_priority) {
      min_priority = get_priority(lines, left_child_idx);
      min_idx = left_child_idx;
    }

    if (right_child_idx < lines->size &&
        get_priority(lines, right_child_idx) < min_priority) {
      min_priority = get_priority(lines, right_child_idx);
      min_idx = right_child_idx;
    }

    if (min_priority == node_priority) {
      return;
    } else {
      swap(lines, node_idx, min_idx);
      node_idx = min_idx;
      left_child_idx = get_left_child(node_idx);
      right_child_idx = get_right_child(node_idx);
    }
  }
}

ssize_t get_priority(line_vec_t *lines, ssize_t idx) {
  if (idx >= lines->size) {
    return INVALID_FILE_POS;
  } else {
    return lines->vec[idx].file_pos;
  }
}
