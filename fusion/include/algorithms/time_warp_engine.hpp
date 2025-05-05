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
    uint32_t getNumLogicalProcesses() const { return logical_processes_.size(); }
    uint32_t getAssignedLpForEntity(uint64_t entity_id) const {
        auto it = entity_to_lp_.find(entity_id);
        if (it != entity_to_lp_.end()) {
            return it->second;
        }
        throw std::runtime_error("Entity not registered");
    }

private:
    //class TimeWarpLP;
    class TimeWarpLP : public LogicalProcess {
        public:
            TimeWarpLP(uint32_t id, uint32_t max_states_saved, bool lazy_cancellation, TimeWarpEngine* engine);
            void scheduleEvent(const Event& event) {
                event_queue_.push(event);
            }
            
            bool eventQueueEmpty() const {
                return event_queue_.empty();
            }
            
            double peekNextEventTime() const {
                return event_queue_.empty() ? std::numeric_limits<double>::infinity() 
                                          : event_queue_.top().getTimestamp();
            }
            const auto& getPendingRemoteEvents() const { return pending_remote_events_; }
            size_t getEventQueueSize() const { 
                std::lock_guard<std::mutex> lock(queue_mutex_);
                return event_queue_.size(); 
            }

            void dumpEventQueue();
            
            bool processNextEvent() override;
            void deliverRemoteEvents(std::vector<TimeWarpLP*>& all_lps);  // Add this
            void rollback(double rollback_time);
            void cancelEvent(const Event& anti_message);
            void saveEntityState(std::shared_ptr<Entity> entity, double time);
            void fossilCollect(double gvt);
            
            uint32_t getProcessedEvents() const;  // Add these getters
            uint32_t getRollbacks() const;
            uint32_t getGeneratedEvents() const;
            bool shouldProcessLocally(uint64_t entity_id) const {
                return getEntities().count(entity_id) > 0;
            }
            
            
            
        
        private:
            TimeWarpEngine* engine_;  
            mutable std::mutex queue_mutex_; 
            mutable std::mutex pending_events_mutex_;

            uint32_t max_states_saved_;
            bool lazy_cancellation_;
            // std::atomic<double> local_virtual_time_{0.0};  
            std::priority_queue<Event, std::vector<Event>, EventComparator> event_queue_;
            std::map<double, std::map<uint32_t, std::any>> saved_states_;
            std::vector<Event> processed_event_history_;
            std::vector<Event> output_event_history_;
            std::map<uint32_t, std::vector<Event>> pending_remote_events_;
            uint32_t processed_events_ = 0;
            uint32_t rollbacks_ = 0;

        
    };
    std::vector<uint64_t> getAllEntityIds() const;
    uint32_t getLpForEntity(uint64_t entity_id) const {
        auto it = entity_to_lp_.find(entity_id);
        if (it != entity_to_lp_.end()) {
            return it->second;
        }
        throw std::runtime_error("Entity " + std::to_string(entity_id) + " not found in any LP");
    }
    uint32_t getTargetLpForEntity(uint64_t entity_id, uint32_t total_lps) const {
        return entity_id % total_lps; // Тот же алгоритм, что и в registerEntity
    }
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