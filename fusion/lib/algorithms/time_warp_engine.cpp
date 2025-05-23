#include "algorithms/time_warp_engine.hpp"
#include <chrono>
#include <iostream>
#include <atomic>
#include <pthread.h>
#include <limits>
#include <algorithm>
#include <map>
#include <fstream>
#include <cmath>
#include <unordered_map> 
#include <unordered_set>
std::ofstream debug_log("timewarp_debug.log");


namespace fusion {

    TimeWarpEngine::TimeWarpLP::TimeWarpLP(uint32_t id, uint32_t max_states_saved, bool lazy_cancellation, TimeWarpEngine* engine)
    : LogicalProcess(id), 
      max_states_saved_(max_states_saved),
      lazy_cancellation_(lazy_cancellation),
      engine_(engine)  {}

    
    bool TimeWarpEngine::TimeWarpLP::processNextEvent() {
        try {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (event_queue_.empty()) {
                return false;
            }

            Event event = event_queue_.top(); 
            event_queue_.pop();
            lock.unlock();

            if (event.isAntiMessage()) {
                cancelEvent(event);
                return true;
            }
    
            uint32_t correct_lp = engine_->getAssignedLpForEntity(event.getTargetId());
            if (correct_lp != getId()) {
                engine_->logical_processes_[correct_lp]->scheduleEvent(event);
                return true;
            }
    
            // 1. Проверка временной метки
            if (event.getTimestamp() < getLocalVirtualTime()) {
                rollback(event.getTimestamp());
                std::lock_guard<std::mutex> relock(queue_mutex_);
                event_queue_.push(event); // заново вставить событие
            }

            // 3. Обновление LVT
            setLocalVirtualTime(event.getTimestamp());

            // 4. Получение целевой сущности
            auto entity = getEntity(event.getTargetId());
            if (!entity) {
                std::cerr << "ERROR: Entity " << event.getTargetId() 
                        << " not found in LP " << getId() 
                        << " (known entities: ";
                for (const auto& e : getEntities()) {
                    std::cerr << e.first << " ";
                }
                std::cerr << ")" << std::endl;
                return true;
            }

            // 5. Сохранение состояния и обработка
            saveEntityState(entity, event.getTimestamp());
            auto new_events = entity->handleEvent(event, getLocalVirtualTime());
            processed_event_history_.push_back(event);

            processed_events_++;
            generated_events_ += new_events.size(); // Подсчет сгенерированных событий

            // 7. Планирование новых событий
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                for (const auto& e : new_events) {
                    // uint32_t target_lp = e.getReceivingLP();
                    // if (target_lp == getId()) {
                    //     // Local event
                    //     event_queue_.push(e);
                    // } else {
                    //     // Remote event
                    //     pending_remote_events_[target_lp].push_back(e);
                    //     output_event_history_.push_back(e);
                    // }
        
                    if (event_queue_.size() >= 100000) {
                        throw std::runtime_error("Event queue overflow");
                    }
                    event_queue_.push(e);
                }
                output_event_history_.push_back(event);

            }

            return true;
    
        } catch (const std::exception& e) {
            std::cerr << "Error in LP " << getId() << ": " << e.what() << std::endl;
            return false;
        }
    }
    
// Вынесенная функция дампа очереди
void TimeWarpEngine::TimeWarpLP::dumpEventQueue() {
    std::ofstream dump("event_queue_dump_" + std::to_string(getId()) + ".txt");
    auto temp_queue = event_queue_;
    while (!temp_queue.empty()) {
        auto e = temp_queue.top();
        dump << "Time: " << e.getTimestamp() 
                << " | Type: " << e.getTypeId()
                << " | Src: " << e.getSourceId()
                << " | Dest: " << e.getTargetId() << "\n";
        temp_queue.pop();
    }
}

void TimeWarpEngine::TimeWarpLP::scheduleEvent(const Event& event) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (event_queue_.size() >= 100000) {
        throw std::runtime_error("Event queue overflow");
    }
    event_queue_.push(event);

}

