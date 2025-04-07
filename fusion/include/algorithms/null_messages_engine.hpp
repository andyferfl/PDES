#ifndef NULL_MESSAGES_ENGINE_HPP
#define NULL_MESSAGES_ENGINE_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <queue>
#include "core/entity.hpp"
#include "core/simulation_engine.hpp"
#include "core/logical_process.hpp"

namespace fusion
{
class NullMessagesEngine : public SimulationEngine
{
public:
    explicit NullMessagesEngine(const SimulationConfig& config);
    ~NullMessagesEngine();
    void registerEntity(std::shared_ptr<Entity> entity) override;
    void initialize() override;
    SimulationStats run() override;
    double getCurrentTime() const override;

private:
    class NullMessagesLP;
    std::vector<std::unique_ptr<NullMessagesLP>> logical_processes_;
    std::vector<std::thread> worker_threads_;

    void sendNullMessages(uint32_t lp_id);
};
}

#endif
