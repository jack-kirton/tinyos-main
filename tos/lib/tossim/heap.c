// $Id: heap.c,v 1.6 2010-06-29 22:07:51 scipio Exp $

/*
 * Copyright (c) 2000-2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the University of California nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE     
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA, 
 * 94704.  Attention:  Intel License Inquiry.
 */

/* Authors:             Philip Levis
 *
 */

/*
 *   FILE: heap.h
 * AUTHOR: Philip Levis <pal@cs.berkeley.edu>
 *   DESC: Simple array-based priority heap for discrete event simulation.
 */

#include <heap.h>
#include <string.h> // For memcpy(3)

static const int STARTING_SIZE = 511;

#define HEAP_NODE(heap, index) (heap->data[index])

static void down_heap(heap_t* heap, int findex);
static void up_heap(heap_t* heap, int findex);
static void swap(heap_node_t* __restrict a, heap_node_t* __restrict b);

void init_heap(heap_t* heap) {
  heap->size = 0;
  heap->private_size = STARTING_SIZE;
  heap->data = (heap_node_t*)malloc(sizeof(heap_node_t) * heap->private_size);
}

void free_heap(heap_t* heap) {
  if (heap != NULL) {
    if (heap->data != NULL) {
      free(heap->data);
      heap->data = (heap_node_t*)NULL;
    }
    heap->size = 0;
    heap->private_size = 0;
  }
}

int heap_size(const heap_t* heap) {
  return heap->size;
}

static inline int is_empty(const heap_t* heap) {
  return heap->size == 0;
}

int heap_is_empty(const heap_t* heap) {
  return is_empty(heap);
}

long long int heap_get_min_key(const heap_t* heap) {
  if (is_empty(heap)) {
    return -1;
  }
  else {
    return HEAP_NODE(heap, 0).key;
  }
}

void* heap_peek_min_data(heap_t* heap) {
  if (is_empty(heap)) {
    return NULL;
  }
  else {
    return HEAP_NODE(heap, 0).data;
  }
}

heap_node_t heap_pop_min(heap_t* heap) {
  const int last_index = heap->size - 1;
  const heap_node_t node = HEAP_NODE(heap, 0);

  HEAP_NODE(heap, 0) = HEAP_NODE(heap, last_index);

  heap->size--;

  down_heap(heap, 0);

  return node;
}

void expand_heap(heap_t* heap) {
  heap->private_size = (heap->private_size * 2) + 1;
  heap->data = (heap_node_t*)realloc(heap->data, sizeof(heap_node_t) * heap->private_size);

  //dbg(DBG_SIM, "Resized heap from %i to %i.\n", heap->private_size, new_size);
}

void heap_insert(heap_t* heap, void* data, long long int key) {
  int findex = heap->size;
  if (findex == heap->private_size) {
    expand_heap(heap);
  }
  
  findex = heap->size;
  HEAP_NODE(heap, findex).key = key;
  HEAP_NODE(heap, findex).data = data;
  up_heap(heap, findex);

  heap->size++;
}

static void swap(heap_node_t* __restrict a, heap_node_t* __restrict b) {
  const heap_node_t c = *a;
  *a = *b;
  *b = c;
}

static void down_heap(heap_t* heap, int findex) {
  int right_index = (findex + 1) * 2;
  int left_index = (findex * 2) + 1;

  if (right_index < heap->size) { // Two children
    long long int left_key = HEAP_NODE(heap, left_index).key;
    long long int right_key = HEAP_NODE(heap, right_index).key;
    int min_key_index = (left_key < right_key)? left_index : right_index;

    if (HEAP_NODE(heap, min_key_index).key < HEAP_NODE(heap, findex).key) {
      swap(&(HEAP_NODE(heap, findex)), &(HEAP_NODE(heap, min_key_index)));
      down_heap(heap, min_key_index);
    }
  }
  else if (left_index >= heap->size) { // No children
    return;
  }
  else { // Only left child
    long long int left_key = HEAP_NODE(heap, left_index).key;
    if (left_key < HEAP_NODE(heap, findex).key) {
      swap(&(HEAP_NODE(heap, findex)), &(HEAP_NODE(heap, left_index)));
      return;
    }
  }
}

static void up_heap(heap_t* heap, int findex) {
  int parent_index;
  if (findex == 0) {
    return;
  }

  parent_index = (findex - 1) / 2;

  if (HEAP_NODE(heap, parent_index).key > HEAP_NODE(heap, findex).key) {
    swap(&(HEAP_NODE(heap, findex)), &(HEAP_NODE(heap, parent_index)));
    up_heap(heap, parent_index);
  }
}

