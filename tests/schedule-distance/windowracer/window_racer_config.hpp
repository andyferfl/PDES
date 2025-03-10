#pragma once

#define DETAILED_STATS

// algorithm tuneables
const double initial_tau = 1e-4;
const double tau_growth_factor = 100.0;
const bool mtb_mode = false;
const bool s3a_mode = false;
const int max_num_prev_states = 10;

// debugging
const bool print_debug = false;

const int random_seed = 1;

// hardware parameters
const bool set_affinity = true;
const int cpu_stride = 1;

const uint64_t num_threads = 6;

const uint64_t num_entities = 6;

const int phold_events_per_entity = 1;
const double phold_lambda = 1;
const double phold_lookahead = 10;
const double phold_remote_ratio = 1;
const double phold_zero_delay_ratio = 0;
