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
#include <hashtable.h>
#include <string.h>

enum {
  DEFAULT_CHANNEL_SIZE = 8
};

typedef struct sim_log_output {
  int num;
  FILE** files;
} sim_log_output_t;

typedef struct sim_log_channel {
  const char* name;
  int numOutputs;
  int size;
  FILE** outputs;
} sim_log_channel_t;

enum {
  SIM_LOG_OUTPUT_COUNT = uniqueCount("TOSSIM.debug")
};

static sim_log_output_t outputs[SIM_LOG_OUTPUT_COUNT];
static struct hashtable* channelTable = NULL;

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
  int count = 0;
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
    
    channel = hashtable_search(channelTable, newName);
    if (channel != NULL) {
      count += channel->numOutputs;
    }

    namePos = termination + 1;
  }

  termination = name;
  namePos = name;
  
  // Allocate
  outputs[id].files = (FILE**)malloc(sizeof(FILE*) * count);
  outputs[id].num = 0;

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
    
    channel = hashtable_search(channelTable, newName);
    if (channel != NULL) {
      int i, j;
      for (i = 0; i < channel->numOutputs; i++) {
        int duplicate = 0;
        int outputCount = outputs[id].num;
        // Check if we already have this file descriptor in the output
        // set, and if so, ignore it.
        for (j = 0; j < outputCount; j++) {
          if (fileno(outputs[id].files[j]) == fileno(channel->outputs[i])) {
            duplicate = 1;
            j = outputCount;
          }
        }
        if (!duplicate) {
          outputs[id].files[outputCount] = channel->outputs[i];
          outputs[id].num++;
        }
      }
    }
    namePos = termination + 1;
  }
  free(newName);
}

void sim_log_init(void) {
  int i;

  channelTable = create_hashtable(128, sim_log_hash, sim_log_eq);
  
  for (i = 0; i < SIM_LOG_OUTPUT_COUNT; i++) {
    outputs[i].num = 1;
    outputs[i].files = (FILE**)malloc(sizeof(FILE*) * 1);
    outputs[i].files[0] = stdout;
  }

  write_performed = FALSE;
}

void sim_log_free(void) {
  int i;

  write_performed = FALSE;

  for (i = 0; i < SIM_LOG_OUTPUT_COUNT; i++) {
    free(outputs[i].files);
    outputs[i].files = NULL;
    outputs[i].num = 0;
  }

  hashtable_destroy(channelTable, &free);
  channelTable = NULL;
}

void sim_log_add_channel(const char* name, FILE* file) {
  sim_log_channel_t* channel;
  channel = (sim_log_channel_t*)hashtable_search(channelTable, name);
  
  // If there's no current entry, allocate one, initialize it,
  // and insert it.
  if (channel == NULL) {
    char* newName = strdup(name);
    
    channel = (sim_log_channel_t*)malloc(sizeof(sim_log_channel_t));
    channel->name = newName;
    channel->numOutputs = 0;
    channel->size = DEFAULT_CHANNEL_SIZE;
    channel->outputs = (FILE**)calloc(channel->size, sizeof(FILE*));
    hashtable_insert(channelTable, newName, channel);
  }

  // If the channel output table is full, double the size of
  // channel->outputs.
  if (channel->numOutputs == channel->size) {
    channel->size *= 2;
    channel->outputs = (FILE**)realloc(channel->outputs, sizeof(FILE*) * channel->size);
  }

  channel->outputs[channel->numOutputs] = file;
  channel->numOutputs++;
  sim_log_commit_change();
}

bool sim_log_remove_channel(const char* output, FILE* file) {
  sim_log_channel_t* channel;
  int i;
  channel = (sim_log_channel_t*)hashtable_search(channelTable, output);  

  if (channel == NULL) {
    return FALSE;
  }

  // Note: if a FILE* has duplicates, this removes all of them
  for (i = 0; i < channel->numOutputs; i++) {
    FILE* f = channel->outputs[i];
    if (file == f) {
      memcpy(&channel->outputs[i], &channel->outputs[i + 1], (channel->numOutputs) - (i + 1));
      channel->outputs[channel->numOutputs - 1] = NULL;
      channel->numOutputs--;
    }
  }
  
  return TRUE;
}
  
void sim_log_commit_change(void) {
  int i;
  for (i = 0; i < SIM_LOG_OUTPUT_COUNT; i++) {
    if (outputs[i].files != NULL) {
      outputs[i].num = 0;
      free(outputs[i].files);
      outputs[i].files = NULL;
    }
  }
}


void sim_log_debug(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    fprintf(file, "D:%lu:%lf:", sim_node(), sim_time() / (double)sim_ticks_per_sec());
    vfprintf(file, format, args); 
    fflush(file);
  }
  write_performed = TRUE;
}

void sim_log_error(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    fprintf(file, "E:%lu:%lf:", sim_node(), sim_time() / (double)sim_ticks_per_sec());
    vfprintf(file, format, args);
    fflush(file);
  }
  write_performed = TRUE;
}

void sim_log_debug_clear(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    vfprintf(file, format, args);
    fflush(file);
  }
  write_performed = TRUE;
}

void sim_log_error_clear(uint16_t id, const char* string, const char* format, ...) {
  va_list args;
  int i;
  if (outputs[id].files == NULL) {
    fillInOutput(id, string);
  }
  for (i = 0; i < outputs[id].num; i++) {
    FILE* file = outputs[id].files[i];
    va_start(args, format);
    vfprintf(file, format, args);
    fflush(file);
  }
  write_performed = TRUE;
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
