#ifndef PHOLD_HPP
#define PHOLD_HPP

#include <cstdint>
#include <memory>
#include <random>
#include <vector>
#include <core/entity.hpp>
#include <core/event.hpp>

namespace fusion
{
class PHoldEntity : public Entity
{
public:
    PHoldEntity(
        uint64_t id,
        uint64_t num_entities,
        double remote_probability,
        double zero_delay_probability,
        double lookahead,
        double mean_delay,
        uint32_t initial_events,
        uint64_t seed,
        uint64_t computing
    );

    std::vector<Event> initialize() override;
    std::vector<Event> handleEvent(const Event& event, double current_time) override;
    PHoldEntity& saveState(double time) override;
    void restoreState(const PHoldEntity& state, double time);

private:
    uint64_t num_entities_;
    double remote_probability_;
    double zero_delay_probability_;
    double lookahead_;
    double mean_delay_;
    uint32_t initial_events_;
    uint64_t computing_;

    std::mt19937_64 rng_;
    std::uniform_real_distribution<double> uniform_dist_;
    std::exponential_distribution<double> delay_dist_;
    std::uniform_int_distribution<uint64_t> entity_dist_;
    
    uint64_t events_processed_;
    uint64_t events_generated_;
    
    Event generateEvent(double current_time); 
};

std::vector<std::shared_ptr<Entity>> createPHoldModel(
    uint64_t num_entities,
    double remote_probability = 0.5,
    double zero_delay_probability = 0.0,
    double lookahead = 0.1,
    double mean_delay = 1.0,
    uint32_t initial_events = 1,
    uint64_t seed = 12345,
    uint64_t computing = 1
);

}


#endif
