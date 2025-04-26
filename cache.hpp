#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <cstdint>

// Cache line states for MESI protocol
enum MESIState { INVALID, SHARED, EXCLUSIVE, MODIFIED };

// Cache line structure
struct CacheLine {
    MESIState state;
    uint32_t tag;
    bool dirty;
    uint32_t lru_counter; // For LRU replacement
    std::vector<uint32_t> data; // Data words in the block
    CacheLine() : state(INVALID), tag(0), dirty(false), lru_counter(0), data() {}
};

// Cache structure for each core
struct Cache {
    std::vector<std::vector<CacheLine>> sets;
    uint32_t num_sets;
    uint32_t assoc;
    uint32_t block_size;
    uint32_t set_index_bits;
    uint32_t block_offset_bits;
    uint32_t tag_bits;
    uint64_t read_count;
    uint64_t write_count;
    uint64_t miss_count;
    uint64_t eviction_count;
    uint64_t writeback_count;
    uint64_t idle_cycles;
};

// Simulation statistics
struct Stats {
    uint64_t total_cycles;
    uint64_t invalidations;
    uint64_t bus_data_traffic; // In bytes
};

// Simulated main memory (simplified)
extern std::vector<uint32_t> memory;

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