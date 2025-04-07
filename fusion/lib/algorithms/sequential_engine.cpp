#include "algorithms/sequential_engine.hpp"
#include <chrono>
#include <iostream>

namespace fusion
{
SequentialEngine::SequentialEngine(const SimulationConfig& config)
    : SimulationEngine(config),
      current_time_(0.0)
{
}

void SequentialEngine::registerEntity(std::shared_ptr<Entity> entity)
{
    entities_[entity->getId()] = entity;
}

void SequentialEngine::initialize()
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

SimulationStats SequentialEngine::run()
{
    auto start_time = std::chrono::high_resolution_clock::now();
    running_ = true;
      
    while (!event_queue_.empty() && running_)
    {
        Event event = event_queue_.top();
        event_queue_.pop();
        current_time_ = event.getTimestamp();

        if (current_time_ >= config_.end_time)
        {
            break;
        }

        auto it = entities_.find(event.getTargetId());
        if (it == entities_.end())
        {
            std::cerr << "Warning: Event for unknown entity " << event.getTargetId() << std::endl;
            continue;
        }

        std::vector<Event> new_events = it->second->handleEvent(event, current_time_);

        for (const auto& new_event : new_events)
        {
            event_queue_.push(new_event);
        }

        stats_.total_events_processed++;
        stats_.total_events_generated += new_events.size();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    stats_.simulation_time = current_time_;
    stats_.wall_clock_time = elapsed.count();
    stats_.efficiency = stats_.total_events_processed / stats_.wall_clock_time;

    running_ = false;

    return stats_;
}

double SequentialEngine::getCurrentTime() const
{
    return current_time_;
}
}
