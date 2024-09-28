#include <ROOT-Sim.h>

#define HEAVY_COMPUTE 1 // Event definition
#define DELAY 1
#define FINISH_CONDITION 10000000 // Termination condition
#define COMPUTATION_CYCLES 3000
#define INIT_DELAY 5

typedef struct event_content_t {
	simtime_t sent_at;
	unsigned int sender;
} event_t;

typedef struct lp_state_t{
	int computation_result;
} lp_state_t;
