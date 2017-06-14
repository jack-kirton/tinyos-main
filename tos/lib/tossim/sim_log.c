// $Id: sim_log.c,v 1.7 2010-06-29 22:07:51 scipio Exp $

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
 * The TOSSIM logging system.
 *
 * @author Phil Levis
 * @date   November 9 2005
 */

#include <sim_log.h>
#include <stdio.h>
#include <stdarg.h>
#include <hash_table.h>
#include <string.h>

enum {
  DEFAULT_CHANNEL_SIZE = 4,
  DEFAULT_CALLBACKS_SIZE = 1,
};

typedef struct sim_log_callback {
  void (*handle)(void* data, const char* line, size_t line_length);
  void* data;
} sim_log_callback_t;

typedef struct sim_log_output {
  int num_files;
  FILE** files;

  int num_callbacks;
  sim_log_callback_t** callbacks;
} sim_log_output_t;

typedef struct sim_log_channel {
  const char* name;

  int num_outputs;
  int size_outputs;
  FILE** outputs;

  int num_callbacks;
  int size_callbacks;
  sim_log_callback_t** callbacks;
} sim_log_channel_t;

enum {
  SIM_LOG_OUTPUT_COUNT = uniqueCount("TOSSIM.debug")
};

static sim_log_output_t outputs[SIM_LOG_OUTPUT_COUNT];
static struct hash_table channelTable;

static bool write_performed = FALSE;

void sim_log_reset_flag(void) __attribute__ ((C, spontaneous))
{
  write_performed = FALSE;
}

bool sim_log_test_flag(void) __attribute__ ((C, spontaneous))
{
  return write_performed;
}


static unsigned int sim_log_hash(const void* key);
static int sim_log_eq(const void* key1, const void* key2);


// First we count how many outputs there are,
// then allocate a FILE** large enough and fill it in.
// This FILE** might be larger than needed, because
// the outputs of the channels might have redundancies.
// E.g., if two channels A and B are both to stdout, then
// you don't want a channel of "A,B" to be doubly printed
// to stdout. So when the channel's FILE*s are copied
// into the debug point output array, this checks
// for redundancies by checking file descriptors.
static void fillInOutput(int id, const char* name) {
  const char* termination = name;
  const char* namePos = name;
  int count_outputs = 0;
  int count_callbacks = 0;
  const size_t nameLen = strlen(name);
  char* newName = (char*)calloc(nameLen + 1, sizeof(char));

  // Count the outputs
  while (termination != NULL) {
    sim_log_channel_t* channel;
    
    termination = strrchr(namePos, ',');
    // If we've reached the end, just copy to the end
    if (termination == NULL) {
      strcpy(newName, namePos);
    }
    // Otherwise, memcpy over and null terminate
    else {
      memcpy(newName, namePos, (termination - namePos));
      newName[termination - namePos] = '\0';
    }
    
    channel = hash_table_search_data(&channelTable, newName);
    if (channel != NULL) {
      count_outputs += channel->num_outputs;
      count_callbacks += channel->num_callbacks;
    }

    namePos = termination + 1;
  }

  termination = name;
  namePos = name;
  
  // Allocate
  outputs[id].files = (FILE**)malloc(sizeof(FILE*) * count_outputs);
  outputs[id].num_files = 0;

  outputs[id].callbacks = (sim_log_callback_t**)malloc(sizeof(sim_log_callback_t*) * count_callbacks);
  outputs[id].num_callbacks = 0;

  // Fill it in
  while (termination != NULL) {
    sim_log_channel_t* channel;
    
    termination = strrchr(namePos, ',');
    // If we've reached the end, just copy to the end
    if (termination == NULL) {
      strcpy(newName, namePos);
    }
    // Otherwise, memcpy over and null terminate
    else {
      memcpy(newName, namePos, (termination - namePos));
      newName[termination - namePos] = 0;
    }
    
    channel = hash_table_search_data(&channelTable, newName);
    if (channel != NULL) {
      int i, j;
      for (i = 0; i < channel->num_outputs; i++) {
        bool duplicate = FALSE;
        const int outputCount = outputs[id].num_files;
        // Check if we already have this file descriptor in the output
        // set, and if so, ignore it.
        for (j = 0; j < outputCount; j++) {
          if (fileno(outputs[id].files[j]) == fileno(channel->outputs[i])) {
            duplicate = TRUE;
            j = outputCount;
          }
        }
        if (!duplicate) {
          outputs[id].files[outputCount] = channel->outputs[i];
          outputs[id].num_files++;
        }
      }

      for (i = 0; i < channel->num_callbacks; ++i) {
        outputs[id].callbacks[outputs[id].num_callbacks] = channel->callbacks[i];
        outputs[id].num_callbacks++;
      }
    }
    namePos = termination + 1;
  }
  free(newName);
}

