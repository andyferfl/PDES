#include "core/entity.hpp"
#include <cstdint>
#include <limits>
namespace fusion
{

// Entity::Entity(uint64_t id)
//     : id_(id)
// {
// }

Entity::Entity(uint64_t id)
    : id_(id),
      logical_process_(std::numeric_limits<uint32_t>::max())  // Инициализируем дефолтным значением
{
}


uint64_t Entity::getId() const
{
    return id_;
}

std::any Entity::saveState(double time)
{
    //Default implementation returns empty state.
    //This method should be overrided to save a specific state.
    return {};
}

void Entity::restoreState(const std::any& state, double time)
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
