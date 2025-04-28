#ifndef BUS_HPP
#define BUS_HPP

#include <vector>
#include <queue>
#include "cache.hpp"

struct BusRequest {
    int core;
    uint32_t addr;
    bool is_write;
    
};

extern std::vector<Cache> caches;
extern Stats global_stats;
extern std::vector<uint32_t> memory;
extern std::queue<BusRequest> bus_queue; 
extern int bus_busy_cycles;
extern int current_initiator;

void snoopBus(int initiator_core, uint32_t addr, bool is_write, bool& shared, bool& supplied, std::vector<uint32_t>& data);
void handleMiss(int core, uint32_t addr, bool is_write, uint32_t set_index, uint32_t tag);

#endif
