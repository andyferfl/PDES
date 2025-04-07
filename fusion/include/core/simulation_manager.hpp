#ifndef SIMULATION_MANAGER_HPP
#define SIMULATION_MANAGER_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include "core/simulation_engine.hpp"

namespace fusion
{
class SimulationManager
{
public:
    SimulationManager();
    SimulationManager& setAlgorithm(SimulationAlgorithm algorithm);
    SimulationManager& setThreadCount(uint32_t num_threads);
    SimulationManager& setLogicalProcessCount(uint32_t num_lps);
    SimulationManager& setEndTime(double end_time);
    SimulationManager& setDetailedStats(bool enable);
    SimulationManager& configureWindowRacer(double initial_window_size = 1e-4, double window_growth_factor = 100.0, int max_states_saved = 10);
    SimulationManager& configureTimeWarp(int gvt_interval = 1000, bool lazy_cancellation = false, int max_states_saved = 10);
    SimulationManager& configureNullMessages(double lookahead = 0.1, bool dynamic_lookahead = false);
    SimulationManager& registerEntity(std::shared_ptr<Entity> entity);
    SimulationManager& initialize();
    SimulationStats run();
    double getCurrentTime() const;
    const SimulationConfig& getConfig() const;
    SimulationEngine* getEngine() const;
    bool saveStatisticsToFile(const std::string& filename, const SimulationStats& stats) const;
    void printStatistics(const SimulationStats& stats) const;
    static std::string algorithmToString(SimulationAlgorithm algorithm);
private:
    SimulationConfig config_;
    std::unique_ptr<SimulationEngine> engine_;
    std::vector<std::shared_ptr<Entity>> entities_;
    bool initialized_ = false;
};

}

#endif