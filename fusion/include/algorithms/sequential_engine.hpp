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
    /**
     * @brief Construct a new Sequential Engine object
     * 
     * @param config  Configuration for the simulation
     */
    explicit SequentialEngine(const SimulationConfig& config);

    /**
     * @brief Register an entity to the simulation
     * 
     * @param entity Entity to register
     */
    void registerEntity(std::shared_ptr<Entity> entity) override;

    /**
     * @brief Initialize the simulation
     * 
     */
    void initialize() override;

    /**
     * @brief Run the simulation until the stop conditions are met.
     * 
     * the stop condition depends on the time limit set in the configurations, 
     * if the time limit is reached no more events will be generated which will lead for the simulation to stop
     * 
     * @return SimulationStats Statistics about the simulation run
     */
    SimulationStats run() override;

    /**
     * @brief Get the Current simulation time
     * 
     * @return double The current simulation time
     */
    double getCurrentTime() const override;

private:
    std::priority_queue<Event, std::vector<Event>, EventComparator> event_queue_;
    std::unordered_map<uint64_t, std::shared_ptr<Entity>> entities_;
    double current_time_ = 0.0;
};

}

#endif
