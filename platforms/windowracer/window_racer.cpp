#include <float.h>
#include <stdint.h>

#include "entity.hpp"
#include "event.hpp"
#include "phold.hpp"
#include "window_racer_config.hpp"

#include <algorithm>
#include <atomic>
#include <climits>
#include <float.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stdio.h>
#include <thread>
#include <unistd.h>

#include <assert.h>

using namespace std;

pthread_barrier_t barrier;

Entity *ent, **prev_ent;
vector<Event> *ev_list;
pthread_mutex_t *mut;
vector<int> num_saved_states;

priority_queue<Event, vector<Event>, EventCmp> q[num_threads];
priority_queue<Event, vector<Event>, EventCmp> new_ev_q[num_threads];

vector<Entity *> dirty_entities[num_threads][num_threads];
vector<Event> new_ev_list[num_threads][num_threads];
vector<Event> initial_events[num_threads][num_threads];

atomic<double> window_lb[2];
atomic<double> window_ub[2];

vector<Event> generated_events[num_threads];

#ifdef DETAILED_STATS
uint64_t stats_num_ent_rollbacks_immediate[num_threads];
uint64_t stats_num_ev_rollbacks_immediate[num_threads];

uint64_t stats_num_ent_rollbacks_at_window_end[num_threads];
uint64_t stats_num_ev_rollbacks_at_window_end[num_threads];

vector<double> stats_initial_window_sizes;
vector<double> stats_final_window_sizes;
uint64_t stats_num_handle_calls[num_threads];
uint64_t stats_num_remote_handle_calls[num_threads];

uint64_t stats_num_instantaneous_events[num_threads];

uint64_t stats_num_windows;

uint64_t stats_max_queue_size[num_threads];
uint64_t stats_max_new_ev_queue_size[num_threads];
#endif

struct thread_params {
  int tid;
  int num_committed;
  int num_iterations;
  double sim_endtime;
};

uint64_t num_committed_total = 0;

