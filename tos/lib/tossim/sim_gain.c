#include <sim_gain.h>

typedef struct sim_gain_noise {
  double mean;
  double range;
} sim_gain_noise_t;

typedef struct sim_gain_list {
  gain_entry_t* entries;
  size_t size;
  size_t capacity;
} sim_gain_list_t;


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
      connectivity[i].size = 0;
      connectivity[i].capacity = 0;
    }
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
    iter = sim_gain_append(src);
  }

  iter->mote = dest;
  iter->gain = gain;

  dbg("Gain", "Adding link from %i to %i with gain %f\n", src, dest, gain);

  sim_set_node(temp);
}

double sim_gain_value(int src, int dest) __attribute__ ((C, spontaneous))  {
  gain_entry_t* iter;
  gain_entry_t* end;

  int temp = sim_node();

  sim_set_node(src);

  for (iter = sim_gain_begin(src), end = sim_gain_end(src); iter != end; iter = sim_gain_next(iter)) {
    if (iter->mote == dest) {
      sim_set_node(temp);
      dbg("Gain", "Getting link from %i to %i with gain %f\n", src, dest, iter->gain);
      return iter->gain;
    }
  }

  sim_set_node(temp);
  dbg("Gain", "Getting default link from %i to %i with gain %f\n", src, dest, 1.0);
  return 1.0;
}

bool sim_gain_connected(int src, int dest) __attribute__ ((C, spontaneous)) {
  gain_entry_t* iter;
  gain_entry_t* end;

  int temp = sim_node();
  sim_set_node(src);
  for (iter = sim_gain_begin(src), end = sim_gain_end(src); iter != end; iter = sim_gain_next(iter)) {
    if (iter->mote == dest) {
      sim_set_node(temp);
      return TRUE;
    }
  }
  sim_set_node(temp);
  return FALSE;
}
  
void sim_gain_remove(int src, int dest) __attribute__ ((C, spontaneous))  {
  gain_entry_t* iter;
  gain_entry_t* end;

  int temp = sim_node();
  
  if (src > TOSSIM_MAX_NODES) {
    src = TOSSIM_MAX_NODES;
  }

  sim_set_node(src);
    
  iter = sim_gain_begin(src);
  end = sim_gain_end(src);

  for (iter = connectivity[src].entries, end = connectivity[src].entries + connectivity[src].size; iter != end; ++iter) {
    if (iter->mote == dest) {

      memcpy(iter, iter + 1, end - (iter + 1));

      connectivity[src].size -= 1;
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

double sim_gain_sensitivity() __attribute__ ((C, spontaneous)) {
  return sensitivity;
}
