#include "bus.hpp"
#include <iostream>

std::vector<uint32_t> memory(1 << 24, 0);
std::queue<BusRequest> bus_queue;
int bus_busy_cycles = 0;
int current_initiator = -1;

// Process snooping for other caches
void snoopBus(int initiator_core, uint32_t addr, bool is_write, bool& shared, bool& supplied, std::vector<uint32_t>& data) {
    uint32_t tag, set_index, block_offset;
    parseAddress(addr, caches[0].set_index_bits, caches[0].block_offset_bits, tag, set_index, block_offset);
    uint32_t mem_addr = (addr >> caches[0].block_offset_bits) << caches[0].block_offset_bits;

    supplied = false;

    for (int i = 0; i < 4; ++i) {
        if (i == initiator_core) continue;
        Cache& cache = caches[i];
        auto& set = cache.sets[set_index];

        for (auto& line : set) {
            if (line.state != INVALID && line.tag == tag) {
                if (is_write) {
                    // comes from write hit at SHARED, or write miss
                    // Write: Invalidate other copies
                    if (line.state == MODIFIED) {
                        // Write back to memory
                        // for (size_t j = 0; j < line.data.size(); ++j) {
                        //     memory[mem_addr / 4 + j] = line.data[j];
                        // }
                        global_stats.bus_data_traffic += cache.block_size;
                        // cache.stall_cycles = -1;
                        bus_busy_cycles+=100;
                        // bus_queue.push({initiator_core, addr, is_write, true});
                        cache.idle_cycles += 100; // Writeback to memory
                        cache.writeback_count++;
                    }
                    if (!shared) {supplied = true; shared = true; cache.idle_cycles += 2 * (cache.block_size / 4);} // Send block
                    line.state = INVALID;
                    global_stats.invalidations++;
                } else {
                    // comes from read miss
                    // Read: Supply data if MODIFIED, update states
                    if (line.state == MODIFIED) {
                        //data gets copied to target cache
                        supplied = true;
                        bus_busy_cycles+=100;
                        global_stats.bus_data_traffic += cache.block_size;
                        cache.idle_cycles += 2 * (cache.block_size / 4); // Send block
                        line.state = SHARED;
                        shared = true;
                    } else if (line.state == EXCLUSIVE) {
                        //data gets copied to target cache
                        line.state = SHARED;
                        supplied = true;
                        global_stats.bus_data_traffic += cache.block_size;
                        cache.idle_cycles += 2 * (cache.block_size / 4); // Send block
                        shared = true;
                    } else if (line.state == SHARED) {
                        //data gets copied to target cache
                        supplied = true;
                        global_stats.bus_data_traffic += cache.block_size;
                        if (!shared) cache.idle_cycles += 2 * (cache.block_size / 4); // Send block
                        shared = true;
                    }
                }
            }
        }
    }
}

// Handle cache miss
void handleMiss(int core, uint32_t addr, bool is_write, uint32_t set_index, uint32_t tag) {
    Cache& cache = caches[core];
    auto& set = cache.sets[set_index];
    int victim_index = findLRU(set);
    uint32_t mem_addr = (addr >> cache.block_offset_bits) << cache.block_offset_bits;

    // Evict if necessary
    if (set[victim_index].state == MODIFIED || set[victim_index].state == EXCLUSIVE) {
        cache.eviction_count++;
        if (set[victim_index].state == MODIFIED) {
            // Write back to memory
            // for (size_t j = 0; j < set[victim_index].data.size(); ++j) {
            //     memory[mem_addr / 4 + j] = set[victim_index].data[j];
            // }
            cache.stall_cycles = 100; //change it to a countdown for cycles the core stays occupied for
            bus_busy_cycles+=100;
            bus_queue.push({core, addr, is_write, true});
            cache.writeback_count++;
            global_stats.bus_data_traffic += cache.block_size;
            cache.idle_cycles += 100; // Writeback to memory
            // bus_busy_cycles +=100;
            // cache.stall_cycles =-1;
        }
    }

    // Snoop other caches
    bool shared = false;
    bool supplied = false;
    std::vector<uint32_t> data;
    snoopBus(core, addr, is_write, shared, supplied, data);

    // Fetch block
    cache.miss_count++;
    set[victim_index].tag = tag;
    if (shared) {
        // Data supplied by another cache
        set[victim_index].data = data;
        // cache.idle_cycles += 2 * (cache.block_size / 4); // cycles get updated in snoopBus
    } else {
        // Fetch from memory
        set[victim_index].data.resize(cache.block_size / 4, 0);
        for (size_t j = 0; j < set[victim_index].data.size(); ++j) {
            set[victim_index].data[j] = memory[mem_addr / 4 + j];
        }
        global_stats.bus_data_traffic += cache.block_size;
        cache.idle_cycles += 100; // Memory access
    }
    set[victim_index].state = is_write ? MODIFIED : (shared ? SHARED : EXCLUSIVE);

    if (is_write && set[victim_index].state == MODIFIED) {
        // Update data (simplified: set first word to non-zero)
        set[victim_index].data[0] = 1;
    }

    updateLRU(set, victim_index);
}