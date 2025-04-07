#ifndef SEQUENTIAL_ENGINE_HPP
#define SEQUENTIAL_ENGINE_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <queue>
#include "core/simulation_engine.hpp"
#include "core/logical_process.hpp"

namespace fusion
{
class SequentialEngine : public SimulationEngine
{
public:
    explicit SequentialEngine(const SimulationConfig& config);
    void registerEntity(std::shared_ptr<Entity> entity) override;
    void initialize() override;
    SimulationStats run() override;
    double getCurrentTime() const override;

private:
    std::priority_queue<Event, std::vector<Event>, EventComparator> event_queue_;
    std::unordered_map<uint64_t, std::shared_ptr<Entity>> entities_;
    double current_time_ = 0.0;
};

}

#endif
