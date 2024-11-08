#include <ROOT-Sim.h>
#include <math.h>
#include <stdlib.h>

unsigned int gen;

const int phold_events_per_entity = 1;
const double phold_lambda = 1;
const double phold_lookahead = 0;
const double phold_remote_ratio = 1;
const double phold_zero_delay_ratio = 0;
const int tau = 5;
#define LOOP		3

// This is the events' payload which is exchanged across LPs
typedef struct _event_content_type {
	int size;
} event_content_type;


typedef struct _lp_state_type {
	unsigned int events;
	int loop_counter;
	simtime_t gvt_ts;


	unsigned int cont_allocation;
	unsigned int num_elementi;
	int actual_size;
	int total_size;
	int next_lp;
	int *taglie;
	int *elementi;
} lp_state_type;

    
