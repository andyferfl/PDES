#include <vector>
#include <random>
#include <assert.h>
#include <sys/types.h>
#include <queue>
#include "entity.hpp"

extern Entity *ent;
extern vector<Entity *> dirty_entities[num_threads][num_threads];
extern vector<Event> generated_events[num_threads];
extern uint64_t stats_num_handle_calls[num_threads];
extern uint64_t stats_num_remote_handle_calls[num_threads];
extern uint64_t stats_num_instantaneous_events[num_threads];
extern double stats_mean_event_delay[num_threads];
extern priority_queue<Event, vector<Event>, EventCmp> new_ev_q[num_threads];
extern uint64_t stats_max_new_ev_queue_size[num_threads];
extern uint64_t num_committed_total;

using namespace std;



void Event::schedule(int tid, int src_id, double now, double ts, int dst_id)
{
  assert(dst_id < num_entities);
  Event(ts, dst_id, now);

  printf_debug("tid %d, entity %d: it's %.50f, scheduling a new event at %.50f for entity %d\n", tid, src_id, now, ts, dst_id);
  
  if (num_threads == 1 && ts == now) {
    printf_debug("tid %d immediately executing zero-delay event\n");
    ent[dst_id].handle(tid, ts);
    num_committed_total++;
  //} else if (ts == now || now == 0.0) {
  } else if (ts == now || now == 0.0) {
    printf_debug("tid %d emplacing into generated_events, which has size %ld\n", tid, generated_events[tid].size());
    generated_events[tid].emplace_back(ts, dst_id, now);
    printf_debug("tid %d done emplacing into generated_events\n", tid, generated_events[tid].size());
  } else {
    new_ev_q[tid].emplace(ts, dst_id, now);
#ifdef DETAILED_STATS
    stats_max_new_ev_queue_size[tid] = max(stats_max_new_ev_queue_size[tid], new_ev_q[tid].size());
#endif
  }

  assert(tid >= 0 && tid < (int)num_threads);
}

void Event::handle_(int tid, bool window_lsb) const
{
  printf_debug("tid %d, entity %d: handle_() was called for ev at %.5g\n", tid, dst_id, ts);

  int generated_events_start_idx = generated_events[tid].size();

  printf_debug("tid %d trying to lock ent %d\n", tid, dst_id);
  if (ent[dst_id].register_event(*this, ts, tid, window_lsb)) {
    printf_debug("tid %d locked ent %d\n", tid, dst_id);
    printf_debug("tid %d, entity %d: calling handle() for ev at %.5g\n", tid, dst_id, ts);
#ifdef DETAILED_STATS
    stats_num_handle_calls[tid]++;
    if (tid != id_to_tid(dst_id))
      stats_num_remote_handle_calls[tid]++;

    double delay = ts - creation_ts;
    if (delay == 0.0)
      stats_num_instantaneous_events[tid]++;
#endif
    ent[dst_id].handle(tid, ts);
    printf_debug("tid %d unlocking %d\n", tid, dst_id);
    //assert(dst_id >= 0 && dst_id < num_entities);
    ent[dst_id].unlock();
  } else {
    return;
  }

  int generated_events_end_idx = generated_events[tid].size();

  for (int i = generated_events_start_idx; i < generated_events_end_idx; i++) {
    const Event &new_ev = generated_events[tid][i];
    //printf_debug("new_ev looks like this: %.5g, %d\n", new_ev->ts, new_ev->dst->id);
    printf_debug("tid %d, entity %d: it's %.5g here, handling new ev at entity %d at %.5g, appending %d to dirty_entities[%d][%d]\n", tid, dst_id, ts, new_ev.dst_id, new_ev.ts, new_ev.dst_id, tid, id_to_tid(new_ev.dst_id));
    //dirty_entities[tid][id_to_tid(new_ev.dst_id)].push_back(&ent[new_ev.dst_id]);
    new_ev.handle_(tid, window_lsb);
  }
}
