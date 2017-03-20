/*
 * Copyright (c) 2006 Stanford University. All rights reserved.
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
 * - Neither the name of the copyright holder nor the names of
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
 * Implementation of all of the Hash-Based Learning primitives and utility
 * functions.
 *
 * @author Hyungjune Lee
 * @date   Oct 13 2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "randomlib.h"
#include "hash_table.h"
#include "murmur3hash.h"
#include "sim_noise.h"
#include "StaticAssert.h"

#ifdef DEBUG
//Tal Debug, to count how often simulation hits the one match case
static int numCase1 = 0;
static int numCase2 = 0;
static int numTotal = 0;
//End Tal Debug
#endif

typedef struct sim_noise_hash_t {
  char key[NOISE_HISTORY];
  unsigned int numElements;
  unsigned int size;
  bool flag;
  char *elements;
  float dist[NOISE_NUM_VALUES];
} sim_noise_hash_t;

typedef struct sim_noise_node_t {
  char key[NOISE_HISTORY];
  char freqKey[NOISE_HISTORY];
  char lastNoiseVal;
  bool generated;
  uint32_t noiseGenTime;
  struct hash_table noiseTable;
  char* noiseTrace;
  uint32_t noiseTraceLen;
  uint32_t noiseTraceIndex;
} sim_noise_node_t;

static uint32_t FreqKeyNum = 0;

static sim_noise_node_t noiseData[TOSSIM_MAX_NODES];

static unsigned int sim_noise_hash(const void *key);
static int sim_noise_eq(const void *key1, const void *key2);

static void makeNoiseModel(uint16_t node_id);
static void makePmfDistr(uint16_t node_id);
static uint8_t search_bin_num(char noise);

void sim_noise_init(void) __attribute__ ((C, spontaneous))
{
  int j;
  
  //printf("Starting\n");

  FreqKeyNum = 0;
  
  for (j = 0; j < TOSSIM_MAX_NODES; j++) {
    hash_table_create(&noiseData[j].noiseTable, sim_noise_hash, sim_noise_eq);
    noiseData[j].noiseGenTime = 0;
    noiseData[j].noiseTrace = (char*)malloc(sizeof(char) * NOISE_MIN_TRACE);
    noiseData[j].noiseTraceLen = NOISE_MIN_TRACE;
    noiseData[j].noiseTraceIndex = 0;
  }
  //printf("Done with sim_noise_init()\n");
}

static void noise_table_entry_free(struct hash_entry* entry)
{
  free(entry->data);
}

void sim_noise_free(void) __attribute__ ((C, spontaneous)) {
  int j;
  for (j = 0; j < TOSSIM_MAX_NODES; j++) {
    hash_table_destroy(&noiseData[j].noiseTable, &noise_table_entry_free);

    noiseData[j].noiseGenTime = 0;

    free(noiseData[j].noiseTrace);
    noiseData[j].noiseTrace = NULL;

    noiseData[j].noiseTraceLen = 0;
    noiseData[j].noiseTraceIndex = 0;
  }

  FreqKeyNum = 0;
}

void sim_noise_create_model(uint16_t node_id) __attribute__ ((C, spontaneous)) {
  makeNoiseModel(node_id);
  makePmfDistr(node_id);
}

char sim_real_noise(uint16_t node_id, uint32_t cur_t) {
  if (cur_t > noiseData[node_id].noiseTraceLen) {
    dbgerror("Noise", "Asked for noise element %u when there are only %u.\n", cur_t, noiseData[node_id].noiseTraceIndex);
    return 0;
  }
  return noiseData[node_id].noiseTrace[cur_t];
}

void sim_noise_reserve(uint16_t node_id, uint32_t num_traces) __attribute__ ((C, spontaneous)) {
  sim_noise_node_t* const noise = &noiseData[node_id];
  if (num_traces > noise->noiseTraceLen) {
    noise->noiseTrace = (char*)realloc(noise->noiseTrace, sizeof(char) * num_traces);
    noise->noiseTraceLen = num_traces;
  }
}

void sim_noise_trace_add(uint16_t node_id, char noiseVal) __attribute__ ((C, spontaneous)) {
  sim_noise_node_t* const noise = &noiseData[node_id];
  // Need to double size of trace array
  if (noise->noiseTraceIndex == noise->noiseTraceLen) {
    noise->noiseTrace = (char*)realloc(noise->noiseTrace, sizeof(char) * noise->noiseTraceLen * 2);
    noise->noiseTraceLen *= 2;
  }
  noise->noiseTrace[noise->noiseTraceIndex] = noiseVal;
  noise->noiseTraceIndex++;

#ifdef DEBUG
  dbg("Insert", "Adding noise value %i for %i of %i\n", (int)noise->noiseTraceIndex, (int)node_id, (int)noiseVal);
#endif
}


uint8_t search_bin_num(char noise) __attribute__ ((C, spontaneous))
{
  uint8_t bin;
  if (noise > NOISE_MAX || noise < NOISE_MIN) {
    noise = NOISE_MIN;
  }
  bin = (noise-NOISE_MIN)/NOISE_QUANTIZE_INTERVAL + 1;
  return bin;
}

char search_noise_from_bin_num(int i)__attribute__ ((C, spontaneous))
{
  char noise;
  noise = NOISE_MIN + (i-1)*NOISE_QUANTIZE_INTERVAL;
  return noise;
}

STATIC_ASSERT_MSG(NOISE_HISTORY == 20, NOISE_HISTORY_must_be_20_bytes_long);

static unsigned int sim_noise_hash(const void *key) {
  const uint32_t* data = (const uint32_t*)key;
  register uint32_t h = _MURMUR_SEED;
  _ROUND32(data[0])
  _ROUND32(data[1])
  _ROUND32(data[2])
  _ROUND32(data[3])
  _ROUND32(data[4])
  _FMIX32(20)
  return h;
}

static int sim_noise_eq(const void *key1, const void *key2) {

  //key1 = (const void *)__builtin_assume_aligned(key1, __alignof(sim_noise_hash_t));
  //key2 = (const void *)__builtin_assume_aligned(key2, __alignof(sim_noise_hash_t));

  return memcmp(key1, key2, NOISE_HISTORY) == 0;
}

void sim_noise_add(uint16_t node_id, char noise)__attribute__ ((C, spontaneous))
{
  int i;

  struct hash_table * const pnoiseTable = &noiseData[node_id].noiseTable;
  const char * const key = noiseData[node_id].key;
  sim_noise_hash_t *noise_hash = (sim_noise_hash_t *)hash_table_search_data(pnoiseTable, key);

#ifdef DEBUG
  dbg("Insert", "Adding noise value %hhi\n", noise);
#endif

  if (noise_hash == NULL)	{
    noise_hash = (sim_noise_hash_t *)malloc(sizeof(sim_noise_hash_t));
    memcpy(noise_hash->key, key, NOISE_HISTORY);
    
    noise_hash->numElements = 0;
    noise_hash->size = NOISE_DEFAULT_ELEMENT_SIZE;
    noise_hash->elements = (char *)calloc(noise_hash->size, sizeof(char));
    noise_hash->flag = FALSE;
    for(i=0; i<NOISE_NUM_VALUES; i++) {
      noise_hash->dist[i] = 0.0f;
    }

    hash_table_insert(pnoiseTable, noise_hash->key, noise_hash);

#ifdef DEBUG
    dbg("Insert", "Inserting %p into table %p with key ", noise_hash, pnoiseTable);
    {
      int ctr;
      for(ctr = 0; ctr < NOISE_HISTORY; ctr++)
        dbg_clear("Insert", "%0.3hhi ", key[ctr]);
    }
    dbg_clear("Insert", "\n");
#endif
  }

  if (noise_hash->numElements == noise_hash->size)
  {
    noise_hash->size *= 2;
    noise_hash->elements = (char *)realloc(noise_hash->elements, sizeof(char) * noise_hash->size);
  }

  noise_hash->elements[noise_hash->numElements] = noise;
//  printf("I hear noise %i\n", noise);
  noise_hash->numElements++;
}

void sim_noise_dist(uint16_t node_id)__attribute__ ((C, spontaneous))
{
  size_t i;
  float cmf = 0.0f;
  struct hash_table * const pnoiseTable = &noiseData[node_id].noiseTable;
  const char * __restrict const key = noiseData[node_id].key;
  char * __restrict const freqKey = noiseData[node_id].freqKey;
  sim_noise_hash_t * const noise_hash = (sim_noise_hash_t *)hash_table_search_data(pnoiseTable, key);
  const unsigned int numElements = noise_hash->numElements;

  if (noise_hash->flag) {
    return;
  }

  for (i=0; i < NOISE_NUM_VALUES; i++) {
    noise_hash->dist[i] = 0.0f;
  }
  
  for (i=0; i< numElements; i++)
  {
    const int bin = noise_hash->elements[i] - NOISE_MIN_QUANTIZE; //search_bin_num(noise_hash->elements[i]) - 1;
    //printf("Bin %i, Noise %i\n", bin, (NOISE_MIN_QUANTIZE + bin));

    noise_hash->dist[bin] += 1.0f;

#ifdef DEBUG
    dbg("Noise_output", "Noise is found to be %i\n", noise_hash->elements[i]);
#endif
  }

  for (i=0; i < NOISE_NUM_VALUES ; i++)
  {
    const float dist_i = noise_hash->dist[i];
    const float dist_i_ave = dist_i / numElements;
    cmf += dist_i_ave;
    noise_hash->dist[i] = cmf;
  }

  noise_hash->flag = TRUE;

  //Find the most frequent key and store it in noiseData[node_id].freqKey[].
  if (numElements > FreqKeyNum)
  {
    FreqKeyNum = numElements;
    memcpy(freqKey, key, NOISE_HISTORY);

#ifdef DEBUG
    {
      int j;
      dbg("HashZeroDebug", "Setting most frequent key (%u): ", FreqKeyNum);
      for (j = 0; j < NOISE_HISTORY; j++) {
        dbg_clear("HashZeroDebug", "[%hhu] ", key[j]);
      }
      dbg_clear("HashZeroDebug", "\n");
    }
#endif
  }
}

void arrangeKey(uint16_t node_id)__attribute__ ((C, spontaneous))
{
  char * const pKey = noiseData[node_id].key;
  memmove(pKey, pKey+1, NOISE_HISTORY-1);
}

/*
 * After makeNoiseModel() is done, make PMF distribution for each bin.
 */
 void makePmfDistr(uint16_t node_id)__attribute__ ((C, spontaneous))
{
  size_t i;
  char * __restrict const pKey = noiseData[node_id].key;

#ifdef DEBUG
  char * __restrict const fKey = noiseData[node_id].freqKey;
#endif

  FreqKeyNum = 0;
  for (i=0; i < NOISE_HISTORY; i++) {
    pKey[i] = /* noiseData[node_id].noiseTrace[i]; // */ search_bin_num(noiseData[node_id].noiseTrace[i]);
  }

  for (i = NOISE_HISTORY; i < noiseData[node_id].noiseTraceIndex; i++) {
    sim_noise_dist(node_id);
    arrangeKey(node_id);
    pKey[NOISE_HISTORY-1] =  search_bin_num(noiseData[node_id].noiseTrace[i]);
  }

#ifdef DEBUG
  dbg_clear("HASH", "FreqKey = ");
  for (i=0; i< NOISE_HISTORY ; i++)
  {
    dbg_clear("HASH", "%d,", fKey[i]);
  }
  dbg_clear("HASH", "\n");
#endif
}

