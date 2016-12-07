#include <sim_gain.h>
#include <hash_table.h>
#include "murmur3hash.h"

typedef struct sim_gain_noise {
  double mean;
  double range;
} sim_gain_noise_t;

static uint32_t node_pair_hash(const void* key)
{
  return *(const int*)key;
}

static int node_pair_equal(const void* a, const void* b)
{
  const int* const desta = (const int*)a;
  const int* const destb = (const int*)b;

  return *desta == *destb;
}

static void node_table_free(struct hash_entry *entry)
{
  free(entry->data);
}


static struct hash_table connectivity[TOSSIM_MAX_NODES + 1];
static sim_gain_noise_t localNoise[TOSSIM_MAX_NODES + 1];
static double sensitivity = 4.0;

void sim_gain_init(void) __attribute__ ((C, spontaneous)) {
  size_t i;

  for (i = 0; i != TOSSIM_MAX_NODES + 1; ++i)
  {
    hash_table_create(&connectivity[i], &node_pair_hash, &node_pair_equal);

    localNoise[i].mean = 0.0;
    localNoise[i].range = 0.0;
  }

  sensitivity = 4.0;
}

void sim_gain_free(void) __attribute__ ((C, spontaneous)) {
  size_t i;

  for (i = 0; i != TOSSIM_MAX_NODES + 1; ++i)
  {
    hash_table_destroy(&connectivity[i], &node_table_free);
  }
}

// To maintain backwards compatibility we need to iterate in reverse

const void* sim_gain_iter(int src) __attribute__ ((C, spontaneous)) {
  if (src > TOSSIM_MAX_NODES) {
    return NULL;
  }

  return hash_table_next_entry_reverse(&connectivity[src], NULL);
}

const void* sim_gain_next(int src, const void* iter) __attribute__ ((C, spontaneous)) {
  return hash_table_next_entry_reverse(&connectivity[src], (struct hash_entry *)iter);
}

const gain_entry_t* sim_gain_iter_get(const void* iter) __attribute__ ((C, spontaneous)) {
  return (const gain_entry_t*)((const struct hash_entry*)iter)->data;
}

void sim_gain_add(int src, int dest, double gain) __attribute__ ((C, spontaneous))  {
  gain_entry_t* item;

  const int temp = sim_node();
  if (src > TOSSIM_MAX_NODES) {
    return;
  }
  sim_set_node(src);

  item = (gain_entry_t*)hash_table_search_data(&connectivity[src], &dest);

  if (item == NULL) {
    item = malloc(sizeof(gain_entry_t));
    item->mote = dest;
    item->gain = gain;

    hash_table_insert(&connectivity[src], &item->mote, item);
  }
  else {
    item->gain = gain;
  }

  dbg("Gain", "Adding link from %i to %i with gain %f\n", src, dest, gain);

  sim_set_node(temp);
}

// Note that these values depend on the hash table iterating in insertion order.

double sim_gain_value(int src, int dest) __attribute__ ((C, spontaneous)) {
  const gain_entry_t* const item = (gain_entry_t*)hash_table_search_data(&connectivity[src], &dest);
  const double result = item == NULL ? 1.0 : item->gain;

  dbg("Gain", "Getting default link from %i to %i with gain %f\n", src, dest, result);

  return result;
}

bool sim_gain_connected(int src, int dest) __attribute__ ((C, spontaneous)) {
  const gain_entry_t* const item = (gain_entry_t*)hash_table_search_data(&connectivity[src], &dest);
  return item != NULL;
}
  
void sim_gain_remove(int src, int dest) __attribute__ ((C, spontaneous)) {
  struct hash_entry* entry;

  const int temp = sim_node();
  
  if (src > TOSSIM_MAX_NODES) {
    return;
  }

  sim_set_node(src);

  entry = hash_table_search(&connectivity[src], &dest);
  if (entry != NULL) {
    node_table_free(entry);
    hash_table_remove_entry(&connectivity[src], entry);
  }

  sim_set_node(temp);
}

void sim_gain_set_noise_floor(int node, double mean, double range) __attribute__ ((C, spontaneous))  {
  if (node >= TOSSIM_MAX_NODES) {
    return;
  }
  localNoise[node].mean = mean;
  localNoise[node].range = range;
}

double sim_gain_noise_mean(int node) {
  if (node >= TOSSIM_MAX_NODES) {
    return NAN;
  }
  return localNoise[node].mean;
}

double sim_gain_noise_range(int node) {
  if (node >= TOSSIM_MAX_NODES) {
    return NAN;
  }
  return localNoise[node].range;
}

// Pick a number a number from the uniform distribution of
// [mean-range, mean+range].
double sim_gain_sample_noise(int node)  __attribute__ ((C, spontaneous)) {
  double val, adjust;
  if (node >= TOSSIM_MAX_NODES) {
    return NAN;
  } 
  val = localNoise[node].mean;
  adjust = (sim_random() % 2000000);
  adjust /= 1000000.0;
  adjust -= 1.0;
  adjust *= localNoise[node].range;
  return val + adjust;
}

void sim_gain_set_sensitivity(double s) __attribute__ ((C, spontaneous)) {
  sensitivity = s;
}

double sim_gain_sensitivity(void) __attribute__ ((C, spontaneous)) {
  return sensitivity;
}
