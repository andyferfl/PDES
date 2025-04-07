#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "event.hpp"

namespace fusion
{

class Entity
{
public:

    /**
     * @brief Construct a new Entity object
     * 
     * @param id Unique identifier for this entity
     */
    explicit Entity(uint64_t id);

    /**
     * @brief Destroy the Entity object
     * 
     */
    virtual ~Entity() = default;

    /**
     * @brief Get the entity ID
     * 
     * @return uint64_t entity ID
     */
    uint64_t getId() const;

    /**
     * @brief Initialize the entity
     * 
     * This method is called once at the begining of the simulation.
     * Override this method to initialize the entity's state.
     * @return std::vector<Event> vector with initial events
     */
    virtual std::vector<Event> initialize() = 0;

    /**
     * @brief Handle an event
     * 
     * This method is called when an event is delivered to this entity.
     * Override this method to define how the entity responds to events.
     * 
     * @param event Event to handle
     * @param current_time Current simulation time
     * @return std::vector<Event> Any new events generated as a result
     */
    virtual std::vector<Event> handleEvent(const Event& event, double current_time) = 0;

    /**
     * @brief Save the entity's state
     * 
     * This method is called by PDES algorithms to save the entity's state
     * before processing an event that might be rolled back.
     * 
     * @param time Simulation time at which the state is saved.
     * @return std::any The saved state.
     */
    virtual std::any saveState(double time);

    /**
     * @brief Restore the entity's state
     * 
     * This method is called by PDES algorithms to restore the entity's
     * state after a rollback.
     * 
     * @param state state to restore 
     * @param time Simulation time to restore to
     */
    virtual void restoreState(const std::any& state, double time);

    /**
     * @brief Set the Logical Process ID
     * 
     * @param lp_id Logical Process ID
     */
    void setLogicalProcess(uint32_t lp_id);

    /**
     * @brief Get the Logical Process ID
     * 
     * @return uint32_t Logical Process ID
     */
    uint32_t getLogicalProcess() const;

protected:

    /**
     * @brief Create a new event
     * 
     * Helper method to create events with this entity as the source.
     * 
     * @param timestamp When the event should occur
     * @param target_id Target entity ID
     * @param type_id event type ID
     * @param data Optional data to include with the event
     * @return Event The created event
     */
    Event createEvent(double timestamp, uint64_t target_id, uint32_t type_id, std::any data = {}) const;

private:
    uint64_t id_;               // Unique ID for this entity
    uint32_t logical_process_;  // ID of the LP this entity belongs to
};

}

#endif