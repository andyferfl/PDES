#pragma once

#include <stddef.h>
#include "event.hpp"
#include "XoshiroCpp.hpp"
#include "window_racer_config.hpp"

using namespace std;

static exponential_distribution<double> delay_dist(phold_lambda);
static uniform_int_distribution<int> dst_dist(0, num_entities - 2); // truly remote
static uniform_real_distribution<double> uniform_dist;

static const int num_entities_per_thread = num_entities / num_threads;

class PHoldEntity {
public:
  int id;

  __int128 dummy_state[phold_events_per_entity - 1]; // one dummy rng state per entity (minus one to account for the actual RNG state)

  XoshiroCpp::Xoroshiro128StarStar gen;

  void schedule_event(int tid, double now) {
    double new_ts;

    int dst_id = id;

    new_ts = now + phold_lookahead + delay_dist(gen);

    if (uniform_dist(gen) < phold_remote_ratio)
      dst_id = (id + dst_dist(gen) + 1) % num_entities;
    
    Event::schedule(tid, id, now, new_ts, dst_id);
  }

  void init(int tid, int id) {
    this->id = id;
    gen.seed(random_seed ^ id);

    for (int i = 0; i < phold_events_per_entity; i++) {
      double new_ts = phold_lookahead + delay_dist(gen);
      Event::schedule(tid, id, 0.0, new_ts, id);
    }

  }

  void handle(int tid, double ts) {
    schedule_event(tid, ts);
  }
};
