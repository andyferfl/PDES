#include "algorithms/null_messages_engine.hpp"

#include <chrono>
#include <iostream>
#include <atomic>
#include <pthread.h>
#include <limits>
#include <algorithm>
#include <map>

namespace fusion
{

class NullMessagesEngine::NullMessagesLP : public LogicalProcess
{
public:
    NullMessagesLP(uint32_t id, double lookahead, bool dynamic_lookahead)
        : LogicalProcess(id),
          lookahead_(lookahead),
          dynamic_lookahead_(dynamic_lookahead),
          last_null_messages_time_(0.0)
    {
    }

    bool processNextEvent() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (event_queue_.empty())
        {
            return false;
        }

        Event event = event_queue_.top();
        event_queue_.pop();

        if (event.getTypeId() == NULL_MESSAGE_TYPE_ID)
        {
            input_times_[event.getSendingLP()] = event.getTimestamp();
            return true;
        }

        setLocalVirtualTime(event.getTimestamp());

        auto entity = getEntity(event.getTargetId());
        if (!entity)
        {
            std::cerr << "Warning: Event for unknown entity " << event.getTargetId() << std::endl;
            return true;
        }

        std::vector<Event> new_events = entity->handleEvent(event, getLocalVirtualTime());

        for (const auto& new_event : new_events)
        {
            uint32_t target_lp = new_event.getReceivingLP();
            if (target_lp == getId())
            {
                event_queue_.push(new_event);
            }
            else
            {
                pending_remote_events_[target_lp].push_back(new_event);
            }
        }

        processed_events_++;
        return true;
    }

    void deliverRemoteEvents(std::vector<NullMessagesLP*>& all_lps)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (uint32_t target_lp = 0; target_lp < pending_remote_events_.size(); ++target_lp)
        {
            auto& events = pending_remote_events_[target_lp];
            if (!events.empty())
            {
                for (const auto& event : events)
                {
                    all_lps[target_lp]->enqueueEvent(event);
                }
                events.clear();
            }
        }
    }

    void sendNullMessages(std::vector<NullMessagesLP*>& all_lps)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        double current_time = getLocalVirtualTime();

        if (current_time > last_null_messages_time_)
        {
            last_null_messages_time_ = current_time;

            for (size_t target_lp = 0; target_lp < all_lps.size(); ++target_lp)
            {
                if (target_lp == getId())
                {
                    continue;
                }

                double safe_time = current_time + getLookahead();

                Event null_message(safe_time, getId(), 0, NULL_MESSAGE_TYPE_ID);
                null_message.setSendingLP(getId());
                null_message.setReceivingLP(target_lp);

                all_lps[target_lp]->enqueueEvent(null_message);
                null_messages_sent_++;
            }
        }
    }

    double getEarliestInputTime() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        double earliest = std::numeric_limits<double>::infinity();
        for (const auto& [lp_id, time] : input_times_)
        {
            earliest = std::min(earliest, time);
        }
        return earliest;
    }

    double getLookahead() const
    {
        if (dynamic_lookahead_)
        {
            return lookahead_ * (1.0 + 0.1 * processed_events_ / 1000.0);
        }
        return lookahead_;
    }

    void setInputChannels(const std::vector<uint32_t>& input_lps)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (uint32_t lp_id : input_lps)
        {
            input_times_[lp_id] = std::numeric_limits<double>::infinity();
        }
    }

    uint32_t getProcessedEvents() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return processed_events_;
    }

    uint32_t getNullMessagesSent() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return null_messages_sent_;
    }

    static constexpr uint32_t NULL_MESSAGE_TYPE_ID = 0xFFFFFFFF;

private:
    double lookahead_;
    bool dynamic_lookahead_;
    double last_null_messages_time_;
    std::vector<std::vector<Event>> pending_remote_events_;
    std::map<uint32_t, double> input_times_;
    uint32_t null_messages_sent_ = 0;
};


NullMessagesEngine::NullMessagesEngine(const SimulationConfig& config)
    : SimulationEngine(config)
{
    logical_processes_.resize(config.num_logical_processes);
    for (uint32_t i = 0; i < config.num_logical_processes; ++i)
    {
        logical_processes_[i] = std::make_unique<NullMessagesLP>(i, config.null_messages.loookahead, config.null_messages.dynamic_lookahead);
    }
}

NullMessagesEngine::~NullMessagesEngine() = default;

void NullMessagesEngine::registerEntity(std::shared_ptr<Entity> entity)
{
    uint32_t lp_id = entity->getId() % config_.num_logical_processes;
    logical_processes_[lp_id]->addEntity(entity);
}

