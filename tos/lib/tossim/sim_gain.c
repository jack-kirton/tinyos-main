#include <sim_gain.h>
#include <hash_table.h>
#include "murmur3hash.h"

typedef struct sim_gain_noise {
  double mean;
  double range;
} sim_gain_noise_t;

typedef struct sim_gain_list {
  gain_entry_t* entries;
  size_t size;
  size_t capacity;
  struct hash_table table;
} sim_gain_list_t;

typedef struct {
  int dest;
  double gain;
} node_dest_gain_t;

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


static sim_gain_list_t connectivity[TOSSIM_MAX_NODES + 1];
static sim_gain_noise_t localNoise[TOSSIM_MAX_NODES + 1];
static double sensitivity = 4.0;

void sim_gain_init(void) __attribute__ ((C, spontaneous)) {
  size_t i;

  for (i = 0; i != TOSSIM_MAX_NODES + 1; ++i)
  {
    connectivity[i].capacity = 16;
    connectivity[i].entries = (gain_entry_t*)malloc(sizeof(gain_entry_t) * connectivity[i].capacity);
    connectivity[i].size = 0;

    hash_table_create(&connectivity[i].table, &node_pair_hash, &node_pair_equal);

    localNoise[i].mean = 0.0;
    localNoise[i].range = 0.0;
  }

  sensitivity = 4.0;
}

void sim_gain_free(void) __attribute__ ((C, spontaneous)) {
  size_t i;

  for (i = 0; i != TOSSIM_MAX_NODES + 1; ++i)
  {
    if (connectivity[i].entries != NULL)
    {
      free(connectivity[i].entries);
      connectivity[i].entries = NULL;
    }

    connectivity[i].size = 0;
    connectivity[i].capacity = 0;

    hash_table_destroy(&connectivity[i].table, &node_table_free);
  }
}

// To maintain backwards compatibility we need to iterate in reverse

gain_entry_t* sim_gain_begin(int src) __attribute__ ((C, spontaneous)) {
  if (src > TOSSIM_MAX_NODES) {
    return connectivity[TOSSIM_MAX_NODES].entries;
  } 
  return connectivity[src].entries + (connectivity[src].size - 1);
}

gain_entry_t* sim_gain_end(int src) __attribute__ ((C, spontaneous)) {
  if (src > TOSSIM_MAX_NODES) {
    return connectivity[TOSSIM_MAX_NODES].entries + connectivity[TOSSIM_MAX_NODES].size;
  } 
  return connectivity[src].entries - 1;
}

static gain_entry_t* sim_gain_append(int src) {
  if (connectivity[src].size == connectivity[src].capacity) {
    connectivity[src].capacity *= 2;
    connectivity[src].entries = (gain_entry_t*)realloc(connectivity[src].entries, sizeof(gain_entry_t) * connectivity[src].capacity);
  }

  connectivity[src].size += 1;

  return connectivity[src].entries + (connectivity[src].size - 1);
}

void sim_gain_add(int src, int dest, double gain) __attribute__ ((C, spontaneous))  {
  gain_entry_t* iter;
  gain_entry_t* end;

  int temp = sim_node();
  if (src > TOSSIM_MAX_NODES) {
    src = TOSSIM_MAX_NODES;
  }
  sim_set_node(src);

  for (iter = sim_gain_begin(src), end = sim_gain_end(src); iter != end; iter = sim_gain_next(iter)) {
    if (iter->mote == dest) {
      sim_set_node(temp);
      break;
    }
  }

  if (iter == end) {
    node_dest_gain_t* item = malloc(sizeof(node_dest_gain_t));
    item->dest = dest;
    item->gain = gain;

    hash_table_insert(&connectivity[src].table, &item->dest, item);

    iter = sim_gain_append(src);
  }
  else {
    node_dest_gain_t* item = (node_dest_gain_t*)hash_table_search_data(&connectivity[src].table, &dest);
    item->gain = gain;
  }

  iter->mote = dest;
  iter->gain = gain;

  dbg("Gain", "Adding link from %i to %i with gain %f\n", src, dest, gain);

  sim_set_node(temp);
}

double sim_gain_value(int src, int dest) __attribute__ ((C, spontaneous))  {
  const node_dest_gain_t* const item = (node_dest_gain_t*)hash_table_search_data(&connectivity[src].table, &dest);
  const double result = item == NULL ? 1.0 : item->gain;

  dbg("Gain", "Getting default link from %i to %i with gain %f\n", src, dest, result);

  return result;
}

bool sim_gain_connected(int src, int dest) __attribute__ ((C, spontaneous)) {
  const node_dest_gain_t* const item = (node_dest_gain_t*)hash_table_search_data(&connectivity[src].table, &dest);
  return item != NULL;
}
  
void sim_gain_remove(int src, int dest) __attribute__ ((C, spontaneous))  {
  gain_entry_t* iter;
  gain_entry_t* end;

  int temp = sim_node();
  
  if (src > TOSSIM_MAX_NODES) {
    src = TOSSIM_MAX_NODES;
  }

  sim_set_node(src);

  for (iter = connectivity[src].entries, end = connectivity[src].entries + connectivity[src].size; iter != end; ++iter) {
    if (iter->mote == dest) {
      struct hash_entry* entry = hash_table_search(&connectivity[src].table, &dest);
      node_table_free(entry);
      hash_table_remove_entry(&connectivity[src].table, entry);

      memcpy(iter, iter + 1, end - (iter + 1));

      connectivity[src].size -= 1;

      break;
    }
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
