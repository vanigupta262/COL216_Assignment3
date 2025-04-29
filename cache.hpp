#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <cstdint>

enum MESIState { INVALID, SHARED, EXCLUSIVE, MODIFIED };

struct CacheLine {
    MESIState state;
    uint32_t tag;
    uint32_t lru_counter;
    CacheLine() : state(INVALID), tag(0), lru_counter(0) {}
};

struct Cache {
    std::vector<std::vector<CacheLine>> sets;
    uint32_t num_sets;
    uint32_t assoc;
    uint32_t block_size;
    uint32_t set_index_bits;
    uint32_t block_offset_bits;
    uint32_t tag_bits;
    uint64_t read_count = 0;
    uint64_t write_count = 0;
    uint64_t miss_count = 0;
    uint64_t eviction_count = 0;
    uint64_t writeback_count = 0;
    uint64_t idle_cycles = 0;
    int stall_cycles = 0; // New field
};

struct Stats {
    uint64_t total_cycles = 0;
    uint64_t invalidations = 0;
    uint64_t bus_data_traffic = 0;
};

// Parse memory address
void parseAddress(uint32_t addr, uint32_t set_index_bits, uint32_t block_offset_bits,
                 uint32_t& tag, uint32_t& set_index, uint32_t& block_offset);

// Find LRU line in a set
int findLRU(const std::vector<CacheLine>& set);

// Update LRU counters
void updateLRU(std::vector<CacheLine>& set, int used_index);

// Process memory reference
void processReference(int core, char op, uint32_t addr);

#endif