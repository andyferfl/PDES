#ifndef SIMULATION_ENGINE_HPP
#define SIMULATION_ENGINE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <stdexcept>
#include "event.hpp"
#include "entity.hpp"
#include "logical_process.hpp"

namespace fusion
{

enum class SimulationAlgorithm
{
    SEQUENTIAL,     // Sequential DES algorithm
    WINDOW_RACER,   // Window racer algorithm
    TIME_WARP,      // Time Warp algorithm
    NULL_MESSAGES   // Null messages algorithm
};

struct SimulationConfig
{
    SimulationAlgorithm algorithm = SimulationAlgorithm::SEQUENTIAL;
    uint32_t num_threads = 1;
    uint32_t num_logical_processes = 1;
    double end_time = 1000.0;
    bool collect_detailed_stats = false; 
    
    struct
    {
        double initial_window_size = 1e-4;
        double window_growth_factor = 100.0;
        int max_states_saved = 10;
    } window_racer;

    struct
    {
        double gvt_interval = 10;
        bool lazy_cancellation = false;
        int max_states_saved = 10;
    } time_warp;

    struct
    {
        double loookahead = 0.1;
        bool dynamic_lookahead = false;
    } null_messages;    
};

struct SimulationStats
{
    double simulation_time = 0.0;
    double wall_clock_time = 0.0;
    uint64_t total_events_processed = 0;
    uint64_t total_events_generated = 0;
    uint64_t total_rollbacks = 0;
    uint64_t total_null_messages = 0;
    double efficiency = 0.0;

    std::vector<uint64_t> events_processed_per_lp;
    std::vector<uint64_t> rollbacks_per_lp;
    std::vector<uint64_t> null_messages_per_lp;

    struct
    {
        std::vector<double> window_sizes;
        uint64_t immediate_rollbacks = 0;
        uint64_t window_end_rollbacks = 0;
    } window_racer;

    struct
    {
        uint64_t gvt_calculations = 0;
        uint64_t fossil_collections = 0;
        uint64_t anti_messages = 0;
    } time_warp;

    struct
    {
        uint64_t blocked_time = 0;
    } null_messages;   
};

class SimulationEngine
{
public:
    explicit SimulationEngine(const SimulationConfig& config);
    virtual ~SimulationEngine() = default;
    virtual void registerEntity(std::shared_ptr<Entity> entity) = 0;
    virtual void initialize() = 0;
    virtual SimulationStats run() = 0;
    virtual double getCurrentTime() const = 0;
    const SimulationConfig& getConfig() const;
    static std::unique_ptr<SimulationEngine> create(const SimulationConfig& config);
protected:
    SimulationConfig config_;
    SimulationStats stats_;
    std::atomic<bool> running_ = false;
};

}

#endif