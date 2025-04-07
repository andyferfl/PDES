#ifndef EVENT_HPP
#define EVENT_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <any>

namespace fusion
{

/**
* @brief Class representing an event in the simulation
*
* It contains the basic information needed for any DES or PDES implementation. 
*/
class Event
{
public:

    /**
     * @brief Construct a new Event object
     * 
     * @param timestamp The simulation time when this event occurs
     * @param source_id ID of the entity that generated this event
     * @param target_id ID of the entity that should receive this event
     * @param type_id Type identifier for the event
     */
    Event(double timestamp, uint64_t source_id, uint64_t target_id, uint32_t type_id);

    /**
     * @brief Construct a new Event object with data
     * 
     * @param timestamp The simulation time when this event occurs
     * @param source_id ID of the entity that generated this event
     * @param target_id ID of the entity that should receive this event
     * @param type_id Type identifier for the event
     * @param data Custom data related to the event
     */
    Event(double timestamp, uint64_t source_id, uint64_t target_id, uint32_t type_id, std::any data);

    /**
     * @brief Get the Timestamp of the evnt
     * 
     * @return double Timestamp
     */
    double getTimestamp() const;

    /**
     * @brief Get the Source entity Id
     * 
     * @return uint64_t source entity ID
     */
    uint64_t getSourceId() const;

    /**
     * @brief Get the Target entity Id
     * 
     * @return uint64_t target entity ID
     */
    uint64_t getTargetId() const;

    /**
     * @brief Get the event Type Id
     * 
     * @return uint64_t event type ID
     */
    uint64_t getTypeId() const;

    /**
     * @brief Get the event data
     * 
     * @return const std::any& event data
     */
    const std::any& getData() const;

    /**
     * @brief Set the Creation Timestamp
     * 
     * @param creation_time the simulation time when this event was created
     */
    void setCreationTime(double creation_time);
    
    /**
     * @brief Get the Creation Timestamp
     * 
     * @return double creation timestamp
     */
    double getCreationTime() const;

    /**
     * @brief Set the Sending LP ID
     * 
     * @param lp_id ID of the LP that sends this event
     */
    void setSendingLP(uint32_t lp_id);

    /**
     * @brief Get the Sending LP ID
     * 
     * @return uint32_t Sending LP ID
     */
    uint32_t getSendingLP() const;

    /**
     * @brief Set the Receiving LP ID
     * 
     * @param lp_id The ID of the LP that receives this event
     */
    void setReceivingLP(uint32_t lp_id);

    /**
     * @brief Get the Receiving LP ID
     * 
     * @return uint32_t receiving LP ID
     */
    uint32_t getReceivingLP() const;

    /**
     * @brief Set the Anti-Message flag
     * 
     * @param is_anti_message whether this is an anti-message
     */
    void setAntiMessage(bool is_anti_message);

    /**
     * @brief Check if this is an anti-message
     * 
     * @return true If this is an anti-message
     * @return false If this is a regular message
     */
    bool isAntiMessage() const;

private:
    double timestamp_;      // Whent the event occurs
    uint64_t source_id_;    // Entity that generated the event
    uint64_t target_id_;    // Entity that should receive the event
    uint32_t type_id_;      // Type of the event
    std::any data_;         // Custom data related to the event

    double creation_time_;  // When the event was created (simulation time)
    uint32_t sending_lp_;   // LP that sent this event
    uint32_t receiving_lp_; // LP that will receive the event
    bool is_anti_message_;  // Whether this is an antimessage (Time Warp)
};


/**
* @brief Comparator for events based on timestamp
*
* This is used for priority queues to order event by timestamp.
*/
struct EventComparator
{
    bool operator()(const Event& a, const Event& b) const
    {
        return a.getTimestamp() > b.getTimestamp();
    }

    bool operator()(const std::shared_ptr<Event>& a, const std::shared_ptr<Event>& b) const
    {
        return a->getTimestamp() > b->getTimestamp();
    }
};

}

#endif