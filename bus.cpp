#include "bus.hpp"

// Process snooping for other caches
void snoopBus(int initiator_core, uint32_t addr, bool is_write, bool& shared) {
    uint32_t tag, set_index, block_offset;
    parseAddress(addr, caches[0].set_index_bits, caches[0].block_offset_bits, tag, set_index, block_offset);

    for (int i = 0; i < 4; ++i) {
        if (i == initiator_core) continue;
        Cache& cache = caches[i];
        auto& set = cache.sets[set_index];

        for (auto& line : set) {
            if (line.state != INVALID && line.tag == tag) {
                if (is_write) {
                    if (line.state == MODIFIED) {
                        global_stats.bus_data_traffic += cache.block_size;
                        cache.idle_cycles += 2 * (cache.block_size / 4); // Send block
                    }
                    line.state = INVALID;
                    global_stats.invalidations++;
                    if (line.dirty) {
                        cache.writeback_count++;
                    }
                } else {
                    if (line.state == MODIFIED) {
                        global_stats.bus_data_traffic += cache.block_size;
                        cache.idle_cycles += 2 * (cache.block_size / 4); // Send block
                        line.state = SHARED;
                        line.dirty = false;
                        shared = true;
                    } else if (line.state == EXCLUSIVE) {
                        line.state = SHARED;
                        shared = true;
                    }
                }
            }
        }
    }
}

// Handle cache miss
void handleMiss( int core, uint32_t addr, bool is_write, uint32_t set_index, uint32_t tag) {
    Cache& cache = caches[core];
    auto& set = cache.sets[set_index];
    int victim_index = findLRU(set);

    // Evict if necessary
    if (set[victim_index].state != INVALID) {
        cache.eviction_count++;
        if (set[victim_index].dirty) {
            cache.writeback_count++;
            global_stats.bus_data_traffic += cache.block_size;
            cache.idle_cycles += 100; // Writeback to memory
        }
    }

    // Fetch block from memory
    cache.idle_cycles += 100;
    cache.miss_count++;
    set[victim_index].tag = tag;
    set[victim_index].dirty = false;
    set[victim_index].data.resize(cache.block_size / 4, 0); // Initialize data
    set[victim_index].state = is_write ? MODIFIED : EXCLUSIVE;

    updateLRU(set, victim_index);
}