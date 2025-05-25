#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <core/simulation_engine.hpp>
#include <core/simulation_manager.hpp>
#include "phold.hpp"

using namespace fusion;

void runPHoldBenchmark(const uint64_t &num_entities, const uint32_t &threads, const double &lookahead, const uint64_t &lps, std::vector<SimulationStats> &stats_list, std::vector<SimulationConfig> &config_list)
{
    std::cout << "Running PHOLD Benchmark Tests\n";
    std::cout << "============================\n\n";
    
    // Parameters
    const double remote_probability = 0.5;
    const double zero_delay_probability = 0.1;
    const double mean_delay = 1.0;
    const uint32_t initial_events = 1;
    const double end_time = 1000.0;
    const uint64_t seed = 12345;
    std::string comparison = "";
    
    // Create entities
    auto entities = createPHoldModel(
        num_entities, 
        remote_probability, 
        zero_delay_probability, 
        lookahead, 
        mean_delay, 
        initial_events, 
        seed);
    
    // Run with sequential algorithm
    {
        SimulationManager manager;
        manager.setAlgorithm(SimulationAlgorithm::SEQUENTIAL)
               .setEndTime(end_time)
               .setDetailedStats(true);
        
        for (const auto& entity : entities)
        {
            manager.registerEntity(entity);
        }
        
        std::cout << "Running Sequential DES...\n";
        auto stats = manager.run();
        manager.printStatistics(stats);
        std::cout << "\n";
        manager.saveStatisticsToFile("des.txt", stats);
        stats_list.push_back(stats);
        config_list.push_back(manager.getConfig());
    }
        
    // Run with Null Message algorithm
    {
        SimulationManager manager;
        manager.setAlgorithm(SimulationAlgorithm::NULL_MESSAGES)
               .setThreadCount(threads)
               .setLogicalProcessCount(lps)
               .setEndTime(end_time)
               .setDetailedStats(true)
               .configureNullMessages(lookahead, false);
        
        for (const auto& entity : entities)
        {
            manager.registerEntity(entity);
        }
        
        std::cout << "Running Null Messages PDES...\n";
        auto stats = manager.run();
        manager.printStatistics(stats);
        std::cout << "\n";
        manager.saveStatisticsToFile("nm.txt", stats);
        stats_list.push_back(stats);
        config_list.push_back(manager.getConfig());
    }

    // Run with Window racer algorithm
    {
        SimulationManager manager;
        manager.setAlgorithm(SimulationAlgorithm::WINDOW_RACER)
               .setThreadCount(threads)
               .setEndTime(end_time)
               .setDetailedStats(true)
               .configureWindowRacer(num_entities,lookahead,100.0,1000);
        
        for (const auto& entity : entities)
        {
            manager.registerEntity(entity);
        }
        
        std::cout << "Running Window Racer PDES...\n";
        auto stats = manager.run();
        manager.printStatistics(stats);
        std::cout << "\n";
        manager.saveStatisticsToFile("wr.txt", stats);
        stats_list.push_back(stats);
        config_list.push_back(manager.getConfig());
    }
    // Run with TIME WARP algorithm
    {
        SimulationManager manager;
        manager.setAlgorithm(SimulationAlgorithm::TIME_WARP)
               .setThreadCount(threads)
               .setLogicalProcessCount(lps)
               .setEndTime(end_time)
               .setDetailedStats(true);

        for (const auto& entity : entities)
        {
            manager.registerEntity(entity);
        }

        std::cout << "Running Time Warp PDES...\n";
        auto stats = manager.run();
        manager.printStatistics(stats);
        std::cout << "\n";
        manager.saveStatisticsToFile("output.txt", stats);

        stats_list.push_back(stats);
        config_list.push_back(manager.getConfig());
    }
    
}


int main()
{
    std::cout << "DES/PDES PHold Test Program\n";
    std::cout << "============================\n\n";
    
    std::ofstream file("test.csv");

    if (!file.is_open())
    {
        return false;
    }
    file << "iteration;algorithm;threads;lps;entities;lookahead;end_time;actual_end_time;commited_events;execution_time;efficiency"<<std::endl;

    uint64_t entities = 1024;
    uint32_t threads = 4;
    double lookahead = 0.5;
    uint64_t lps = 100;

    for (int i = 0; i < 100 ; ++i)
    {
        std::vector<SimulationStats> stats_list;
        std::vector<SimulationConfig> config_list;
        runPHoldBenchmark(entities, threads, lookahead, lps, stats_list, config_list);
        
       auto stats = stats_list.begin();
       auto config = config_list.begin();

       while (stats != stats_list.end() && config != config_list.end()) {
            uint64_t commited = config->algorithm == SimulationAlgorithm::WINDOW_RACER ? stats->total_events_commited : stats->total_events_processed;

            file << i << ";" << SimulationManager::algorithmToString(config->algorithm) << ";" << config->num_threads << ";" 
                << config->num_logical_processes << ";" << entities << ";" << lookahead << ";" << config->end_time << ";" 
                << stats->simulation_time << ";" << commited << ";" << stats->wall_clock_time << ";" 
                << stats->efficiency << std::endl;

           ++stats;
           ++config;
        }

    }



    std::cout << "\n";
        
    return 0;
}