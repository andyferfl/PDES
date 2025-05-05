#ifndef LOGICAL_PROCESS_HPP
#define LOGICAL_PROCESS_HPP

#include <cstdint>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include "event.hpp"
#include "entity.hpp"

namespace fusion
{
class LogicalProcess
{
public:
    /**
     * @brief Construct a new Logical Process object
     * 
     * @param id Unique ID for this LP
     */
    explicit LogicalProcess(uint32_t id);

    /**
     * @brief Destroy the Logical Process object
     * 
     */
    virtual ~LogicalProcess() = default;

    /**
     * @brief Get the LP ID
     * 
     * @return uint32_t LP ID
     */
    uint32_t getId() const;

    /**
     * @brief Add an entity to this LP
     * 
     * @param entity Entity to add
     */
    void addEntity(std::shared_ptr<Entity> entity);

    /**
     * @brief Get an entity by ID
     * 
     * @param entity_id Entity ID
     * @return std::shared_ptr<Event> Entity or nullptr if not found
     */
    std::shared_ptr<Entity> getEntity(uint64_t entity_id) const;

    /**
     * @brief Get all entities managed by this LP
     * 
     * @return const std::unordered_map<uint64_t, std::shared_ptr<Entity>>& Entities
     */
    const std::unordered_map<uint64_t, std::shared_ptr<Entity>>& getEntities() const;

    /**
     * @brief Enqueue an event for processing
     * 
     * @param event Event to enqueue
     */
    void enqueueEvent(const Event& event);
    
    /**
     * @brief Get the Local Virtual Time (LVT) of this LP
     * 
     * @return double LVT
     */
    double getLocalVirtualTime() const;

    /**
     * @brief Set the Local Virtual Time (LVT) of this LP
     * 
     * @param time new LVT
     */
    void setLocalVirtualTime(double time);

    /**
     * @brief Process next event in queue
     * 
     * @return true If an event was processed
     * @return false If the queue is empty
     */
    virtual bool processNextEvent() = 0;

    /**
     * @brief Initialize all entities in this LP
     * 
     */
    void initializeEntities();

    /**
     * @brief Get the number of events in the queue
     * 
     * @return size_t the number of events
     */
    size_t getEventQueueSize() const;

    /**
     * @brief Get the LP that an entity is in
     * 
     * @param entity_id entity id
     * @return uint32_t Lp id
     */
    uint32_t getEntityLP(uint64_t entity_id) const;

    /**
     * @brief  Get the next event time without removing it
     * 
     * @return double Timestamp of the next event, or infinity if queue is empty
     */
    double peekNextEventTime() const;

    /**
     * @brief Lock this LP for thread safe access
     * 
     */
    void lock();

    /**
     * @brief Unlock this LP
     * 
     */
    void unlock();

protected:
    uint32_t id_;                                                                   // Unique identifier for this LP
    std::priority_queue<Event, std::vector<Event>, EventComparator> event_queue_;   // Queue of pending events
    std::unordered_map<uint64_t, std::shared_ptr<Entity>> entities_;                // Entities managed by this LP
    std::atomic<double> local_virtual_time_;                                        // Current simulation time for this LP

    mutable std::mutex mutex_;                                                      // Mutex for thread-safe access
    inline static std::unordered_map<uint64_t, uint32_t> entities_lp_;

    // Statistics
    size_t processed_events_ = 0;
    size_t generated_events_ = 0;
    size_t rollbacks_ = 0;
};

}

#endif