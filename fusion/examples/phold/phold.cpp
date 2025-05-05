#include "phold.hpp"
#include <algorithm>
#include <cmath>
// #include <stdexcept>  
// #include <ostream>
// #include <iostream>

namespace fusion
{
enum PHoldEventType
{
    PHOLD_EVENT = 1
};

struct PHoldEntityState
{
    uint64_t events_processed;
    uint64_t events_generated;
};

PHoldEntity::PHoldEntity(
    uint64_t id,
    uint64_t num_entities,
    double remote_probability,
    double zero_delay_probability,
    double lookahead,
    double mean_delay,
    uint32_t initial_events,
    uint64_t seed)
    : Entity(id),
        num_entities_(num_entities),
        remote_probability_(remote_probability),
        zero_delay_probability_(zero_delay_probability),
        lookahead_(lookahead),
        mean_delay_(mean_delay),
        initial_events_(initial_events),
        rng_(seed ^ id),
        uniform_dist_(0.0, 1.0),
        delay_dist_(1.0 / mean_delay),
        entity_dist_(0, num_entities - 1),
        events_processed_(0),
        events_generated_(0)
{
    // if (mean_delay <= 0.0) {
    //     throw std::runtime_error("mean_delay must be positive");
    // }
    // if (lookahead <= 0.0) {
    //     throw std::runtime_error("lookahead must be positive");
    // }
    // if (num_entities == 0) {
    //     throw std::runtime_error("num_entities cannot be zero");
    // }


}

std::vector<Event> PHoldEntity::initialize()
{
    std::vector<Event> events;
    for (uint32_t i = 0; i < initial_events_; ++i)
    {
        double delay = lookahead_ + delay_dist_(rng_);
        
        uint64_t dst_id;
        if (uniform_dist_(rng_) < remote_probability_)
        {
            dst_id = entity_dist_(rng_);
            if (dst_id == getId())
            {
                dst_id = (dst_id + 1) % num_entities_;
            }
        }
        else
        {
            dst_id = getId();
        }
        
        Event event = createEvent(delay, getId(), PHOLD_EVENT);
        events.push_back(event);
        events_generated_++;
    }
    return events;
}

std::vector<Event> PHoldEntity::handleEvent(const Event& event, double current_time)
{
    events_processed_++;
    std::vector<Event> new_events;
    new_events.push_back(generateEvent(current_time));
    
    return new_events;
}

std::any PHoldEntity::saveState(double time)
{
    PHoldEntityState state;
    state.events_processed = events_processed_;
    state.events_generated = events_generated_;
    return state;
}

void PHoldEntity::restoreState(const std::any& state, double time)
{
    try
    {
        const PHoldEntityState& saved_state = std::any_cast<const PHoldEntityState&>(state);
        events_processed_ = saved_state.events_processed;
        events_generated_ = saved_state.events_generated;
    }
    catch (const std::bad_any_cast& e)
    {
        // Handle error
    }
}

Event PHoldEntity::generateEvent(double current_time)
{
    double delay;
    if (uniform_dist_(rng_) < zero_delay_probability_)
    {
        delay = lookahead_;
    }
    else
    {
        delay = lookahead_ + delay_dist_(rng_);
    }
    
    uint64_t dst_id = getId();
    
    Event event = createEvent(current_time + delay, dst_id, PHOLD_EVENT);
    events_generated_++;
    
    return event;
}

std::vector<std::shared_ptr<Entity>> createPHoldModel(
    uint64_t num_entities,
    double remote_probability,
    double zero_delay_probability,
    double lookahead,
    double mean_delay,
    uint32_t initial_events,
    uint64_t seed)
{
    std::vector<std::shared_ptr<Entity>> entities;
    entities.reserve(num_entities);
    
    for (uint64_t i = 0; i < num_entities; ++i)
    {
        entities.push_back(std::make_shared<PHoldEntity>(
            i, num_entities, remote_probability, zero_delay_probability,
            lookahead, mean_delay, initial_events, seed));
    }
    
    return entities;
}
}