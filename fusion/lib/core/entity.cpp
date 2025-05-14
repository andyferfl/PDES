#include "core/entity.hpp"

namespace fusion
{

Entity::Entity(uint64_t id)
    : id_(id)
{
}

uint64_t Entity::getId() const
{
    return id_;
}

uint64_t Entity::getNumSavedStates() const
{
    return num_saved_states_;
}

uint64_t Entity::getMaxNumSavedStates() const
{
    return max_num_prev_states_;
}

void Entity::setNumSavedStates(const uint64_t saved_states)
{
    num_saved_states_ = saved_states;
}


/*Entity& Entity::saveState(double time)
{
    if (num_saved_states_ == max_num_prev_states_)
    {
        //return;
    }

    num_saved_states_++;

    return *this;
}*/

void Entity::restoreState(const Entity& state, double time)
{
    //Default implementation does nothing.
    //This method should be overrided to restore a specific state.   
}

void Entity::setLogicalProcess(uint32_t lp_id)
{
    logical_process_ = lp_id;
}

uint32_t Entity::getLogicalProcess() const
{
    return logical_process_;
}

Event Entity::createEvent(double timestamp, uint64_t target_id, uint32_t type_id, std::any data) const
{
    Event event(timestamp, id_, target_id, type_id, std::move(data));
    event.setSendingLP(logical_process_);
    return event;
}

}
