#include <ROOT-Sim.h>
#include "granularity.h"

struct _topology_settings_t topology_settings = {.default_geometry = TOPOLOGY_SQUARE};

void ProcessEvent(unsigned int me, simtime_t now, unsigned int event, event_t *content, unsigned int size, lp_state_t *state) {
    (void)content;
    (void)size;
	event_t new_event;
	simtime_t timestamp;

    switch (event) {

        case INIT:
            state = (lp_state_t *)malloc(sizeof(lp_state_t));
            state->computation_result = 0;
            SetState(state);

            // Schedule a very large computational task far in the future
            timestamp = (simtime_t)(INIT_DELAY * Random());
            ScheduleNewEvent(me, timestamp, HEAVY_COMPUTE, NULL, 0);
            break;

        case HEAVY_COMPUTE:
            // Large computation (simulating CPU-bound task)
            for (int i = 0; i < COMPUTATION_CYCLES; i++) {
                state->computation_result += (i % 2 == 0) ? i : -i;
            }

			new_event.sent_at = now;
			new_event.sender = me;

			int recv = FindReceiver();

            // Schedule the next heavy compute far into the future
            timestamp = now + (simtime_t)(DELAY * Random());
            ScheduleNewEvent(recv, timestamp, HEAVY_COMPUTE, &new_event, sizeof(new_event));
            break;
    }
}

bool OnGVT(unsigned int me, lp_state_t *snapshot) {
	if(me == 0) {
		if (snapshot->computation_result < FINISH_CONDITION)
			return false;
	}
	return true;
}