void *run_thread(void *p) {
  thread_params *args = (thread_params *)p;
  int tid = args->tid;
  uint64_t num_committed_thread_total = 0;

  for (uint64_t i = 0; i < num_entities; i++) {

    if (id_to_tid(i) != tid)
      continue;

    prev_ent[i] = new Entity[max_num_prev_states];

    printf_debug("initializing entity %d\n", i);

    ent[i].init(tid, i);
    assert(ent[i].id == i);

    for (auto it = generated_events[tid].begin();
         it != generated_events[tid].end(); it++) {
      // Event & ev = *it;
      int dst_tid = id_to_tid(it->dst_id);
      printf_debug("pushing event for entity %d at time %.5g, tid %d\n",
                   it->dst_id, it->ts, dst_tid);
      initial_events[tid][dst_tid].push_back(*it);
    }

    generated_events[tid].clear();
  }

  pthread_barrier_wait(&barrier);

  for (uint64_t src_tid = 0; src_tid < num_threads; src_tid++) {
    for (auto it = initial_events[src_tid][tid].begin();
         it != initial_events[src_tid][tid].end(); it++) {
      q[tid].push(*it);
#ifdef DETAILED_STATS
      stats_max_queue_size[tid] = max(stats_max_queue_size[tid], q[tid].size());
#endif
    }
    initial_events[src_tid][tid].clear();
  }

  int iter = 0;
  double tau = initial_tau;

  double thread_now = 0.0;

  double final_window_ub = 0.0;

  bool window_lsb = 0;

  if (!tid)
    window_ub[window_lsb] = window_lb[window_lsb] = args->sim_endtime;

  // SYNC for init of window_ub and window_lb
  pthread_barrier_wait(&barrier);

  while (true) {

    thread_now = q[tid].empty() ? DBL_MAX : q[tid].top().ts;

    printf_debug("tid %d, thread_now is %.5g, %lu events in queue\n", tid,
                 thread_now, q[tid].size());

    assert(thread_now >= 0.0 || !iter);

    // printf_debug("tid %d entering barrier A\n", tid);

    atomic_min(window_ub[window_lsb], thread_now + tau);
    atomic_min(window_lb[window_lsb], thread_now);

    // init of next round
    if (!tid)
      window_ub[1 - window_lsb] = window_lb[1 - window_lsb] = args->sim_endtime;

    printf_debug("tid %d: iteration %d, thread_now is %.5g, tau is %.5g\n", tid,
                 iter, thread_now, tau);
    printf_debug("tid %d entering barrier B\n", tid);

    // SYNC for window_ub
    pthread_barrier_wait(&barrier);

    if (window_lb[window_lsb] >= args->sim_endtime) {
      break;
    }

#ifdef DETAILED_STATS
    if (!tid)
      stats_initial_window_sizes.push_back(tau);
#endif

    printf_debug("tid %d: iteration %d, thread_now is %.5g, window_lb is %.5g, "
                 "window_ub is %.5g, tau is %.5g\n",
                 tid, iter, thread_now, (double)window_lb[window_lsb],
                 (double)window_ub[window_lsb], tau);

    assert(window_ub[window_lsb] >= 0.0);
    assert(window_lb[window_lsb] <= thread_now);

    printf_debug("tid %d: first event is at %.5g, got %ld in queue\n", tid,
                 q[tid].empty() ? DBL_MAX : q[tid].top().ts, q[tid].size());

    while (true) {

      const Event *ev_a = NULL;

      if (!q[tid].empty() && q[tid].top().ts < window_ub[window_lsb])
        ev_a = &q[tid].top();

      const Event *ev_b = NULL;
      if (!new_ev_q[tid].empty() &&
          new_ev_q[tid].top().ts < window_ub[window_lsb])
        ev_b = &new_ev_q[tid].top();

      Event exec_ev = Event(0, 0, 0);
      bool exec_ev_was_set = false;
      if (ev_a != NULL && (ev_b == NULL || ev_a->ts < ev_b->ts)) {
        exec_ev = *ev_a;
        exec_ev_was_set = true;
        q[tid].pop();
      } else if (ev_b != NULL) {
        exec_ev = *ev_b;
        exec_ev_was_set = true;
        new_ev_q[tid].pop();
      }

      if (!exec_ev_was_set) {
        printf_debug("tid %d: no events left to handle, ts of tops: %.5g and "
                     "%.5g, window_ub is %.5g\n",
                     tid, q[tid].empty() ? DBL_MAX : q[tid].top().ts,
                     new_ev_q[tid].empty() ? DBL_MAX : new_ev_q[tid].top().ts,
                     (double)window_ub[window_lsb]);
        break;
      }

      printf_debug("tid %d: handling event from queue, entity %d, ts %.5g\n",
                   tid, exec_ev.dst_id, exec_ev.ts);

      // assert(id_to_tid(ev.dst_id) == tid);

      //dirty_entities[tid][id_to_tid(exec_ev.dst_id)].push_back(
      //    &ent[exec_ev.dst_id]);
      printf_debug("tid %d appending %d to dirty_entities[%d][%d]\n", tid,
                   exec_ev.dst_id, tid, tid);
      exec_ev.handle_(tid, window_lsb);
    }

    vector<Event>& new_ev_q_container = get_container(new_ev_q[tid]);
    for (auto& ev: new_ev_q_container) {
      new_ev_list[tid][id_to_tid(ev.dst_id)].push_back(ev);
    }
    //new_ev_q_container.clear();
    new_ev_q[tid] = priority_queue<Event, vector<Event>, EventCmp>();

    printf_debug("tid %d entering barrier C\n", tid);
    // SYNC to protect entities and window_ub
    pthread_barrier_wait(&barrier);
    final_window_ub = window_ub[window_lsb];

    int num_committed = 0;

    for (uint64_t src_tid = 0; src_tid < num_threads; src_tid++) {

      for (auto ev_it = new_ev_list[src_tid][tid].begin();
           ev_it != new_ev_list[src_tid][tid].end(); ev_it++) {
        printf_debug("tid %d pushing ev from new_ev_list at %.50g to q\n", tid,
                     (*ev_it).ts);
        assert((*ev_it).ts >= final_window_ub);
        if ((*ev_it).creation_ts < final_window_ub) {
          q[tid].push(*ev_it);
#ifdef DETAILED_STATS
          stats_max_queue_size[tid] =
              max(stats_max_queue_size[tid], q[tid].size());
#endif
        }
      }
      new_ev_list[src_tid][tid].clear();

      for (auto ent_it = dirty_entities[src_tid][tid].begin();
           ent_it != dirty_entities[src_tid][tid].end(); ent_it++) {
        Entity *ent = *ent_it;
        vector<Event> &ent_ev_list = ev_list[ent->id];

        printf_debug("tid %d: found local dirty entity %d in "
                     "dirty_entities[%d][%d], ev_list has %d entries\n",
                     tid, ent->id, src_tid, tid, ent_ev_list.size());

        for (auto ev_it = ent_ev_list.begin(); ev_it != ent_ev_list.end();
             ev_it++) {
          Event &ev = *ev_it;

          printf_debug("tid %d: checking event at ent %d at %.5g, "
                       "final_window_ub is %.5g\n",
                       tid, ev.dst_id, ev.ts, final_window_ub);

          if (ev.creation_ts >=
              final_window_ub) { // creating event has been excluded
            printf_debug("tid %d: creator of event (which was at %.5g) at ent "
                         "%lu at %.5g has been excluded\n",
                         tid, ev.creation_ts, ev.dst_id, ev.ts);
            continue;
          }

          if (ev.ts < final_window_ub) { // event can be committed
            num_committed++;
            continue;
          }

          if (ev.ts >=
              final_window_ub) { // event creation has been committed, but event
                                 // itself is postponed to a later round
            printf_debug("tid %d: event at ent %lu at %.5g has been created by "
                         "a newly committable event, but is excluded, top of "
                         "queue is now %.5g, entity %lu\n",
                         tid, ev.dst_id, ev.ts,
                         q[tid].empty() ? -1.0 : q[tid].top().ts,
                         q[tid].empty() ? -1 : q[tid].top().dst_id);
            assert(ev.dst_id == ent->id);
            q[tid].push(ev);
#ifdef DETAILED_STATS
            stats_max_queue_size[tid] =
                max(stats_max_queue_size[tid], q[tid].size());
#endif
            printf_debug("tid %d: next thread_now should be %.5g\n", tid,
                         q[tid].empty() ? -1.0 : q[tid].top().ts);
            continue;
          }

          printf_debug("tid %d: BUG: event at ent %d at %.5g, created at %.5g, "
                       "final_window_ub is %.5g\n",
                       tid, ev.dst_id, ev.ts, ev.creation_ts, final_window_ub);
          // assert(false);
        }

        if (ent->latest_change_ts < final_window_ub) {
          // committing means doing nothing here
        } else {
          int i = num_saved_states[ent->id];
          printf_debug("entity %d has %d saved states, latest change is %.5g\n",
                       ent->id, i, ent->latest_change_ts);

#ifdef DETAILED_STATS
          stats_num_ent_rollbacks_at_window_end[tid]++;
          stats_num_ev_rollbacks_at_window_end[tid]++;
#endif

          for (; prev_ent[ent->id][i - 1].latest_change_ts >= final_window_ub;
               i--) {
            printf_debug("entity %d: can't commit state index %d, time %.5g, "
                         "final_window_ub is %.5g\n",
                         ent->id, i, prev_ent[ent->id][i - 1].latest_change_ts,
                         final_window_ub);
#ifdef DETAILED_STATS
            stats_num_ev_rollbacks_at_window_end[tid]++;
#endif
          }

          assert(i >= 0 && i <= max_num_prev_states);

          printf_debug("entity %d: committing state index %d\n", ent->id,
                       i - 1);

          *ent = prev_ent[ent->id][i - 1];
        }
        ent->latest_change_ts = -1.0;
        ev_list[ent->id].clear();
        num_saved_states[ent->id] = 0;

        printf_debug("entity %d\n", ent->id);

        assert(ent->id < num_entities);
      }
    }

    // printf_debug("tid %d: %d events committed this round\n", tid,
    // num_committed);
    printf_debug("tid %d: %d events committed in iteration %d, tau was %.5g, "
                 "window_ub was %.5g, final_window_ub: %.5g\n",
                 tid, num_committed, iter, (double)window_lb[window_lsb], tau,
                 final_window_ub);

    // XXX: doesn't have to be an issue, as long as non-zero events are committed per round
    //if (final_window_ub <= window_lb[window_lsb]) {
    //  printf("final_window_ub: %.5g, window_lb %.5g\n", final_window_ub,
    //         (double)window_lb[window_lsb]);
    //  exit(1);
    //}

    assert(final_window_ub > window_lb[window_lsb]);
    double hindsight_optimal_tau = final_window_ub - window_lb[window_lsb];
#ifdef DETAILED_STATS
    if (!tid) {
      stats_num_windows++;
      stats_final_window_sizes.push_back(hindsight_optimal_tau);
    }
#endif
    assert(hindsight_optimal_tau > 0.);
    tau = hindsight_optimal_tau * tau_growth_factor;

    printf_debug("hindsight_optimal_tau is %.5g, tau is now %.5g\n",
                 hindsight_optimal_tau, tau);

    pthread_barrier_wait(&barrier);

    assert(tau > 0.0);

    num_committed_thread_total += num_committed;

    num_committed = 0;
    iter++;

    for (uint64_t src_tid = 0; src_tid < num_threads; src_tid++)
      dirty_entities[src_tid][tid].clear();

    window_lsb = 1 - window_lsb;
  }

  printf_debug("tid %d: returning, committed %d events\n", tid,
               num_committed_thread_total);

  args->num_committed = num_committed_thread_total;
  args->num_iterations = iter + 1;

  return NULL;
}

