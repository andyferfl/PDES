//time_warp_engine.hpp
#ifndef TIME_WARP_ENGINE_HPP
#define TIME_WARP_ENGINE_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <queue>
#include "core/entity.hpp"
#include "core/simulation_engine.hpp"
#include "core/logical_process.hpp"
#include <map>
#include "../../examples/phold/phold.hpp"  
constexpr uint32_t INIT_EVENT = 0;


namespace fusion
{
class TimeWarpEngine : public SimulationEngine 
{
public:
    /**
     * @brief Construct a new Time Warp Engine object
     * 
     * @param config Configuration for the simulation
     */
    explicit TimeWarpEngine(const SimulationConfig& config);
    
    /**
     * @brief Register an entity with the simulation
     * 
     * @param entity The entity to register
     */
    void registerEntity(std::shared_ptr<Entity> entity) override;
    
    /**
     * @brief Initialize the simulation
     */
    void initialize() override;
    
    /**
     * @brief Run the simulation until the end time
     * 
     * @return SimulationStats Statistics about the simulation run
     */
    SimulationStats run() override;
    
    /**
     * @brief Get the current simulation time
     * 
     * @return double The current simulation time
     */
    double getCurrentTime() const override;
    uint32_t getAssignedLpForEntity(uint64_t entity_id); 
    
private:
    //class TimeWarpLP;
    class TimeWarpLP : public LogicalProcess {
        public:
            TimeWarpLP(uint32_t id, uint32_t max_states_saved, bool lazy_cancellation, TimeWarpEngine* engine);

            void dumpEventQueue();

            void scheduleEvent(const Event& event);
            
            void setEngine(TimeWarpEngine* engine) {
                engine_ = engine;
            }

            bool processNextEvent() override;
            void deliverRemoteEvents(std::vector<TimeWarpLP*>& all_lps);  
            void rollback(double rollback_time);
            void cancelEvent(const Event& anti_message);
            void saveEntityState(std::shared_ptr<Entity> entity, double time);
            void fossilCollect(double gvt);
            
            uint32_t getProcessedEvents() const;
            uint32_t getRollbacks() const;
            uint32_t getGeneratedEvents() const;
            bool eventQueueEmpty() const;
            double peekNextEventTime() const;
            const auto& getPendingRemoteEvents() const;
            size_t getEventQueueSize() const;
        
        private:
            TimeWarpEngine* engine_;  
            mutable std::mutex queue_mutex_; 
            mutable std::mutex pending_events_mutex_;

            uint32_t max_states_saved_;
            bool lazy_cancellation_;
            std::priority_queue<Event, std::vector<Event>, EventComparator> event_queue_;
            std::map<double, std::map<uint32_t, Entity*>> saved_states_;
            std::vector<Event> processed_event_history_;
            std::vector<Event> output_event_history_;
            std::map<uint32_t, std::vector<Event>> pending_remote_events_;
            uint32_t processed_events_ = 0;
            uint32_t rollbacks_ = 0;

        
    };
    
    struct EntityInfo {
        uint32_t lp_id;
        std::shared_ptr<Entity> entity;
    };
    std::unordered_map<uint64_t, EntityInfo> entity_registry_;


    std::vector<std::unique_ptr<TimeWarpLP>> logical_processes_;
    std::vector<std::thread> worker_threads_;
    std::atomic<double> global_virtual_time_ = 0.0;
    std::unordered_map<uint64_t, uint32_t> entity_to_lp_; 
    void calculateGVT();
    void performFossilCollection(double gvt);
};

} 
#endif