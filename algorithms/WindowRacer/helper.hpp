#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <chrono>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>
#include "window_racer_config.hpp"

using namespace std;

static uint64_t dummy_r;                                                                           
extern uint64_t wait_iter;

inline uint64_t get_ns() {
  struct timespec ts;
  
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return 1e9 * ((uint64_t)ts.tv_sec) + (uint64_t)ts.tv_nsec;
}

inline void wait()
{
  dummy_r = 0;

  for (uint64_t i = 0; i < wait_iter; i++)
    dummy_r += i;
}


// from: https://stackoverflow.com/questions/44800510/updating-maximum-value-atomically
template<typename T>
inline void atomic_min(std::atomic<T> & atom, const T val)
{
  for(T atom_val=atom;
      atom_val > val &&
      !atom.compare_exchange_weak(atom_val, val, std::memory_order_relaxed);
     );
}

inline void printf_debug(const char *format, ...)
{
  if(!print_debug)
    return;

  va_list args;
  va_start(args, format);
  
  vfprintf(stderr, format, args);
  
  va_end(args);
}

inline long int get_current_time_ms()
{
    return chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count(); // such a helpful abstraction
}

inline int id_to_tid(int id)
{
  return min(id / (num_entities / num_threads), num_threads - 1);
}

inline size_t get_rss()
{
    long rss = 0L;
    FILE* fp = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL)
      return (size_t)0L;
    if (fscanf(fp, "%*s%ld", &rss) != 1)
    {
      fclose(fp);
      return (size_t)0L;
    }
    fclose(fp);
    return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE) / 1024 / 1024;
}

template <class ADAPTER>
typename ADAPTER::container_type & get_container (ADAPTER &a)
{
    struct hack : ADAPTER {
        static typename ADAPTER::container_type & get (ADAPTER &a) {
            return a.*&hack::c;
        }
    };
    return hack::get(a);
}

