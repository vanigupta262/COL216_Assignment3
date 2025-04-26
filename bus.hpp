#ifndef BUS_HPP
#define BUS_HPP

#include <vector>
#include "cache.hpp"

extern std::vector<Cache> caches;
extern Stats global_stats;
extern std::vector<uint32_t> memory;

// Process snooping for other caches
void snoopBus(int initiator_core, uint32_t addr, bool is_write, bool& shared, bool& supplied, std::vector<uint32_t>& data);

// Handle cache miss
void handleMiss(int core, uint32_t addr, bool is_write, uint32_t set_index, uint32_t tag);

#endif