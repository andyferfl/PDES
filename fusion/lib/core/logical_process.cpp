#include "core/logical_process.hpp"

namespace fusion
{
// LogicalProcess::LogicalProcess(uint32_t id)
//     : id_(id)
// {
// }

LogicalProcess::LogicalProcess(uint32_t id)
    : id_(id),
      local_virtual_time_(0.0)
{
}


uint32_t LogicalProcess::getId() const
{
    return id_;
}

void LogicalProcess::addEntity(std::shared_ptr<Entity> entity)
{
    entity->setLogicalProcess(id_);
    entities_[entity->getId()] = entity;
    
    entities_lp_[entity->getId()] = id_;
}

std::shared_ptr<Entity> LogicalProcess::getEntity(uint64_t entity_id) const
{
    auto it = entities_.find(entity_id);
    if (it != entities_.end())
    {
        return it->second;
    }
    return nullptr;
}

const std::unordered_map<uint64_t, std::shared_ptr<Entity>>& LogicalProcess::getEntities() const
{
    return entities_;
}

void LogicalProcess::enqueueEvent(const Event& event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    event_queue_.push(event);
}

double LogicalProcess::getLocalVirtualTime() const
{
    return local_virtual_time_.load();
}

void LogicalProcess::setLocalVirtualTime(double time)
{
    local_virtual_time_.store(time);
}

void LogicalProcess::initializeEntities()
{
    for (auto& pair : entities_)
    {
        std::vector<Event> events = pair.second->initialize();

        for (const auto& event : events)
        {
            event_queue_.push(event);
        }
    }
}

size_t LogicalProcess::getEventQueueSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return event_queue_.size();
}

uint32_t LogicalProcess::getEntityLP(uint64_t entity_id) const
{
    return entities_lp_[entity_id];
}

double LogicalProcess::peekNextEventTime() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (event_queue_.empty())
    {
        return std::numeric_limits<double>::infinity();
    }

    return event_queue_.top().getTimestamp();
}

void LogicalProcess::lock()
{
    mutex_.lock();
}

void LogicalProcess::unlock()
{
    mutex_.unlock();
}

}