void NullMessagesEngine::initialize()
{
    for (auto& lp : logical_processes_)
    {
        lp->initializeEntities();
    }

    for (uint32_t i = 0; i < logical_processes_.size(); ++i)
    {
        std::vector<uint32_t> input_lps;
        for (uint32_t j = 0; j < logical_processes_.size(); ++j)
        {
            if (i != j)
            {
                input_lps.push_back(j);
            }
        }
        static_cast<NullMessagesLP*>(logical_processes_[i].get())->setInputChannels(input_lps);
    }

    stats_ = SimulationStats();
}

SimulationStats NullMessagesEngine::run()
{
    auto start_time = std::chrono::high_resolution_clock::now();
    running_ = true;

    worker_threads_.resize(config_.num_threads);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, nullptr, config_.num_threads);

    auto thread_func = [this, &barrier](uint32_t thread_id) {
        std::vector<NullMessagesLP*> my_lps;
        for (uint32_t lp_id = thread_id; lp_id < logical_processes_.size(); lp_id += config_.num_threads)
        {
            my_lps.push_back(static_cast<NullMessagesLP*>(logical_processes_[lp_id].get()));
        }

        while (running_)
        {
            bool processed = false;
            bool all_empty = true;
            for (auto lp : my_lps)
            {
                double safe_time = lp->getEarliestInputTime();

                while (lp->peekNextEventTime() < safe_time)
                {
                    if (lp->processNextEvent())
                    {
                        processed = true;
                        all_empty = false;
                    }
                    else
                    {
                        break;
                    }
                }

                if (lp->getEventQueueSize() > 0)
                {
                    all_empty = false;
                }
            }

            pthread_barrier_wait(&barrier);
            for (auto lp : my_lps)
            {
                lp->deliverRemoteEvents(reinterpret_cast<std::vector<NullMessagesLP*>&>(logical_processes_));
            }
            pthread_barrier_wait(&barrier);

            for (auto lp : my_lps)
            {
                lp->sendNullMessages(reinterpret_cast<std::vector<NullMessagesLP*>&>(logical_processes_));
            }
            pthread_barrier_wait(&barrier);

            bool terminate = true;
            for (const auto& lp : logical_processes_)
            {
                if (lp->getLocalVirtualTime() < config_.end_time || lp->getEventQueueSize() > 0)
                {
                    terminate = false;
                    break;
                }
            }

            if (terminate|| (all_empty && !processed))
            {
                running_ = false;
            }
            pthread_barrier_wait(&barrier);

           
        }
    };

    for (uint32_t i = 0; i < config_.num_threads; ++i)
    {
        worker_threads_[i] = std::thread(thread_func, i);
    }

    for (auto& thread : worker_threads_)
    {
        thread.join();
    }

    pthread_barrier_destroy(&barrier);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    double max_time = 0.0;
    for (const auto& lp : logical_processes_)
    {
        max_time = std::max(max_time, lp->getLocalVirtualTime());
    }

    stats_.simulation_time = max_time;
    stats_.wall_clock_time = elapsed.count();
    stats_.total_events_processed = 0;
    stats_.total_null_messages = 0;
    stats_.events_processed_per_lp.resize(logical_processes_.size());
    stats_.null_messages_per_lp.resize(logical_processes_.size());

    for (size_t i = 0; i < logical_processes_.size(); ++i)
    {
        auto lp = static_cast<NullMessagesLP*>(logical_processes_[i].get());
        stats_.events_processed_per_lp[i] = lp->getProcessedEvents();
        stats_.null_messages_per_lp[i] = lp->getNullMessagesSent();
        stats_.total_events_processed += lp->getProcessedEvents();
        stats_.total_null_messages += lp->getNullMessagesSent();
    }

    stats_.efficiency = stats_.total_events_processed / stats_.wall_clock_time;

    return stats_;    
}

double NullMessagesEngine::getCurrentTime() const
{
    double min_time = std::numeric_limits<double>::infinity();
    for (const auto& lp : logical_processes_)
    {
        min_time = std::min(min_time, lp->getLocalVirtualTime());
    }
    return min_time;
}

void NullMessagesEngine::sendNullMessages(uint32_t lp_id)
{
    auto lp = static_cast<NullMessagesLP*>(logical_processes_[lp_id].get());
    lp->sendNullMessages(reinterpret_cast<std::vector<NullMessagesLP*>&>(logical_processes_));
}
}
