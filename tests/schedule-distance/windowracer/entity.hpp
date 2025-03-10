#pragma once

#include <atomic>
#include <cfloat>
#include <assert.h>
#include <pthread.h>
#include "event.hpp"
#include "helper.hpp"
#include "phold.hpp"

extern vector<Event> generated_events[num_threads];

using namespace std;

class Entity;

extern atomic<double> window_ub[2]; // declared in window_racer.cpp
extern pthread_mutex_t *mut;
extern vector<Event> *ev_list;
extern vector<int> num_saved_states;
extern Entity *ent;
extern Entity **prev_ent;
extern uint64_t stats_num_ent_rollbacks_immediate[num_threads];
extern uint64_t stats_num_ev_rollbacks_immediate[num_threads];
extern vector<Entity *> dirty_entities[num_threads][num_threads];

class Entity {

public:

int id;
struct PHoldEntity phold_ent;
double latest_change_ts;

bool save_state()
{
  if (num_saved_states[id] == max_num_prev_states)
    return false;

  printf_debug("entity %d saving state at index %d\n", id, num_saved_states[id]);

  num_saved_states[id]++;

  assert(num_saved_states[id] >= 0 && num_saved_states[id] <= max_num_prev_states);

  prev_ent[id][num_saved_states[id] - 1] = *this;

  return true;
}

double restore_state(double ts, int tid)
{
  double earliest_change_ts = latest_change_ts;

#ifdef DETAILED_STATS
  stats_num_ent_rollbacks_immediate[tid]++;
  stats_num_ev_rollbacks_immediate[tid]++;
#endif


  for (; prev_ent[id][num_saved_states[id] - 1].latest_change_ts > ts; num_saved_states[id]--) {
    earliest_change_ts = prev_ent[id][num_saved_states[id] - 1].latest_change_ts;
#ifdef DETAILED_STATS
    stats_num_ev_rollbacks_immediate[tid]++;
#endif
  }
  printf_debug("entity %d restoring state at %.5g < %.5g, index %d, id %d\n", id, prev_ent[id][num_saved_states[id] - 1].latest_change_ts, ts, num_saved_states[id], prev_ent[id][num_saved_states[id] - 1].id);
  assert(num_saved_states[id] >= 0 && num_saved_states[id] <= max_num_prev_states);
  *this = prev_ent[id][num_saved_states[id] - 1];
  printf_debug("entity %d: latest_change_ts is now %.5g\n", id, latest_change_ts);
  return earliest_change_ts;
}

bool register_event(const Event &ev, double ts, int tid, bool window_lsb)
{
  pthread_mutex_lock(&mut[id]);

  if (num_saved_states[id] == 0)
    dirty_entities[tid][id_to_tid(id)].push_back(this);

  if (ts != ev.creation_ts)
    ev_list[id].push_back(ev);
  
  printf_debug("entity %d pushed to its ev_list\n", id);

  if(!check_bound(ts, tid, window_lsb, ev.creation_ts)) {
    unlock();

    return false;
  }

  return true; // unlock is responsibility of the caller
}

void unlock()
{
  printf_debug("entity %d being unlocked\n", id);
  pthread_mutex_unlock(&mut[id]);
}

bool check_bound(double ts, int tid, bool window_lsb, double creation_ts)
{
  if (ts >= window_ub[window_lsb])
    return false;

  // in Minimum Time Buckets (and the slightly less efficient Breathing Time Bucket), the event horizon
  // ends at the first event at a remote thread
  if (mtb_mode && tid != id_to_tid(id)) {
    atomic_min(window_ub[window_lsb], ts);
    return false;
  }

  // in S3A, we allow zero-delay events to be executed across thread boundaries.
  // this is a generous interpretation of S3A, since NRM only schedule a new event for the current entity, which
  // is excluded from the window since there is only one state saving slot. 
  // phold may schedule an event targeting any entity and we allow these events to be included in the window, as
  // long as the target entity is assigned to the local thread.
  if (s3a_mode && tid != id_to_tid(id) && ts != nextafter(creation_ts, DBL_MAX)) {
    atomic_min(window_ub[window_lsb], ts);
    return false;
  }

  if (ts < latest_change_ts) {
    if (latest_change_ts >= 0.0) {
      printf_debug("entity %d received event at %.5g, latest_change_ts is %.5g, calling restore_state()\n", id, ts, latest_change_ts);
      double earliest_displaced_event_ts = restore_state(ts, tid);

      for (auto it = ev_list[id].begin(); it != ev_list[id].end(); it++) {
        printf_debug("tid %d, ev_list of %d: %.5g, %ld entries in total\n", tid, id, it->ts, ev_list[id].size());
      }

      atomic_min(window_ub[window_lsb], earliest_displaced_event_ts);

      latest_change_ts = ts;

      return true;
    }
  }

  bool r = save_state();

  //if (num_saved_states[id] > 2) {
  //  for (int i = 0; i < num_saved_states[id]; i++) {
  //    printf_debug("entity %d's state list now looks like this: %d, %.5g\n", id, i, prev_ent[id][i].latest_change_ts);
  //  }
  //}

  if (!r) {
    atomic_min(window_ub[window_lsb], ts); // we cannot commit the newly arrived event
    return false;
  }

  latest_change_ts = ts;

  return true;
}

void init(int tid, int id)
{
  this->id = id;

  pthread_mutex_init(&mut[id], NULL);

  latest_change_ts = -1.0;
  num_saved_states[id] = 0;

  phold_ent.init(tid, id);
}

void handle(int tid, double ts)
{
  phold_ent.handle(tid, ts);
}

};