#ifdef DEBUG
int dummy;
void sim_noise_alarm() {
  dummy = 5;
}
#endif

char sim_noise_gen(uint16_t node_id)__attribute__ ((C, spontaneous))
{
  int i;
  //int noiseIndex = 0;
  char noise = 0;
  struct hash_table * const pnoiseTable = &noiseData[node_id].noiseTable;
  char * __restrict const pKey = noiseData[node_id].key;
  const char * __restrict const fKey = noiseData[node_id].freqKey;
  const double ranNum = RandomUniform(); // TODO: PERFORMANCE: Move after if
  sim_noise_hash_t *noise_hash;

  noise_hash = (sim_noise_hash_t *)hash_table_search_data(pnoiseTable, pKey);

  if (noise_hash == NULL) {
#ifdef DEBUG
    //Tal Debug
    dbg("Noise_c", "Did not pattern match");
    //End Tal Debug
    sim_noise_alarm();
    dbg_clear("HASH", "(N)Noise\n");
    dbg("HashZeroDebug", "Defaulting to common hash.\n");
#endif
    memcpy(pKey, fKey, NOISE_HISTORY);
    noise_hash = (sim_noise_hash_t *)hash_table_search_data(pnoiseTable, pKey);
  }
  
#ifdef DEBUG
  dbg_clear("HASH", "Key = ");
  for (i = 0; i < NOISE_HISTORY; i++) {
    dbg_clear("HASH", "%d,", pKey[i]);
  }
  dbg_clear("HASH", "\n");
  
  dbg("HASH", "Printing Key\n");
  dbg("HASH", "noise_hash->numElements=%d\n", noise_hash->numElements);

  //Tal Debug
  numTotal++;
  //End Tal Debug
#endif

  if (noise_hash->numElements == 1) {
    noise = noise_hash->elements[0];

#ifdef DEBUG
    dbg_clear("HASH", "(E)Noise = %d\n", noise);
    //Tal Debug
    numCase1++;
    dbg("Noise_c", "In case 1: %i of %i\n", numCase1, numTotal);
    //End Tal Debug
    dbg("NoiseAudit", "Noise: %i\n", noise);
#endif

    return noise;
  }

#ifdef DEBUG
  //Tal Debug
  numCase2++;
  dbg("Noise_c", "In case 2: %i of %i\n", numCase2, numTotal);
  //End Tal Debug
#endif
 
  for (i = 0; i < NOISE_NUM_VALUES - 1; i++) {
    //dbg("HASH", "IN:for i=%d\n", i);
    if (i == 0) {
      if (ranNum <= noise_hash->dist[i]) {
        //noiseIndex = i;
        //dbg_clear("HASH", "Selected Bin = %d -> ", i+1);
        break;
      }
    }
    else if ((noise_hash->dist[i-1] < ranNum) && (ranNum <= noise_hash->dist[i])) {
      //noiseIndex = i;
      //dbg_clear("HASH", "Selected Bin = %d -> ", i+1);
      break;
    }
  }
  //dbg("HASH", "OUT:for i=%d\n", i);
  
  noise = NOISE_MIN_QUANTIZE + i; //TODO search_noise_from_bin_num(i+1);
  //dbg("NoiseAudit", "Noise: %i\n", noise);		
  return noise;
}

