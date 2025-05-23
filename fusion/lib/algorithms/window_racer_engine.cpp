#include "algorithms/window_racer_engine.hpp"

#include <chrono>
#include <iostream>
#include <atomic>
#include <pthread.h>
#include <limits>
#include <algorithm>
#include <map>

namespace fusion
{

template<typename T>
inline void atomic_min(std::atomic<T> & atom, const T val)
{
  for(T atom_val=atom;
      atom_val > val &&
      !atom.compare_exchange_weak(atom_val, val, std::memory_order_relaxed);
     );
}

template <class ADAPTER>
typename ADAPTER::container_type & get_container (ADAPTER &a)
{
    struct hack : ADAPTER {
        static typename ADAPTER::container_type & get (ADAPTER &a) {
            return a.*&hack::c;
        }
    };
    return hack::get(a);
}


// Specialized LP for window racer
class WindowRacerEngine::WindowRacerLP : public LogicalProcess
{
public:
    WindowRacerLP(uint32_t id, uint64_t max_states_saved)
        : LogicalProcess(id),
          last_change_timestamp_(0.0),
          num_saved_states_(0),
          max_saved_states_(max_states_saved),
          immediate_rollbacks_(0),
          window_end_rollbacks_(0)
    {
        previous_states_.reserve(max_states_saved);
    }

    void addEntity(std::shared_ptr<Entity> entity)
    {
        entity_ = entity;
        entity_->setMaxNumSavedStates(max_saved_states_);
    }

    std::vector<Event> initializeEntities()
    {
        return entity_->initialize();
    }

    double getLastChangeTimestamp()
    {
        return last_change_timestamp_;
    }

    void setLastChangeTimestamp(double last_change_timestamp)
    {
        last_change_timestamp_ = last_change_timestamp;
    }

    uint64_t getNumSavedStates()
    {
        return num_saved_states_;
    }

    void setNumSavedStates(uint64_t num_saved_states)
    {
        num_saved_states_ = num_saved_states;
        entity_->setNumSavedStates(num_saved_states);
    }

    std::vector<Event>& getEventList()
    {
        return event_list_;
    }

    std::vector<std::pair<double, Entity*>>& getSavedStates()
    {
        return previous_states_;
    }

    uint64_t getProcessedEvents() const
    {
        return processed_events_;
    }

    uint64_t getImmediateRollbacks() const
    {
        return immediate_rollbacks_;
    }

    uint64_t getWindowEndRollbacks() const
    {
        return window_end_rollbacks_;
    }

    uint64_t getGeneratedEvents() const
    {
        return generated_events_;
    }

    uint64_t getLastChangeTimestamp() const
    {
        return last_change_timestamp_;
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
        return true;
    }

    bool saveState()
    {
        uint64_t saved_states = entity_->getNumSavedStates();

        if (saved_states == max_saved_states_)
        {
            return false;
        }

        saved_states++;

        previous_states_[saved_states - 1] = std::make_pair(last_change_timestamp_, &entity_->saveState(last_change_timestamp_));

        entity_->setNumSavedStates(saved_states);
        num_saved_states_ = saved_states;

        return true;
    }

    uint64_t restoreStateWindowEnd(double ts)
    {
        double earliest_change_ts = last_change_timestamp_;
        uint64_t i = entity_->getNumSavedStates();
        uint64_t rollbacks = 1;

        for (; previous_states_[i - 1].first >= ts > 0; i--)
        {
            rollbacks++;
            window_end_rollbacks_++;
        }

        window_end_rollbacks_++;

        std::shared_ptr<Entity> tmp(previous_states_[i-1].second, [](Entity*){/* no-op deleter */});
        entity_ = tmp;
        entity_->setNumSavedStates(i-1);
        num_saved_states_ = i-1;
        last_change_timestamp_ = previous_states_[i-1].first;
        return rollbacks;
    }

    double restoreState(double ts)
    {
        double earliest_change_ts = last_change_timestamp_;
        uint64_t i = entity_->getNumSavedStates();
        for (; previous_states_[i - 1].first > ts; i--)
        {
            earliest_change_ts = previous_states_[i - 1].first;
            immediate_rollbacks_++;
        }
        
        immediate_rollbacks_++;

        std::shared_ptr<Entity> tmp(previous_states_[i-1].second, [](Entity*){/* no-op deleter */});
        entity_ = tmp;
        entity_->setNumSavedStates(i-1);
        num_saved_states_ = i-1;
        last_change_timestamp_ = previous_states_[i-1].first;

        return earliest_change_ts;
    }