void TimeWarpEngine::TimeWarpLP::deliverRemoteEvents(std::vector<TimeWarpLP*>& all_lps) {
    std::lock_guard<std::mutex> lock(pending_events_mutex_);
    
    for (auto& [receiver_id, events] : pending_remote_events_) {
        // Используем стандартный алгоритм распределения
        uint32_t target_lp = receiver_id % all_lps.size();
        
        if (target_lp < all_lps.size()) {
            for (const auto& event : events) {
                all_lps[target_lp]->scheduleEvent(event);
            }
        } else {
            std::cerr << "Invalid LP index for entity " << receiver_id << std::endl;
        }
        events.clear();
    }
}

void TimeWarpEngine::TimeWarpLP::rollback(double rollback_time) {
    // Increment rollback counter
    // 1. Найти последнее состояние перед rollback_time
    std::lock_guard<std::mutex> lock(queue_mutex_);
    rollbacks_++;
    debug_log << "LP " << getId() << " ROLLBACK from " 
              << getLocalVirtualTime() << " to " << rollback_time << "\n";

    // 1. Find nearest state before rollback_time
    auto state_it = saved_states_.upper_bound(rollback_time);
    if (state_it == saved_states_.begin()) {
        return;
    }
    --state_it;

    // 2. Восстанавливаем состояния сущностей
    for (const auto& [entity_id, state] : state_it->second) {
        if (auto entity = getEntity(entity_id)) {
            entity->restoreState(*state, state_it->first);
        }
    }

    // 3. Создаем анти-сообщения
    std::vector<Event> new_output_history;
    {
        std::lock_guard<std::mutex> pending_lock(pending_events_mutex_);
        for (const auto& sent_event : output_event_history_) {
            if (sent_event.getTimestamp() > rollback_time) {
                // Создаем анти-сообщение через конструктор
                Event anti_msg(
                    -sent_event.getTimestamp(),  // отрицательное время
                    sent_event.getSourceId(),
                    sent_event.getTargetId(),
                    sent_event.getTypeId(),
                    sent_event.getData()
                );
                anti_msg.setAntiMessage(true);
                anti_msg.setSendingLP(sent_event.getSendingLP());
                anti_msg.setReceivingLP(sent_event.getReceivingLP());

                pending_remote_events_[sent_event.getReceivingLP()].push_back(anti_msg);
                engine_->stats_.time_warp.anti_messages++;   
            } else {
                new_output_history.push_back(sent_event);
            }
        }
        output_event_history_ = std::move(new_output_history);
    }

    // 4. Перестраиваем очередь событий
    std::priority_queue<Event, std::vector<Event>, EventComparator> new_queue;
    std::vector<Event> kept_events;
    
    while (!event_queue_.empty()) {
        Event e = event_queue_.top();
        event_queue_.pop();
        
        if (e.getTimestamp() <= rollback_time || 
           (e.isAntiMessage() && -e.getTimestamp() <= rollback_time)) {
            kept_events.push_back(e);
        }
    }
    
    for (const auto& e : kept_events) {
        new_queue.push(e);
    }
    event_queue_ = std::move(new_queue);

    // 5. Обновляем LVT
    setLocalVirtualTime(rollback_time);
}

void TimeWarpEngine::TimeWarpLP::cancelEvent(const Event& anti_message) {
    // Find and remove the corresponding positive message
    std::lock_guard<std::mutex> lock(queue_mutex_);
    std::vector<Event> kept_events;
    bool found = false;
    
    while (!event_queue_.empty()) {
        Event e = event_queue_.top();
        event_queue_.pop();
        
        // Сравниваем абсолютные значения времени
        double event_time = e.isAntiMessage() ? -e.getTimestamp() : e.getTimestamp();
        double anti_time = -anti_message.getTimestamp();
        
        if (!found && !e.isAntiMessage() &&
            e.getSourceId() == anti_message.getSourceId() &&
            e.getTargetId() == anti_message.getTargetId() &&
            e.getTypeId() == anti_message.getTypeId() &&
            std::abs(event_time - anti_time) < 1e-9) {  // сравнение с плавающей точкой
            found = true; // Пропускаем это событие (оно отменено)
        } else {
            kept_events.push_back(e);
        }
    }
    
    // Восстанавливаем очередь
    for (const auto& e : kept_events) {
        event_queue_.push(e);
    }
}

