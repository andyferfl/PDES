#ifndef WINDOW_RACER_ENGINE_HPP
#define WINDOW_RACER_ENGINE_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <queue>
#include "core/entity.hpp"
#include "core/simulation_engine.hpp"
#include "core/logical_process.hpp"

namespace fusion
{
class WindowRacerEngine : public SimulationEngine
{
public:
    /**
     * @brief Construct a new Null Messages Engine object
     * 
     * @param config Configuration for the simulation
     */
    explicit WindowRacerEngine(const SimulationConfig& config);
    ~WindowRacerEngine();

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

protected:
    inline int id_to_tid(int id);
    void handleEvent(Event event, uint32_t thread_id, bool window_lsb);

private:
    class WindowRacerLP;
   
    uint64_t num_entities_ = 0;

    std::vector<std::shared_ptr<WindowRacerLP>> logical_processes_;
    std::vector<std::priority_queue<Event, std::vector<Event>, EventComparator>> event_queues_;
    std::vector<std::priority_queue<Event, std::vector<Event>, EventComparator>> new_event_queues_;
    std::vector<std::vector<std::vector<Event>>> new_events_list_;
    std::vector<std::vector<std::vector<WindowRacerLP *>>> dirty_entities_;

    std::atomic<double> global_virtual_time_ = 0.0;
    std::array<std::atomic<double>,2> window_lb_;
    std::array<std::atomic<double>,2> window_ub_;
    std::vector<std::thread> worker_threads_;
};
}

#endif
