#pragma once

#include <array>
#include <random>

class Entity;

class Event {

public:
double ts;
int dst_id;
double creation_ts;

static void schedule(int tid, int id, double now, double ts, int dst_id);
void handle_(int tid, bool window_lsb) const;

Event(double ts, size_t dst_id, double creation_ts) : ts(ts), dst_id(dst_id), creation_ts(creation_ts) {}

};

class EventCmp {
public:
  inline bool operator() (const Event &a, const Event &b) {
      return a.ts > b.ts; // || (a->ts == b->ts && a->dst->id > b->dst->id);
  }
};
