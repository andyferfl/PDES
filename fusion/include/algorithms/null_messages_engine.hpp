#ifndef NULL_MESSAGES_ENGINE_HPP
#define NULL_MESSAGES_ENGINE_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <queue>
#include "core/entity.hpp"
#include "core/simulation_engine.hpp"
#include "core/logical_process.hpp"

namespace fusion
{
class NullMessagesEngine : public SimulationEngine
{
public:
    /**
     * @brief Construct a new Null Messages Engine object
     * 
     * @param config Configuration for the simulation
     */
    explicit NullMessagesEngine(const SimulationConfig& config);
    ~NullMessagesEngine();

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
     * the stop condition depends on the time limit set in the configurations 
     * and the events left in the event queue, if the time limit is reached 
     * or there are no more events in all lp's then the simulation stops
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
    class NullMessagesLP;
    std::vector<std::unique_ptr<NullMessagesLP>> logical_processes_;
    std::vector<std::thread> worker_threads_;

    void sendNullMessages(uint32_t lp_id);
};
}

#endif
