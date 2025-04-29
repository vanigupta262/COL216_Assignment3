#include "bus.hpp"
#include <iostream>

std::queue<BusRequest> bus_queue;
int bus_busy_cycles = 0;
int current_initiator = -1;

// Process snooping for other caches
void snoopBus(int initiator_core, uint32_t addr, bool is_write, bool& shared, bool& supplied) {
    uint32_t tag, set_index, block_offset;
    parseAddress(addr, caches[0].set_index_bits, caches[0].block_offset_bits, tag, set_index, block_offset);

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
                        global_stats.bus_data_traffic += cache.block_size;
                        bus_busy_cycles+=100;
                        cache.stall_cycles = 100-1;
                        cache.idle_cycles += 100;
                        cache.writeback_count++;
                    }
                    if (!shared) {
                        cache.idle_cycles += 2 * (cache.block_size / 4);
                        supplied = true;
                        shared = true;
                    } // Send block
                    line.state = INVALID;
                    global_stats.invalidations++;
                } else {
                    // comes from read miss
                    // Read: Supply data if MODIFIED, update states
                    if (line.state == MODIFIED) {
                        //data gets copied to target cache
                        cache.idle_cycles += 2 * (cache.block_size / 4); // Send block
                        bus_busy_cycles+=100;
                        global_stats.bus_data_traffic += cache.block_size;
                        line.state = SHARED;
                        supplied = true;
                        shared = true;
                    } else {
                        //data gets copied to target cache
                        if (!shared) cache.idle_cycles += 2 * (cache.block_size / 4); // Send block
                        global_stats.bus_data_traffic += cache.block_size;
                        line.state = SHARED;
                        supplied = true;
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
            cache.stall_cycles = 100; //change it to a countdown for cycles the core stays occupied for
            bus_busy_cycles += 100;
            bus_queue.push({core, addr, is_write, true});
            cache.writeback_count++;
            global_stats.bus_data_traffic += cache.block_size;
            cache.idle_cycles += 100; // Writeback to memory
        }
    }

    // Snoop other caches
    bool shared = false;
    bool supplied = false;
    snoopBus(core, addr, is_write, shared, supplied);

    // Fetch block
    cache.miss_count++;
    set[victim_index].tag = tag;
    if (supplied) {
        // Data supplied by another cache
        cache.idle_cycles += 2 * (cache.block_size / 4);
    } else {
        // Fetch from memory
        global_stats.bus_data_traffic += cache.block_size;
        cache.idle_cycles += 100; // Memory access
    }
    set[victim_index].state = is_write ? MODIFIED : (supplied ? SHARED : EXCLUSIVE);

    updateLRU(set, victim_index);
}