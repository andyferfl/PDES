#include "core/simulation_engine.hpp"
#include "algorithms/sequential_engine.hpp"
#include "algorithms/null_messages_engine.hpp"
#include "algorithms/window_racer_engine.hpp"
#include "algorithms/time_warp_engine.hpp"

#include <iostream>

namespace fusion
{
SimulationEngine::SimulationEngine(const SimulationConfig& config)
    : config_(config)
{
}

const SimulationConfig& SimulationEngine::getConfig() const
{
    return config_;
}

std::unique_ptr<SimulationEngine> SimulationEngine::create(const SimulationConfig& config)
{
    switch (config.algorithm)
    {
    case SimulationAlgorithm::SEQUENTIAL:
        return std::make_unique<SequentialEngine>(config);
    case SimulationAlgorithm::WINDOW_RACER:
        return std::make_unique<WindowRacerEngine>(config);
    case SimulationAlgorithm::TIME_WARP:
        return std::make_unique<TimeWarpEngine>(config);
    case SimulationAlgorithm::NULL_MESSAGES:
        return std::make_unique<NullMessagesEngine>(config);
    default:
        throw std::runtime_error("Unknown simulation algorithm");
    }
}
}