char sim_noise_generate(uint16_t node_id, uint32_t cur_t)__attribute__ ((C, spontaneous)) {
  uint32_t i;
  const uint32_t prev_t = noiseData[node_id].noiseGenTime;
  uint32_t delta_t;
  char noise;

  if (!noiseData[node_id].generated) {
    dbgerror("TOSSIM", "Tried to generate noise from an uninitialized radio model of node %hu.\n", node_id);
    return 127;
  }
  
  if (/*0U <= cur_t &&*/ cur_t < NOISE_HISTORY) {
    noiseData[node_id].noiseGenTime = cur_t;
    noiseData[node_id].key[cur_t] = search_bin_num(noiseData[node_id].noiseTrace[cur_t]);
    noiseData[node_id].lastNoiseVal = noiseData[node_id].noiseTrace[cur_t];
    return noiseData[node_id].noiseTrace[cur_t];
  }

  delta_t = (prev_t == 0) ? (cur_t - (NOISE_HISTORY-1)) : (cur_t - prev_t);
  
  //dbg_clear("HASH", "delta_t = %d\n", delta_t);
  
  if (delta_t == 0) {
    noise = noiseData[node_id].lastNoiseVal;
  }
  else {
    for (i=0; i< delta_t; i++) {
      noise = sim_noise_gen(node_id);
      arrangeKey(node_id);
      noiseData[node_id].key[NOISE_HISTORY-1] = search_bin_num(noise);
    }
    noiseData[node_id].lastNoiseVal = noise;
  }

  noiseData[node_id].noiseGenTime = cur_t;

#ifdef DEBUG
  if (noise == 0) {
    dbg("HashZeroDebug", "Generated noise of zero.\n");
  }
#endif

  return noise;
}

/* 
 * When initialization process is going on, make noise model by putting
 * experimental noise values.
 */
void makeNoiseModel(uint16_t node_id)__attribute__ ((C, spontaneous)) {
  size_t i, end;
  for(i=0; i<NOISE_HISTORY; i++) {
    noiseData[node_id].key[i] = search_bin_num(noiseData[node_id].noiseTrace[i]);
    //dbg("Insert", "Setting history %i to be %i\n", (int)i, (int)noiseData[node_id].key[i]);
  }
  
  //sim_noise_add(node_id, noiseData[node_id].noiseTrace[NOISE_HISTORY]);
  //arrangeKey(node_id);
  
  for(i = NOISE_HISTORY, end = noiseData[node_id].noiseTraceIndex; i < end; i++) {
    sim_noise_add(node_id, noiseData[node_id].noiseTrace[i]);
    arrangeKey(node_id);
    noiseData[node_id].key[NOISE_HISTORY-1] = search_bin_num(noiseData[node_id].noiseTrace[i]);
  }
  noiseData[node_id].generated = TRUE;
}
