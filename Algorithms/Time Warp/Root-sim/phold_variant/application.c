#include <stdlib.h>
#include <stdio.h>
#include <ROOT-Sim.h>
#include <stddef.h>
#include <math.h>
#include "application.h"

struct _topology_settings_t topology_settings = {.type = TOPOLOGY_OBSTACLES, .default_geometry = TOPOLOGY_GRAPH, .write_enabled = false};

unsigned int gen = 0;

void ProcessEvent(int me, simtime_t now, int event_type, event_content_type *event_content, unsigned int size, void *state) {
    (void)size;

    simtime_t timestamp;
    lp_state_type *state_ptr = (lp_state_type*)state;

    if(state_ptr!=NULL)
    {
        state_ptr->gvt_ts=now;
        state_ptr->events++;
        if(me==ctp_root) {
            state_ptr->root = true;
        }
    }

    switch (event_type) {

        case INIT:
            state_ptr = (lp_state_type *)malloc(sizeof(lp_state_type));
            if(state_ptr == NULL){
                exit(-1);
            }

            SetState(state_ptr);
            memset(state_ptr, 0, sizeof(lp_state_type));
            gen = rand();

            // Schedule initial events
            for (int i = 0; i < phold_events_per_entity; i++) {
                double new_ts = phold_lookahead + Expent(phold_lambda);
                ScheduleNewEvent(me, new_ts, LOOP, NULL, 0);
            }

            break;

        case LOOP:
            if (state_ptr->gvt_ts > phold_gvt){
                state_ptr->end = true;
            }

            int dst_id = me;

            // Select timestamp to schedule new event
            timestamp = now + phold_lookahead + Expent(phold_lambda); 

            // Select LP that will receive the event
            if (Random() < phold_remote_ratio) {
                dst_id = (me + RandomRange(0, n_prc_tot - 2) + 1) % n_prc_tot;
            } else {
                dst_id = me;
            }

            // Simulate calculations made by event
           /* for (int i = 0; i <= 1; i++) //1000000000 10000
            {
                continue;
            }
            */
            // Schedule event
            ScheduleNewEvent(dst_id, timestamp, LOOP, NULL, 0);
            break;
    }
}

bool OnGVT(unsigned int me, lp_state_type *snapshot) {

    // Finish condition
    if(((lp_state_type*)snapshot)->gvt_ts>phold_gvt)
    {
        return true;
    }

    return false;
}
