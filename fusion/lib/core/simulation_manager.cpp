#include "core/simulation_manager.hpp"

namespace fusion
{
SimulationManager::SimulationManager()
    : config_()
{
}

SimulationManager& SimulationManager::setAlgorithm(SimulationAlgorithm algorithm)
{
    config_.algorithm = algorithm;
    return *this;
}

SimulationManager& SimulationManager::setThreadCount(uint32_t num_threads)
{
    config_.num_threads = num_threads;
    return *this;
}

SimulationManager& SimulationManager::setLogicalProcessCount(uint32_t num_lps)
{
    config_.num_logical_processes = num_lps;
    return *this;
}

SimulationManager& SimulationManager::setEndTime(double end_time)
{
    config_.end_time = end_time;
    return *this;
}

SimulationManager& SimulationManager::setDetailedStats(bool enable)
{
    config_.collect_detailed_stats = enable;
    return *this;
}

SimulationManager& SimulationManager::configureWindowRacer(double initial_window_size, double window_growth_factor, int max_states_saved)
{
    config_.window_racer.initial_window_size = initial_window_size;
    config_.window_racer.window_growth_factor = window_growth_factor;
    config_.window_racer.max_states_saved = max_states_saved;
    return *this;
}

SimulationManager& SimulationManager::configureTimeWarp(int gvt_interval, bool lazy_cancellation, int max_states_saved)
{
    config_.time_warp.gvt_interval = gvt_interval;
    config_.time_warp.lazy_cancellation = lazy_cancellation;
    config_.time_warp.max_states_saved = max_states_saved;
    return *this;
}

SimulationManager& SimulationManager::configureNullMessages(double lookahead, bool dynamic_lookahead)
{
    config_.null_messages.dynamic_lookahead = dynamic_lookahead;
    config_.null_messages.loookahead = lookahead;
    return *this;
}

SimulationManager& SimulationManager::registerEntity(std::shared_ptr<Entity> entity)
{
    entities_.push_back(entity);
    
    if (initialized_ && engine_)
    {
        engine_->registerEntity(entity);
    }

    return *this;
}

SimulationManager& SimulationManager::initialize()
{
    engine_ = SimulationEngine::create(config_);
    
    for (const auto& entity : entities_)
    {
        engine_->registerEntity(entity);
    }

    engine_->initialize();

    initialized_ = true;
    return *this;
}

SimulationStats SimulationManager::run()
{
    if (!initialized_)
    {
        initialize();
    }

    return engine_->run();
}

double SimulationManager::getCurrentTime() const
{
    if (engine_)
    {
        return engine_->getCurrentTime();
    }
    return 0.0;
}

const SimulationConfig& SimulationManager::getConfig() const
{
    return config_;
}

SimulationEngine* SimulationManager::getEngine() const
{
    return engine_.get();
}

bool SimulationManager::saveStatisticsToFile(const std::string& filename, const SimulationStats& stats) const
{
    return true;
}

void SimulationManager::printStatistics(const SimulationStats& stats) const
{
    std::cout << "Simulation Statistics\n";
    std::cout << "=====================\n\n";

    std::cout << "Algorithm: " << algorithmToString(config_.algorithm) << "\n";
    std::cout << "Threads: " << config_.num_threads << "\n";
    std::cout << "LPs: " << config_.num_logical_processes << "\n\n";

    std::cout << "Simulation time: " << stats.simulation_time << "\n";
    std::cout << "Wall Clock Time: " << stats.wall_clock_time << " seconds\n";
    std::cout << "Events processed: " << stats.total_events_processed << "\n";
    std::cout << "Event generated: " << stats.total_events_generated << "\n";
    std::cout << "Efficiency: " << stats.efficiency << " events/second\n";

    if (config_.algorithm == SimulationAlgorithm::WINDOW_RACER ||
        config_.algorithm == SimulationAlgorithm::TIME_WARP)
    {
        std::cout << "Rollbacks: " << stats.total_rollbacks << "\n";
    }

    if (config_.algorithm == SimulationAlgorithm::NULL_MESSAGES)
    {
        std::cout << "Null Messages: " << stats.total_null_messages << "\n";
    }
}

std::string SimulationManager::algorithmToString(SimulationAlgorithm algorithm)
{
    switch (algorithm)
    {
    case SimulationAlgorithm::SEQUENTIAL:
        return "Sequential DES";
    case SimulationAlgorithm::WINDOW_RACER:
        return "Window Racer";
    case SimulationAlgorithm::TIME_WARP:
        return "Time Warp";
    case SimulationAlgorithm::NULL_MESSAGES:
        return "Null Messages";    
    default:
        return "Unknown";
    }
}

}
