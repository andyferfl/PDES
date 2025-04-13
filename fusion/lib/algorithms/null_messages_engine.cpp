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
// Specialized LP for null messages
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

    /**
     * @brief process the next event in the event_queue
     * 
     * @return true if the event was processed
     * @return false if the event_queue is empty 
     */
    bool processNextEvent() override
    {
        if (event_queue_.empty())
        {
            return false;
        }

        Event event = event_queue_.top();
        event_queue_.pop();

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
            uint64_t target_entity = new_event.getTargetId();
            uint32_t target_lp = getEntityLP(target_entity);

            if (target_lp == getId())
            {
                event_queue_.push(new_event);
            }
            else
            {
               pending_remote_events_[target_lp].push_back(new_event);
            }
        }
        generated_events_ += new_events.size();
        processed_events_++;
        return true;
    }

    /**
     * @brief Deliver the events to the corresponding LP's
     * 
     * @param all_lps a vector with references to all LP's
     */
    void deliverRemoteEvents(std::vector<NullMessagesLP*>& all_lps)
    {
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

    /**
     * @brief Send null messages using input channels
     * 
     * @param all_lps a vector with references to all LP's
     */
    void sendNullMessages(std::vector<NullMessagesLP*>& all_lps)
    {
        double current_time = getLocalVirtualTime();
        if (current_time >= last_null_messages_time_)
        {
            last_null_messages_time_ = current_time;

            for (size_t target_lp = 0; target_lp < all_lps.size(); ++target_lp)
            {
                if (target_lp == getId())
                {
                    continue;
                }

                double safe_time = current_time + getLookahead();
                all_lps[target_lp]->updateInputChannel(getId(), safe_time);
                null_messages_sent_++;
            }
        }
    }

    /**
     * @brief Get the Earliest null message timestamp for this LP
     * 
     * @return double the earliest timestamp
     */
    double getEarliestInputTime() const
    {
        double earliest = std::numeric_limits<double>::infinity();
        for (const auto& [lp_id, time] : input_times_)
        {
            earliest = std::min(earliest, time);
        }
        return earliest;
    }

    /**
     * @brief Get the Lookahead
     * 
     * @return double lookahead
     */
    double getLookahead() const
    {
        if (dynamic_lookahead_)
        {
            return lookahead_ * (1.0 + 0.1 * processed_events_ / 1000.0);
        }
        return lookahead_;
    }

    /**
     * @brief Set the Input Channels
     * 
     * It is used at initialization but can also be used for updating null messages times
     * for all lp's at once
     * 
     * @param input_lps 
     */
    void setInputChannels(const std::vector<std::pair<uint32_t, double>>& input_lps)
    {
        for (const auto& [lp_id, earliest_event] : input_lps)
        {
            input_times_[lp_id] = earliest_event;
        }
    }

    /**
     * @brief sets the null message channel from a determined lp
     * 
     * @param lp_id lp that sent the null message
     * @param time timestamp of the null message
     */
    void updateInputChannel(const uint32_t& lp_id, const double& time )
    {
        std::lock_guard<std::mutex> lock(mutex_);
        input_times_[lp_id] = time;
    }

    /**
     * @brief Set the Last Null Message Time
     * 
     * @param time 
     */
    void setLastNullMessageTime(const double& time)
    {
        last_null_messages_time_ = time;
    }

    uint32_t getProcessedEvents() const
    {
        return processed_events_;
    }

    uint32_t getGeneratedEvents() const
    {
        return generated_events_;
    }

    uint32_t getNullMessagesSent() const
    {
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
        std::vector<std::pair<uint32_t, double>>input_lps;
        for (uint32_t j = 0; j < logical_processes_.size(); ++j)
        {
            if (i != j)
            {
                uint32_t lp_id = j;
                double earliest_event = logical_processes_[j]->peekNextEventTime();
                logical_processes_[j]->setLastNullMessageTime(earliest_event);
                input_lps.push_back(std::make_pair(lp_id, earliest_event));
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
                if (lp->getLocalVirtualTime() < config_.end_time && lp->getEventQueueSize() > 0)
                {
                    terminate = false;
                    break;
                }
            }

            pthread_barrier_wait(&barrier);

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
    stats_.total_events_generated = 0;
    stats_.total_null_messages = 0;
    stats_.events_processed_per_lp.resize(logical_processes_.size());
    stats_.null_messages_per_lp.resize(logical_processes_.size());

    for (size_t i = 0; i < logical_processes_.size(); ++i)
    {
        auto lp = static_cast<NullMessagesLP*>(logical_processes_[i].get());
        stats_.events_processed_per_lp[i] = lp->getProcessedEvents();
        stats_.null_messages_per_lp[i] = lp->getNullMessagesSent();
        stats_.total_events_processed += lp->getProcessedEvents();
        stats_.total_events_generated += lp->getGeneratedEvents();
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

}