#define PRINT_STAT(S)                                                          \
  fprintf(f, "    " #S ":\n");                                                 \
  for (int tid = 0; tid < num_threads; tid++)                                  \
    fprintf(f, "        %d: %.3g\n", tid, (double)S[tid]);                     \
  fprintf(f, "\n");

#define COMPUTE_MEAN(S)                                                        \
  double mean_##S = 0;                                                         \
  for (int tid = 0; tid < num_threads; tid++)                                  \
  mean_##S += S[tid] / num_threads

#define PRINT_MEAN(S) fprintf(f, "    " #S ": %.3g\n", mean_##S);

#ifdef DETAILED_STATS
void dump_stats() {
  COMPUTE_MEAN(stats_num_ent_rollbacks_immediate);
  COMPUTE_MEAN(stats_num_ev_rollbacks_immediate);

  COMPUTE_MEAN(stats_num_ent_rollbacks_at_window_end);
  COMPUTE_MEAN(stats_num_ev_rollbacks_at_window_end);

  COMPUTE_MEAN(stats_num_handle_calls);
  COMPUTE_MEAN(stats_num_remote_handle_calls);

  COMPUTE_MEAN(stats_num_instantaneous_events);

  FILE *f = fopen("window_racer_stats.txt", "w");
  fprintf(f, "--- SUMMARY STATS ---\n\n");
  fprintf(f, "  num_windows: %lu\n", stats_num_windows);
  double mean_initial_window_size = 0;
  double mean_final_window_size = 0;
  double mean_final_vs_initial_window_size = 0;
  assert(stats_num_windows == stats_initial_window_sizes.size() &&
         stats_num_windows == stats_final_window_sizes.size());
  for (size_t i = 0; i < stats_initial_window_sizes.size(); i++) {
    mean_initial_window_size +=
        stats_initial_window_sizes[i] / stats_num_windows;
    mean_final_window_size += stats_final_window_sizes[i] / stats_num_windows;
    mean_final_vs_initial_window_size +=
        (stats_final_window_sizes[i] / stats_initial_window_sizes[i]) /
        stats_num_windows;
  }
  fprintf(f, "\n  mean initial window size: %.3g\n\n",
          mean_initial_window_size);
  fprintf(f, "  mean final window size: %.3g\n\n", mean_final_window_size);
  fprintf(f, "  mean final/initial window size: %.3g\n\n",
          mean_final_vs_initial_window_size);

  double mean_total_num_ev_rollbacks =
      mean_stats_num_ev_rollbacks_immediate +
      mean_stats_num_ev_rollbacks_at_window_end;
  double mean_total_num_ent_rollbacks =
      mean_stats_num_ent_rollbacks_immediate +
      mean_stats_num_ent_rollbacks_at_window_end;
  fprintf(f, "  num handle calls: %.3g\n\n", mean_stats_num_handle_calls * num_threads);

  fprintf(f, "  event rollbacks/handle: %.3g\n\n",
          mean_total_num_ev_rollbacks / mean_stats_num_handle_calls);
  fprintf(f, "  entity rollbacks/handle: %.3g\n\n",
          mean_total_num_ent_rollbacks / mean_stats_num_handle_calls);
  fprintf(f, "  mean rollback length: %.3g\n\n",
          mean_total_num_ev_rollbacks / mean_total_num_ent_rollbacks);
  fprintf(f, "  proportion remote handle calls: %.3g\n\n",
          mean_stats_num_remote_handle_calls / mean_stats_num_handle_calls);
  fprintf(
      f,
      "  proportion instantaneous events among all handle_() calls: %.3g\n\n",
      mean_stats_num_instantaneous_events / mean_stats_num_handle_calls);

  fprintf(f, "--- RAW STATS ---\n\n");
  fprintf(f, "  MEANS ACROSS THREADS:\n");
  PRINT_MEAN(stats_num_ent_rollbacks_immediate);
  PRINT_MEAN(stats_num_ev_rollbacks_immediate);

  PRINT_MEAN(stats_num_ent_rollbacks_at_window_end);
  PRINT_MEAN(stats_num_ev_rollbacks_at_window_end);

  PRINT_MEAN(stats_num_handle_calls);
  PRINT_MEAN(stats_num_remote_handle_calls);

  fprintf(f, "\n  PER-THREAD:\n");
  PRINT_STAT(stats_num_ent_rollbacks_immediate);
  PRINT_STAT(stats_num_ev_rollbacks_immediate);

  PRINT_STAT(stats_num_ent_rollbacks_at_window_end);
  PRINT_STAT(stats_num_ev_rollbacks_at_window_end);

  PRINT_STAT(stats_num_handle_calls);
  PRINT_STAT(stats_num_remote_handle_calls);

  PRINT_STAT(stats_max_queue_size);

  PRINT_STAT(stats_max_new_ev_queue_size);

  PRINT_STAT(stats_num_instantaneous_events);

  fprintf(f, "\n  initial and final window sizes: ");
  for (size_t i = 0; i < stats_initial_window_sizes.size(); i++) {
    fprintf(f, "(%.5g, %.5g) ", stats_initial_window_sizes[i],
            stats_final_window_sizes[i]);
  }
  fprintf(f, "\n");

  fclose(f);
}
#endif

int main(int argc, char **argv) {
  if (num_threads * cpu_stride > std::thread::hardware_concurrency()) {
    std::cout << "more threads (" << num_threads << ") requested than hardware provides ("
              << std::thread::hardware_concurrency() << ")\n";
    return 1;
  }
  const double endtime = std::stod(argv[1]);

  mut = new pthread_mutex_t[num_entities];
  ev_list = new vector<Event>[num_entities];
  for (uint64_t eid = 0; eid < num_entities; eid++)
    ev_list[eid].reserve(20);

  ent = new Entity[num_entities];
  prev_ent = new Entity *[num_entities];

  num_saved_states.resize(num_entities);

  long int start_ms = get_current_time_ms();

  if (num_threads == 1) {
    for (uint64_t i = 0; i < num_entities; i++) {

      prev_ent[i] = new Entity[max_num_prev_states];

      printf_debug("initializing entity %d\n", i);

      ent[i].init(0, i);

      while (!new_ev_q[0].empty()) {
        q[0].push(new_ev_q[0].top());
        new_ev_q[0].pop();
      }

      for (auto it = generated_events[0].begin();
           it != generated_events[0].end(); it++) {
        int tid = id_to_tid(it->dst_id);
        printf_debug(
            "pushing event for entity %d at time %.5g in tid %d's queue\n",
            it->dst_id, (*it).ts, tid);
        q[tid].push(*it);
      }

      generated_events[0].clear();
    }
  }

  int num_iterations = 0;

  pthread_barrier_init(&barrier, NULL, num_threads);

  pthread_t thread[num_threads];

  thread_params thread_args[num_threads];

  pthread_attr_t attr;
  cpu_set_t cpus;
  pthread_attr_init(&attr);

  if (num_threads > 1) {
    for (uint64_t i = 0; i < num_threads; i++) {
      if (set_affinity) {
        CPU_ZERO(&cpus);
        CPU_SET(i * cpu_stride, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
      }

      thread_args[i].tid = i;
      thread_args[i].num_committed = 0;
      thread_args[i].num_iterations = 0;
      thread_args[i].sim_endtime = endtime;
      pthread_create(&thread[i], &attr, run_thread, &thread_args[i]);
    }

    for (uint64_t i = 0; i < num_threads; i++) {
      pthread_join(thread[i], NULL);
      num_committed_total += thread_args[i].num_committed;
    }
    num_iterations = thread_args[0].num_iterations;
  } else {
    while (!q[0].empty() && q[0].top().ts < endtime) {
      const Event ev = q[0].top();
      q[0].pop();

      printf_debug("handling event at entity %d at time %.5g, queue size %d\n",
                   ev.dst_id, ev.ts, q[0].size());

      ent[ev.dst_id].handle(0, ev.ts);

      while (!new_ev_q[0].empty()) {
        q[0].push(new_ev_q[0].top());
        new_ev_q[0].pop();
      }

      num_committed_total++;
      num_iterations++;
    }
  }

  long total_ms = get_current_time_ms() - start_ms;

  printf("committed %lu events in %.2fs, %.2f Mev/s\n", num_committed_total,
         total_ms / 1e3, (double)num_committed_total / total_ms / 1e3);
  printf("%d windows, %.2f events committed per window\n", num_iterations,
         (double)num_committed_total / num_iterations);

#ifdef DETAILED_STATS
  dump_stats();
#endif
}
