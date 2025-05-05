#include <ROOT-Sim.h>
#include <math.h>
#include <stdlib.h>

// unsigned int gen;

const int phold_events_per_entity = 1;
const double phold_lambda = 1;
const double phold_lookahead = 1;
const double phold_remote_ratio = 1;
const double phold_zero_delay_ratio = 0;
const int tau = 5;
const simtime_t phold_gvt = 20.0f;
const unsigned ctp_root = 0;


#define LOOP		3

// This is the events' payload which is exchanged across LPs
typedef struct _event_content_type {
	int size;
} event_content_type;


typedef struct _lp_state_type {
	unsigned int events;
	simtime_t gvt_ts;
	unsigned int finished;
	bool root;
	bool end;
} lp_state_type;

    