void TimeWarpEngine::TimeWarpLP::saveEntityState(std::shared_ptr<Entity> entity, double time) {
    // Save entity state at this time
    Entity* state = &entity->saveState(time);
    
    // Store in saved states map
    saved_states_[time][entity->getId()] = state;
    
    // Limit the number of saved states
    while (saved_states_.size() > max_states_saved_) {
        saved_states_.erase(saved_states_.begin());
    }
}

void TimeWarpEngine::TimeWarpLP::fossilCollect(double gvt) {
    // Remove saved states older than GVT
    auto it = saved_states_.begin();
    while (it != saved_states_.end() && it->first < gvt) {
        it = saved_states_.erase(it);
    }
    
    // Remove processed events older than GVT
    for (auto it = processed_event_history_.begin(); it != processed_event_history_.end();) {
        if (it->getTimestamp() < gvt) {
            it = processed_event_history_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Remove output events older than GVT
    for (auto it = output_event_history_.begin(); it != output_event_history_.end();) {
        if (it->getTimestamp() < gvt) {
            it = output_event_history_.erase(it);
        } else {
            ++it;
        }
    }
}


uint32_t TimeWarpEngine::TimeWarpLP::getProcessedEvents() const {
    return processed_events_;
}

uint32_t TimeWarpEngine::TimeWarpLP::getRollbacks() const {
    return rollbacks_;
}

uint32_t TimeWarpEngine::TimeWarpLP::getGeneratedEvents() const {
    return generated_events_;
}

bool TimeWarpEngine::TimeWarpLP::eventQueueEmpty() const {
    return event_queue_.empty();
}

double TimeWarpEngine::TimeWarpLP::peekNextEventTime() const {
    return event_queue_.empty() ? std::numeric_limits<double>::infinity() 
                                : event_queue_.top().getTimestamp();
}

const auto& TimeWarpEngine::TimeWarpLP::getPendingRemoteEvents() const { 
    return pending_remote_events_; 
}

size_t TimeWarpEngine::TimeWarpLP::getEventQueueSize() const { 
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return event_queue_.size(); 
}


// TimeWarpEngine implementation
TimeWarpEngine::TimeWarpEngine(const SimulationConfig& config)
    : SimulationEngine(config), global_virtual_time_(0.0) {
    
    // Create logical processes
    if (config.num_logical_processes == 0) {
        throw std::invalid_argument("Number of LPs cannot be zero");
    }
    if (config.time_warp.max_states_saved == 0) {
        throw std::invalid_argument("max_states_saved cannot be zero");
    }

    logical_processes_.resize(config.num_logical_processes);
    for (uint32_t i = 0; i < config.num_logical_processes; ++i) {
        logical_processes_[i] = std::make_unique<TimeWarpLP>(i, 
            config.time_warp.max_states_saved, 
            config.time_warp.lazy_cancellation,
            this);
    }
    for (auto& lp : logical_processes_) {
        lp->setEngine(this);
    }

}

void TimeWarpEngine::registerEntity(std::shared_ptr<Entity> entity) {
    if (config_.num_logical_processes == 0) {
        throw std::runtime_error("Number of LPs cannot be zero");
    }

    uint32_t lp_id = entity->getId() % config_.num_logical_processes;
    logical_processes_[lp_id]->addEntity(entity);
    
    // Сохраняем mapping entity_id -> lp_id для быстрого поиска
    entity_registry_[entity->getId()] = {lp_id, entity};
    entity_to_lp_[entity->getId()] = lp_id;
}

void TimeWarpEngine::initialize() {
    // srand(static_cast<unsigned>(time(nullptr)));

    if (logical_processes_.empty()) {
        throw std::runtime_error("No Logical Processes initialized");
    }

    // std::cout << "Initializing TimeWarp with " << logical_processes_.size() << " LPs\n";

    // 2. Инициализация LP и сущностей
    for (auto& lp : logical_processes_) {
        lp->initializeEntities();
    }

    // 2. Создание начальных событий с гарантией существования получателей
    std::vector<uint64_t> entity_ids;
    for (const auto& [id, _] : entity_registry_) {
        entity_ids.push_back(id);
    }

    double base_time = 0.1;
    std::cout << "Entities registered: " << entity_registry_.size() << std::endl;

    for (auto& [entity_id, info] : entity_registry_) {
        uint64_t sender_id = entity_id;
        uint64_t receiver_id;
        
        if (rand() % 2 == 0 && entity_ids.size() > 1) {
            do {
                receiver_id = entity_ids[rand() % entity_ids.size()];
            } while (receiver_id == sender_id);
        } else {
            auto& lp_entities = logical_processes_[info.lp_id]->getEntities();
            if (lp_entities.size() > 1) {
                auto it = lp_entities.begin();
                do {
                    it = lp_entities.begin();
                    std::advance(it, rand() % lp_entities.size());
                    receiver_id = it->first;
                } while (receiver_id == sender_id);
            } else {
                receiver_id = sender_id;
            }
        }

        // Добавляем небольшую случайную задержку к базовому времени
        double delay = 0.01 * (rand() % 10);
        Event event(base_time + delay, sender_id, receiver_id, 1, {});
        logical_processes_[info.lp_id]->scheduleEvent(event);
    }

    stats_ = SimulationStats();
    global_virtual_time_ = 0.0;
}

SimulationStats TimeWarpEngine::run() {

    std::ofstream debug_log("timewarp_debug.log");
    if (!debug_log.is_open()) {
        std::cout << "ERROR: Failed to open debug log file!" << std::endl;
        return {};
    }
    // std::cout << "Initial event counts per LP:\n";
    for (auto& lp : logical_processes_) {
        debug_log << "LP " << lp->getId() << ": " 
                 << static_cast<TimeWarpLP*>(lp.get())->getEventQueueSize() 
                 << " events\n";
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    running_ = true;
    
    // Вывод конфигурации
    debug_log << "Starting Time Warp with:\n"
              << "- Threads: " << config_.num_threads << "\n"
              << "- LPs: " << logical_processes_.size() << "\n"
              << "- GVT interval: " << config_.time_warp.gvt_interval << "\n"
              << "- End time: " << config_.end_time << std::endl;
    
    debug_log << "==== TIME WARP STARTED ====\n"
              << "Config: threads=" << config_.num_threads 
              << " lps=" << logical_processes_.size() 
              << " end_time=" << config_.end_time << "\n";

    // Инициализация барьера
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, nullptr, config_.num_threads);
    
    // Мьютекс для синхронизации
    static std::mutex log_mutex;
    std::atomic<bool> global_progress{false};

    auto thread_func = [this, &barrier, &debug_log, &global_progress](uint32_t thread_id) {
        std::vector<TimeWarpLP*> my_lps;
        for (uint32_t lp_id = thread_id; lp_id < logical_processes_.size(); lp_id += config_.num_threads) {
            my_lps.push_back(static_cast<TimeWarpLP*>(logical_processes_[lp_id].get()));
        }
        
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            debug_log << "Thread " << thread_id << " started (assigned LPs: ";
            for (auto lp : my_lps) debug_log << lp->getId() << " ";
            debug_log << ")\n";
        }

        uint64_t event_count = 0;
        auto last_status_time = std::chrono::steady_clock::now();
        
        while (running_.load()) {
            bool local_progress = false;

            // Обработка событий
            for (auto lp : my_lps) {
                if (lp->processNextEvent()) {
                    local_progress = true;
                    event_count++;
                    
                    if (event_count % 1000 == 0) {
                        std::lock_guard<std::mutex> lock(log_mutex);
                        debug_log << "Thread " << thread_id << " processed " 
                                 << event_count << " events (LP " << lp->getId() 
                                 << " LVT: " << lp->getLocalVirtualTime() << ")\n";
                    }
                }
            }

            if (local_progress) {
                global_progress.store(true);
            }

            // Барьер 1: синхронизация перед доставкой сообщений
            pthread_barrier_wait(&barrier);
            // Проверка завершения
            if (global_virtual_time_.load() >= config_.end_time) {
                std::cout << "Termination condition met:\n"
                    << "GVT = " << global_virtual_time_.load() 
                    << ", End Time = " << config_.end_time << "\n";

                running_.store(false);
                pthread_barrier_wait(&barrier);
                break;
            }
            
            // Доставка сообщений
            for (auto lp : my_lps) {
                lp->deliverRemoteEvents(
                    reinterpret_cast<std::vector<TimeWarpLP*>&>(logical_processes_)
                );
            }

            // Барьер 2: синхронизация после доставки
            pthread_barrier_wait(&barrier);

            // Расчет GVT (только thread 0)
            if (thread_id == 0) {
                // Проверка отсутствия прогресса
                if (!global_progress.load()) {
                    bool all_empty = true;
                    for (const auto& lp : logical_processes_) {
                        if (!static_cast<TimeWarpLP*>(lp.get())->eventQueueEmpty()) {
                            all_empty = false;
                            break;
                        }
                    }
                    
                    if (all_empty) {
                        std::lock_guard<std::mutex> lock(log_mutex);
                        debug_log << "Termination detected - all queues empty\n";
                        running_.store(false);
                    }
                }

                // Периодический расчет GVT
                if (config_.time_warp.gvt_interval > 0 && 
                    event_count % static_cast<uint64_t>(config_.time_warp.gvt_interval) == 0) {
                    calculateGVT();
                    stats_.time_warp.gvt_calculations++;
                    
                    double gvt = global_virtual_time_.load();
                    for (auto& lp : logical_processes_) {
                        // std::cout << "fossilCollect" << std::endl;
                        static_cast<TimeWarpLP*>(lp.get())->fossilCollect(gvt);
                    }
                    stats_.time_warp.fossil_collections++; 
                }

                global_progress.store(false); // Сброс флага прогресса
            }
            
            // Барьер 3: финальная синхронизация
            pthread_barrier_wait(&barrier);
        }
    };

    // Запуск потоков
    worker_threads_.resize(config_.num_threads);
    for (uint32_t i = 0; i < config_.num_threads; ++i) {
        worker_threads_[i] = std::thread(thread_func, i);
    }

    // Ожидание завершения
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Завершение работы
    pthread_barrier_destroy(&barrier);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    double max_lvt = 0.0;
    bool all_completed = true;  // Объявляем переменную здесь

    for (const auto& lp : logical_processes_) {
        double lvt = lp->getLocalVirtualTime();
        max_lvt = std::max(max_lvt, lvt);
        
        bool lp_completed = (lvt >= config_.end_time);
        auto* tw_lp = static_cast<TimeWarpLP*>(lp.get());
        if (!lp_completed && !tw_lp->eventQueueEmpty()) {
            double next_event = tw_lp->peekNextEventTime();
            lp_completed = next_event >= config_.end_time;
        }
        all_completed = all_completed && lp_completed;  
    }

    // Устанавливаем время симуляции
    // stats_.simulation_time = all_completed ? config_.end_time : max_lvt;
    // Сбор статистики
    stats_.simulation_time = global_virtual_time_.load();
    stats_.wall_clock_time = elapsed.count();
    stats_.total_events_processed = 0;
    stats_.total_events_generated = 0;
    stats_.total_rollbacks = 0;

    stats_.events_processed_per_lp.resize(logical_processes_.size());
    stats_.rollbacks_per_lp.resize(logical_processes_.size());

    // Детальная статистика по LP
    std::cout << "\n=== Simulation Results ===\n";
    debug_log << "\n=== Final Statistics ===\n";
    
    for (size_t i = 0; i < logical_processes_.size(); ++i) {
        auto lp = static_cast<TimeWarpLP*>(logical_processes_[i].get());
        // stats_.events_processed_per_lp.push_back(lp->getProcessedEvents());
        // stats_.rollbacks_per_lp.push_back(lp->getRollbacks());
        stats_.events_processed_per_lp[i] = lp->getProcessedEvents();
        stats_.rollbacks_per_lp[i] = lp->getRollbacks();
        stats_.total_events_processed += lp->getProcessedEvents();
        stats_.total_rollbacks += lp->getRollbacks();
        stats_.total_events_generated += lp->getGeneratedEvents();
                  
        debug_log << "LP " << i << ": events=" << lp->getProcessedEvents()
                  << ", rollbacks=" << lp->getRollbacks()
                  << ", final LVT=" << lp->getLocalVirtualTime() << "\n";
    }

    stats_.efficiency = (stats_.wall_clock_time > 0) ? 
        stats_.total_events_processed / stats_.wall_clock_time : 0;
    
    debug_log << "\n=== Simulation Results ===\n"
          << "Requested end time: " << config_.end_time << "\n"
          << (all_completed ? "All LPs completed successfully" 
                           : "Simulation stopped early") << "\n"
          << "Max LVT achieved: " << max_lvt << "\n"
          << "GVT at stop: " << global_virtual_time_.load() << "\n"
          << "Total events: " << stats_.total_events_processed << "\n"
          << "Wall clock time: " << stats_.wall_clock_time << "s\n";

    
    debug_log.close();
    return stats_;
}

double TimeWarpEngine::getCurrentTime() const {
    return global_virtual_time_.load();
}

uint32_t TimeWarpEngine::getAssignedLpForEntity(uint64_t entity_id)  {
    auto it = entity_to_lp_.find(entity_id);
    if (it != entity_to_lp_.end()) {
        return it->second;
    }
    throw std::runtime_error("Entity not registered");
}

void TimeWarpEngine::calculateGVT() {

    double min_time = std::numeric_limits<double>::max();
    
    // 1. Найти минимальное LVT среди всех LP
    for (const auto& lp : logical_processes_) {
        double lvt = static_cast<TimeWarpLP*>(lp.get())->getLocalVirtualTime();
        // std::cout << "[GVT] LP LVT: " << lvt << std::endl;
        min_time = std::min(min_time, lvt);
    }
    
    // 2. Учесть pending remote events
    for (const auto& lp : logical_processes_) {
        auto events = static_cast<TimeWarpLP*>(lp.get())->getPendingRemoteEvents();
        for (const auto& [_, ev_list] : events) {
            for (const auto& ev : ev_list) {
                double ts = ev.getTimestamp();
                // std::cout << "[GVT] Pending event ts: " << ts << std::endl;
                min_time = std::min(min_time, ts);

            }
        }
    }
    // std::cout << "[GVT] Calculated min_time: " << min_time << std::endl;

    global_virtual_time_ = min_time;

    // if (min_time > global_virtual_time_) {
    //     global_virtual_time_ = min_time;
    // }

    // 4. Check termination
    if (global_virtual_time_ >= config_.end_time) {
        running_.store(false);
    }
}

void TimeWarpEngine::performFossilCollection(double gvt) {
    for (auto& lp : logical_processes_) {
        static_cast<TimeWarpLP*>(lp.get())->fossilCollect(gvt);
    }
}
} // namespace fusion