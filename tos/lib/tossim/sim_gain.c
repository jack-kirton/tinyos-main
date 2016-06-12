#include <sim_gain.h>

typedef struct sim_gain_noise {
  double mean;
  double range;
} sim_gain_noise_t;


static gain_entry_t* connectivity[TOSSIM_MAX_NODES + 1];
static sim_gain_noise_t localNoise[TOSSIM_MAX_NODES + 1];
static double sensitivity = 4.0;

static gain_entry_t* sim_gain_allocate_link(int mote);
static void sim_gain_deallocate_link(gain_entry_t* linkToDelete);
static void sim_gain_deallocate_links(gain_entry_t* linkToDelete);

void sim_gain_init(void) __attribute__ ((C, spontaneous)) {
  size_t i;

  for (i = 0; i != TOSSIM_MAX_NODES + 1; ++i)
  {
    connectivity[i] = NULL;
    localNoise[i].mean = 0.0;
    localNoise[i].range = 0.0;
  }

  sensitivity = 4.0;
}

void sim_gain_free(void) __attribute__ ((C, spontaneous)) {
  size_t i;

  for (i = 0; i != TOSSIM_MAX_NODES + 1; ++i)
  {
    if (connectivity[i] != NULL) {
      sim_gain_deallocate_links(connectivity[i]);
      connectivity[i] = NULL;
    }
  }
}



gain_entry_t* sim_gain_first(int src) __attribute__ ((C, spontaneous)) {
  if (src > TOSSIM_MAX_NODES) {
    return connectivity[TOSSIM_MAX_NODES];
  } 
  return connectivity[src];
}

gain_entry_t* sim_gain_next(gain_entry_t* currentLink) __attribute__ ((C, spontaneous)) {
  return currentLink->next;
}

void sim_gain_add(int src, int dest, double gain) __attribute__ ((C, spontaneous))  {
  gain_entry_t* current;
  int temp = sim_node();
  if (src > TOSSIM_MAX_NODES) {
    src = TOSSIM_MAX_NODES;
  }
  sim_set_node(src);

  for (current = sim_gain_first(src); current != NULL; current = current->next) {
    if (current->mote == dest) {
      sim_set_node(temp);
      break;
    }
  }

  if (current == NULL) {
    current = sim_gain_allocate_link(dest);
    current->next = connectivity[src];
    connectivity[src] = current;
  }
  current->mote = dest;
  current->gain = gain;
  dbg("Gain", "Adding link from %i to %i with gain %f\n", src, dest, gain);
  sim_set_node(temp);
}

double sim_gain_value(int src, int dest) __attribute__ ((C, spontaneous))  {
  gain_entry_t* current;
  int temp = sim_node();
  sim_set_node(src);
  for (current = sim_gain_first(src); current != NULL; current = current->next) {
    if (current->mote == dest) {
      sim_set_node(temp);
      dbg("Gain", "Getting link from %i to %i with gain %f\n", src, dest, current->gain);
      return current->gain;
    }
  }
  sim_set_node(temp);
  dbg("Gain", "Getting default link from %i to %i with gain %f\n", src, dest, 1.0);
  return 1.0;
}

bool sim_gain_connected(int src, int dest) __attribute__ ((C, spontaneous)) {
  gain_entry_t* current;
  int temp = sim_node();
  sim_set_node(src);
  for (current = sim_gain_first(src); current != NULL; current = current->next) {
    if (current->mote == dest) {
      sim_set_node(temp);
      return TRUE;
    }
  }
  sim_set_node(temp);
  return FALSE;
}
  
void sim_gain_remove(int src, int dest) __attribute__ ((C, spontaneous))  {
  gain_entry_t* current;
  gain_entry_t* prevLink;
  int temp = sim_node();
  
  if (src > TOSSIM_MAX_NODES) {
    src = TOSSIM_MAX_NODES;
  }

  sim_set_node(src);
    
  current = sim_gain_first(src);
  prevLink = NULL;
    
  while (current != NULL) {
    gain_entry_t* tmp;
    if (current->mote == dest) {
      if (prevLink == NULL) {
        connectivity[src] = current->next;
      }
      else {
        prevLink->next = current->next;
      }
      tmp = current->next;
      sim_gain_deallocate_link(current);
      current = tmp;
    }
    else {
      prevLink = current;
      current = current->next;
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

static gain_entry_t* sim_gain_allocate_link(int mote) {
  gain_entry_t* newLink = (gain_entry_t*)malloc(sizeof(gain_entry_t));
  newLink->next = NULL;
  newLink->mote = mote;
  newLink->gain = -10000000.0;
  return newLink;
}

static void sim_gain_deallocate_link(gain_entry_t* linkToDelete) {
  free(linkToDelete);
}

static void sim_gain_deallocate_links(gain_entry_t* linkToDelete) {
  gain_entry_t* next = linkToDelete->next;
  sim_gain_deallocate_link(linkToDelete);
  if (next != NULL) {
    sim_gain_deallocate_link(next);
  }
}

void sim_gain_set_sensitivity(double s) __attribute__ ((C, spontaneous)) {
  sensitivity = s;
}

double sim_gain_sensitivity() __attribute__ ((C, spontaneous)) {
  return sensitivity;
}
