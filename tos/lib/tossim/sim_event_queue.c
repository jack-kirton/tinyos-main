// $Id: sim_event_queue.c,v 1.6 2010-06-29 22:07:51 scipio Exp $

/*
* Copyright (c) 2005 Stanford University. All rights reserved.
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
 * - Neither the name of the copyright holders nor the names of
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
 */
/**
 * The simple TOSSIM wrapper around the underlying heap.
 *
 * @author Phil Levis
 * @date   November 22 2005
 */


#include <heap.h>
#include <sim_event_queue.h>

static heap_t eventHeap;

void sim_queue_init(void) __attribute__ ((C, spontaneous)) {
  init_heap(&eventHeap);
}

void sim_queue_free(void) __attribute__ ((C, spontaneous)) {
  free_heap(&eventHeap);
}

void sim_queue_insert(sim_event_t* event) __attribute__ ((C, spontaneous)) {
  //dbg("Queue", "Inserting 0x%p\n", event);
  heap_insert(&eventHeap, event, event->time);
}

sim_event_t* sim_queue_pop(void) __attribute__ ((C, spontaneous)) {
  return (sim_event_t*)heap_pop_min(&eventHeap).data;
}

bool sim_queue_is_empty(void) __attribute__ ((C, spontaneous)) {
  return heap_is_empty(&eventHeap);
}

long long int sim_queue_peek_time(void) __attribute__ ((C, spontaneous)) {
  // If the heap is empty this returns -1
  return heap_get_min_key(&eventHeap);
}


void sim_queue_cleanup_none(sim_event_t* event) __attribute__ ((C, spontaneous)) {
  //dbg("Queue", "cleanup_none: 0x%p\n", event);
  // Do nothing. Useful for statically allocated events.
}

void sim_queue_cleanup_event(sim_event_t* event) __attribute__ ((C, spontaneous)) {
  //dbg("Queue", "cleanup_event: 0x%p\n", event);
  sim_queue_free_event(event);
}

void sim_queue_cleanup_data(sim_event_t* event) __attribute__ ((C, spontaneous)) {
  //dbg("Queue", "cleanup_data: 0x%p\n", event);
  free(event->data);
  event->data = NULL;
}
    
void sim_queue_cleanup_total(sim_event_t* event) __attribute__ ((C, spontaneous)) {
  //dbg("Queue", "cleanup_total: 0x%p\n", event);
  free(event->data);
  event->data = NULL;
  sim_queue_free_event(event);
}

sim_event_t* sim_queue_allocate_raw_event(void) {
  return (sim_event_t*)malloc(sizeof(sim_event_t));
}

sim_event_t* sim_queue_allocate_event(void) {
  sim_event_t* evt = (sim_event_t*)calloc(1, sizeof(sim_event_t));
  evt->mote = sim_node();
  return evt;
}

void sim_queue_free_event(sim_event_t* event) {
  free(event);
}