void sim_log_init(void) {
  int i;

  hash_table_create(&channelTable, sim_log_hash, sim_log_eq);
  
  for (i = 0; i < SIM_LOG_OUTPUT_COUNT; i++) {
    outputs[i].num_files = 1;
    outputs[i].files = (FILE**)malloc(sizeof(FILE*) * 1);
    outputs[i].files[0] = stdout;

    outputs[i].callbacks = NULL;
    outputs[i].num_callbacks = 0;
  }

  write_performed = FALSE;
}

static void channel_table_entry_free(struct hash_entry* entry)
{
  free((void*)entry->key);
  free(entry->data);
}

void sim_log_free(void) {
  int i;

  write_performed = FALSE;

  for (i = 0; i < SIM_LOG_OUTPUT_COUNT; i++) {
    free(outputs[i].files);
    outputs[i].files = NULL;
    outputs[i].num_files = 0;

    free(outputs[i].callbacks);
    outputs[i].callbacks = NULL;
    outputs[i].num_callbacks = 0;
  }

  hash_table_destroy(&channelTable, &channel_table_entry_free);
}

void sim_log_add_channel(const char* name, FILE* file) {
  sim_log_channel_t* channel;
  channel = (sim_log_channel_t*)hash_table_search_data(&channelTable, name);

  // If there's no current entry, allocate one, initialize it,
  // and insert it.
  if (channel == NULL) {
    const char* newName = strdup(name);
    
    channel = (sim_log_channel_t*)malloc(sizeof(sim_log_channel_t));
    channel->name = newName;

    channel->num_outputs = 0;
    channel->size_outputs = DEFAULT_CHANNEL_SIZE;
    channel->outputs = (FILE**)calloc(channel->size_outputs, sizeof(FILE*));

    channel->num_callbacks = 0;
    channel->size_callbacks = DEFAULT_CALLBACKS_SIZE;
    channel->callbacks = (sim_log_callback_t**)calloc(channel->size_callbacks, sizeof(sim_log_callback_t*));

    hash_table_insert(&channelTable, newName, channel);
  }

  // If the channel output table is full, double the size of
  // channel->outputs.
  if (channel->num_outputs == channel->size_outputs) {
    channel->size_outputs *= 2;
    channel->outputs = (FILE**)realloc(channel->outputs, sizeof(FILE*) * channel->size_outputs);
  }

  channel->outputs[channel->num_outputs] = file;
  channel->num_outputs++;

  sim_log_commit_change();
}

bool sim_log_remove_channel(const char* output, FILE* file) {
  sim_log_channel_t* channel;
  int i;
  channel = (sim_log_channel_t*)hash_table_search_data(&channelTable, output);  

  if (channel == NULL) {
    return FALSE;
  }

  // Note: if a FILE* has duplicates, this removes all of them
  for (i = 0; i < channel->num_outputs; i++) {
    FILE* f = channel->outputs[i];
    if (file == f) {
      memcpy(&channel->outputs[i], &channel->outputs[i + 1], (channel->num_outputs) - (i + 1));
      channel->outputs[channel->num_outputs - 1] = NULL;
      channel->num_outputs--;
    }
  }
  
  return TRUE;
}

void sim_log_add_callback(const char* name, void (*handle)(void* data, const char* line, size_t line_length), void* data) {
  sim_log_channel_t* channel;
  sim_log_callback_t* callback;

  channel = (sim_log_channel_t*)hash_table_search_data(&channelTable, name);
  
  // If there's no current entry, allocate one, initialize it,
  // and insert it.
  if (channel == NULL) {
    const char* newName = strdup(name);
    
    channel = (sim_log_channel_t*)malloc(sizeof(sim_log_channel_t));
    channel->name = newName;

    channel->num_outputs = 0;
    channel->size_outputs = DEFAULT_CHANNEL_SIZE;
    channel->outputs = (FILE**)calloc(channel->size_outputs, sizeof(FILE*));

    channel->num_callbacks = 0;
    channel->size_callbacks = DEFAULT_CALLBACKS_SIZE;
    channel->callbacks = (sim_log_callback_t**)calloc(channel->size_callbacks, sizeof(sim_log_callback_t*));

    hash_table_insert(&channelTable, newName, channel);
  }

  // If the channel output table is full, double the size of
  // channel->outputs.
  if (channel->num_callbacks == channel->size_callbacks) {
    channel->size_callbacks *= 2;
    channel->callbacks = (sim_log_callback_t**)realloc(channel->callbacks, sizeof(FILE*) * channel->size_callbacks);
  }

  callback = malloc(sizeof(*callback));
  callback->handle = handle;
  callback->data = data;

  channel->callbacks[channel->num_callbacks] = callback;
  channel->num_callbacks++;

  sim_log_commit_change();
}