    bool checkBound(double ts, std::array<std::atomic<double>,2>& window_ub, bool window_lsb)
    {
        if (ts >= window_ub[window_lsb])
        {
            unlock();
            return false;  
        }

        if (ts < last_change_timestamp_ && last_change_timestamp_ >= 0.0)
        {
            double earliest_displaced_event_ts = restoreState(ts);
            atomic_min(window_ub[window_lsb], earliest_displaced_event_ts);
            last_change_timestamp_ = ts;
            return true;
        }

        bool r = saveState();

        if (!r)
        {
            atomic_min(window_ub[window_lsb], ts);
            return false;
        }

        last_change_timestamp_ = ts;
        return true;
    }

    /**
     * @brief Process an specific event in the entity that is contained in this lp
     * 
     * @param event 
     * @param window_ub Window upper bound
     * @param window_lsb Window bit
     * @return std::tuple<bool, bool, std::vector<Event>, std::vector<Event>> flag correctly completed, flag dirty entity, generated events, new events
     */
    std::tuple<bool, bool, std::vector<Event>, std::vector<Event>> processEvent(const Event& event, std::array<std::atomic<double>,2>& window_ub, bool window_lsb)
    {
        lock();

        std::vector<Event> generated_events;
        std::vector<Event> new_events;
        bool is_dirty = false;

        auto entity = entity_;
        
        if (!entity)
        {
            std::cerr << "Warning: Event for unknown entity " << event.getTargetId() << std::endl;
            unlock();
            return {false, is_dirty, generated_events, new_events};
        }

        if (num_saved_states_ == 0)
        {
            is_dirty = true;
        }

        if (event.getTimestamp() != event.getCreationTime())
        {
            event_list_.push_back(event);
        }

        if (!checkBound(event.getTimestamp(), window_ub, window_lsb))
        {
            unlock();
            return {false, is_dirty, generated_events, new_events};
        }

        std::vector<Event> result_events = entity->handleEvent(event, last_change_timestamp_);

        generated_events.reserve(new_events.size());
        new_events.reserve(new_events.size());

        for (const auto& new_event : result_events)
        {
            if (new_event.getTimestamp() == last_change_timestamp_ || last_change_timestamp_ == 0.0)
            {
                generated_events.push_back(new_event);
            }
            else
            {
                new_events.push_back(new_event);
            }
        }

        generated_events_ += new_events.size();
        processed_events_++;

        return {true, is_dirty, generated_events, new_events};
    }


private:
    uint64_t id;
    double last_change_timestamp_;
    uint64_t num_saved_states_;
    uint64_t max_saved_states_;
    uint64_t immediate_rollbacks_;
    uint64_t window_end_rollbacks_;
    std::shared_ptr<Entity> entity_;
    std::vector<Event> event_list_;
    std::vector<std::pair<double, Entity*>> previous_states_;
};



WindowRacerEngine::WindowRacerEngine(const SimulationConfig& config)
    : SimulationEngine(config),
    num_entities_(config.window_racer.num_entities)
{
    logical_processes_.resize(config.window_racer.num_entities);
    for (uint32_t i = 0; i < config.window_racer.num_entities; ++i)
    {
        logical_processes_[i] = std::make_unique<WindowRacerLP>(i, config.window_racer.max_states_saved);
    }

    event_queues_.resize(config.num_threads);
    new_event_queues_.resize(config.num_threads);
    new_events_list_.resize(config.num_threads);
    dirty_entities_.resize(config.num_threads);
    for (uint32_t i = 0; i < config.num_threads; ++i)
    {
        event_queues_[i] = std::priority_queue<Event, std::vector<Event>, EventComparator>();
        new_event_queues_[i] = std::priority_queue<Event, std::vector<Event>, EventComparator>();

        new_events_list_[i].resize(config.num_threads);
        dirty_entities_[i].resize(config.num_threads);
        for (uint32_t j = 0; j < config.num_threads; ++j)
        {
            new_events_list_[i][j] = std::vector<Event>();
            dirty_entities_[i][j] = std::vector<WindowRacerLP *>();
        }
    }

}

WindowRacerEngine::~WindowRacerEngine() = default;

void WindowRacerEngine::registerEntity(std::shared_ptr<Entity> entity)
{
    uint32_t lp_id = entity->getId();
    logical_processes_[lp_id]->addEntity(entity);
}

void WindowRacerEngine::initialize()
{
    for (auto& lp : logical_processes_)
    {
        auto events = lp->initializeEntities();

        for (auto& event : events)
        {
            event_queues_[id_to_tid(event.getTargetId())].push(event);
        }
    }

    stats_ = SimulationStats();
}

inline int WindowRacerEngine::id_to_tid(int id)
{
  return std::min(id / (num_entities_ / config_.num_threads), (uint64_t)config_.num_threads - 1);
}

void WindowRacerEngine::handleEvent(Event next_event, uint32_t thread_id, bool window_lsb)
{
    auto lp = logical_processes_[next_event.getTargetId()];

    auto result = lp->processEvent(next_event, window_ub_, window_lsb);
    if (std::get<1>(result))
    {
        dirty_entities_[thread_id][id_to_tid(next_event.getTargetId())].push_back(lp.get());
    }

    if (!std::get<0>(result))
    {
        return;
    }

    for (auto & event : std::get<3>(result))
    {
        new_event_queues_[thread_id].push(event);
    }
    
    //std::cout<<"tid:"<<thread_id<<" new_ev:"<<new_event_queues_.size()<<" gen_ev:"<<std::get<2>(result).size()<<std::endl;

    lp->unlock();

    for (auto event : std::get<2>(result))
    {
        handleEvent(event, thread_id, window_lsb);
    }

    return;
}

SimulationStats WindowRacerEngine::run()
{
    auto start_time = std::chrono::high_resolution_clock::now();
    running_ = true;

    worker_threads_.resize(config_.num_threads);
    stats_.window_racer.windows = 0;
    stats_.events_commited_per_lp.resize(config_.num_threads);
    stats_.window_racer.window_sizes.reserve(config_.end_time/config_.window_racer.initial_window_size);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, nullptr, config_.num_threads);
    