void sim_log_commit_change(void) {
  int i;
  for (i = 0; i < SIM_LOG_OUTPUT_COUNT; i++) {
    if (outputs[i].files != NULL) {
      outputs[i].num_files = 0;
      free(outputs[i].files);
      outputs[i].files = NULL;
    }

    if (outputs[i].callbacks != NULL) {
      outputs[i].num_callbacks = 0;
      free(outputs[i].callbacks);
      outputs[i].callbacks = NULL;
    }
  }
}


void sim_log_debug(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL || outputs[id].callbacks == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num_files; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    fprintf(file, "D:%lu:%lf:", sim_node(), sim_time() / (double)sim_ticks_per_sec());
    vfprintf(file, format, args); 
    va_end(args);
    fflush(file);
  }
  write_performed = (outputs[id].num_files > 0) || outputs[id].num_callbacks > 0;

  if (outputs[id].num_callbacks > 0) {
    char buffer[512];
    int length;

    va_start(args, format);
    length = snprintf(buffer, sizeof(buffer), "D:%lu:%lf:", sim_node(), sim_time() / (double)sim_ticks_per_sec());
    length += vsnprintf(buffer + length, sizeof(buffer) - length, format, args);
    va_end(args);

    for (i = 0; i < outputs[id].num_callbacks; ++i) {
      sim_log_callback_t* callback = outputs[id].callbacks[i];

      callback->handle(callback->data, buffer, length);
    }
  }
}

void sim_log_error(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL || outputs[id].callbacks == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num_files; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    fprintf(file, "E:%lu:%lf:", sim_node(), sim_time() / (double)sim_ticks_per_sec());
    vfprintf(file, format, args);
    va_end(args);
    fflush(file);
  }
  write_performed = (outputs[id].num_files > 0) || outputs[id].num_callbacks > 0;

  if (outputs[id].num_callbacks > 0) {
    char buffer[512];
    int length;

    va_start(args, format);
    length = snprintf(buffer, sizeof(buffer), "E:%lu:%lf:", sim_node(), sim_time() / (double)sim_ticks_per_sec());
    length += vsnprintf(buffer + length, sizeof(buffer) - length, format, args);
    va_end(args);

    for (i = 0; i < outputs[id].num_callbacks; ++i) {
      sim_log_callback_t* callback = outputs[id].callbacks[i];

      callback->handle(callback->data, buffer, length);
    }
  }
}

void sim_log_debug_clear(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL || outputs[id].callbacks == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num_files; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    fflush(file);
  }
  write_performed = (outputs[id].num_files > 0) || outputs[id].num_callbacks > 0;

  if (outputs[id].num_callbacks > 0) {
    char buffer[512];
    int length;

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (i = 0; i < outputs[id].num_callbacks; ++i) {
      sim_log_callback_t* callback = outputs[id].callbacks[i];

      callback->handle(callback->data, buffer, length);
    }
  }
}

void sim_log_error_clear(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL || outputs[id].callbacks == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num_files; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    fflush(file);
  }
  write_performed = (outputs[id].num_files > 0) || outputs[id].num_callbacks > 0;

  if (outputs[id].num_callbacks > 0) {
    char buffer[512];
    int length;

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (i = 0; i < outputs[id].num_callbacks; ++i) {
      sim_log_callback_t* callback = outputs[id].callbacks[i];

      callback->handle(callback->data, buffer, length);
    }
  }
}

/* This is the sdbm algorithm, taken from
   http://www.cs.yorku.ca/~oz/hash.html -pal */
static unsigned int sim_log_hash(const void* key) {
  const char* str = (const char*)key;
  unsigned int hashVal = 0;
  int hashChar;
  
  while ((hashChar = *str++))
    hashVal = hashChar + (hashVal << 6) + (hashVal << 16) - hashVal;
  
  return hashVal;
}

static int sim_log_eq(const void* key1, const void* key2) {
  return strcmp((const char*)key1, (const char*)key2) == 0;
}