    auto thread_func = [this, &barrier](uint32_t thread_id) {
        
        double tau = config_.window_racer.initial_window_size;
        double local_virtual_time = 0.0;
        bool window_lsb = 0;
        double final_window_ub = 0.0;
        
        if (!thread_id)
        {
            window_ub_[window_lsb] = window_lb_[window_lsb] = config_.end_time;
        }

        while (running_)
        {
            double local_virtual_time = event_queues_[thread_id].empty() ? config_.end_time : event_queues_[thread_id].top().getTimestamp();
            double final_window_ub = 0.0;

            atomic_min(window_ub_[window_lsb], local_virtual_time + tau);
            atomic_min(window_lb_[window_lsb], local_virtual_time);

            if (!thread_id)
            {
                window_ub_[1 - window_lsb] = window_lb_[1 - window_lsb] = config_.end_time;
            }

            pthread_barrier_wait(&barrier);

            if (window_lb_[window_lsb] >= config_.end_time)
            {
                break;
            }
            
            if (!thread_id)
            {
                stats_.window_racer.window_sizes.push_back(tau);
            }

            if (window_ub_[window_lsb] < 0 or window_lb_[window_lsb] > local_virtual_time)
            {
                //boundary error
            }

            while (true)
            {
                const Event *ev_a = nullptr;

                if (!event_queues_[thread_id].empty() && event_queues_[thread_id].top().getTimestamp() < window_ub_[window_lsb])
                {
                    ev_a = &event_queues_[thread_id].top();
                }
            

                const Event *ev_b = nullptr;

                if (!new_event_queues_[thread_id].empty() && new_event_queues_[thread_id].top().getTimestamp() < window_ub_[window_lsb])
                {
                    ev_b = &new_event_queues_[thread_id].top();
                }

                Event next_event = Event(0, 0, 0, 0);
                bool next_event_was_set = false;

                if (ev_a != nullptr && (ev_b == nullptr || ev_a->getTimestamp() < ev_b->getTimestamp()))
                {
                    next_event = *ev_a;
                    next_event_was_set = true;
                    event_queues_[thread_id].pop();
                }
                else if (ev_b != nullptr)
                {
                    next_event = *ev_b;
                    next_event_was_set = true;
                    new_event_queues_[thread_id].pop();
                }

                if (!next_event_was_set)
                {
                    break;
                }

                handleEvent(next_event, thread_id, window_lsb);
            }

            std::vector<Event>& new_event_queues_container = get_container(new_event_queues_[thread_id]);

            for (auto& ev : new_event_queues_container)
            {
                new_events_list_[thread_id][id_to_tid(ev.getTargetId())].push_back(ev);
            }

            new_event_queues_[thread_id] = std::priority_queue<Event, std::vector<Event>, EventComparator>();
            pthread_barrier_wait(&barrier);

            final_window_ub = window_ub_[window_lsb];

            int num_commited = 0;

            for (uint64_t src_thread = 0; src_thread < config_.num_threads; src_thread++)
            {
                for (auto ev_it = new_events_list_[src_thread][thread_id].begin();
                        ev_it != new_events_list_[src_thread][thread_id].end(); ev_it++)
                {
                    // assert ev_it timestamp > ub
                    if ((*ev_it).getCreationTime() < final_window_ub)
                    {
                        event_queues_[thread_id].push(*ev_it);
                    }
                }
                new_events_list_[src_thread][thread_id].clear();

                // Iterate through the entities that were updated
                for (auto ent_it = dirty_entities_[src_thread][thread_id].begin();
                        ent_it != dirty_entities_[src_thread][thread_id].end(); ent_it++)
                {
                    WindowRacerLP *entity = *ent_it;
                    std::vector<Event> &entity_event_list = entity->getEventList();

                  //  std::cout<<"src_tid: "<< src_thread <<" tid: "<<thread_id<<" dirty: "<<dirty_entities_[src_thread][thread_id].size()<<" entity: "<<entity->getId()<<" events to commit: "<<entity_event_list.size()<<std::endl;
                    // Iterate through the events that were executed in this entity
                    for (auto ev_it = entity_event_list.begin(); ev_it != entity_event_list.end(); ev_it++)
                    {
                        Event &event = *ev_it;

                        //if created after the window upper bound is excluded
                        if (event.getCreationTime() >= final_window_ub)
                        {
                            continue;
                        }

                        //if the timestamp is below the window upper bound, is commited
                        if (event.getTimestamp() < final_window_ub)
                        {
                            num_commited++;
                            continue;
                        }

                        // Creation has been commited but the execution is delayed 
                        // to a later iteration because the timestamp is out of bounds
                        if (event.getTimestamp() >= final_window_ub)
                        {
                            //assert dest_id = entity id
                            event_queues_[thread_id].push(event);
                            continue;
                        }
                    }

                    // if the entity had a change out of bounds,
                    // we restore a state before the window upper bound
                    if (entity->getLastChangeTimestamp() >= final_window_ub)
                    {
                        // assert i >= 0 and <= max_num_prev_states
                        entity->restoreStateWindowEnd(final_window_ub);
                    }

                    // we reset the values for the next iteration
                    
                    logical_processes_[entity->getId()]->setLocalVirtualTime(logical_processes_[entity->getId()]->getLastChangeTimestamp());
                    logical_processes_[entity->getId()]->setLastChangeTimestamp(-1.0);
                    entity->getEventList().clear();
                    logical_processes_[entity->getId()]->setNumSavedStates(0);
                    logical_processes_[entity->getId()]->getSavedStates().clear();
                }
            }

            //calculate window size
            double hindsight_optimal_tau = final_window_ub - window_lb_[window_lsb];

            if (!thread_id)
            {
                stats_.window_racer.windows++;
                //stats_.window_racer.window_sizes.push_back(hindsight_optimal_tau);
            }

            //calculate new tau
            tau = hindsight_optimal_tau * config_.window_racer.window_growth_factor;
            pthread_barrier_wait(&barrier);

            stats_.events_commited_per_lp[thread_id] += num_commited;
            num_commited = 0;
            
            //clear dirty entities
            for (uint64_t src_thread = 0; src_thread < config_.num_threads; src_thread++)
            {
                dirty_entities_[src_thread][thread_id].clear();
            }

            window_lsb = 1 - window_lsb;
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
    stats_.total_events_commited = 0;
    stats_.total_rollbacks = 0;
    stats_.window_racer.immediate_rollbacks = 0;
    stats_.window_racer.window_end_rollbacks = 0;
    stats_.events_processed_per_lp.resize(config_.num_threads);
    stats_.rollbacks_per_lp.resize(config_.num_threads);

    for (size_t i = 0; i < logical_processes_.size(); ++i)
    {
        auto lp = static_cast<WindowRacerLP*>(logical_processes_[i].get());
        stats_.events_processed_per_lp[id_to_tid(lp->getId())] = lp->getProcessedEvents();
        stats_.total_events_processed += lp->getProcessedEvents();
        stats_.total_events_generated += lp->getGeneratedEvents();

        stats_.rollbacks_per_lp[id_to_tid(lp->getId())] = lp->getImmediateRollbacks() + lp->getWindowEndRollbacks();
        stats_.window_racer.immediate_rollbacks += lp->getImmediateRollbacks();
        stats_.window_racer.window_end_rollbacks += lp->getWindowEndRollbacks();
        stats_.total_rollbacks += lp->getImmediateRollbacks() + lp->getWindowEndRollbacks();
    }

    for (size_t i = 0; i < stats_.events_commited_per_lp.size(); ++i)
    {
        stats_.total_events_commited += stats_.events_commited_per_lp[i];
    }

    stats_.efficiency = stats_.total_events_commited / stats_.wall_clock_time;

    running_ = false;
    return stats_;    
}

double WindowRacerEngine::getCurrentTime() const
{
    double min_time = std::numeric_limits<double>::infinity();
    for (const auto& lp : logical_processes_)
    {
        min_time = std::min(min_time, lp->getLocalVirtualTime());
    }
    return min_time;
}

void atomic_min(std::atomic<double>& a, double b)
{
    double current = a.load();
    while (b < current && !a.compare_exchange_weak(current, b))
    {
    }
}

}
