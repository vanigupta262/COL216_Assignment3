#include "cache.hpp"
#include "bus.hpp"
#include <iostream>
#include <iomanip>

// Parse memory address
void parseAddress(uint32_t addr, uint32_t set_index_bits, uint32_t block_offset_bits,
                 uint32_t& tag, uint32_t& set_index, uint32_t& block_offset) {
    block_offset = addr & ((1 << block_offset_bits) - 1);
    addr >>= block_offset_bits;
    set_index = addr & ((1 << set_index_bits) - 1);
    addr >>= set_index_bits;
    tag = addr;
    std::cout << "Parsed addr=0x" << std::hex << (addr << (set_index_bits + block_offset_bits) | (set_index << block_offset_bits) | block_offset)
              << ": tag=0x" << tag << ", set_index=" << std::dec << set_index << ", block_offset=0x" << std::hex << block_offset << "\n";
}

// Find LRU line in a set
int findLRU(const std::vector<CacheLine>& set) {
    uint32_t max_lru = 0;
    int lru_index = 0;
    for (size_t i = 0; i < set.size(); ++i) {
        if (set[i].lru_counter > max_lru) {
            max_lru = set[i].lru_counter;
            lru_index = i;
        }
    }
    return lru_index;
}

// Update LRU counters
void updateLRU(std::vector<CacheLine>& set, int used_index) {
    for (size_t i = 0; i < set.size(); ++i) {
        set[i].lru_counter++;
    }
    set[used_index].lru_counter = 0;
}

// Process memory reference
void processReference(int core, char op, uint32_t addr) {
    Cache& cache = caches[core];
    bool is_write = (op == 'W');
    if (is_write) cache.write_count++;
    else cache.read_count++;

    uint32_t tag, set_index, block_offset;
    parseAddress(addr, cache.set_index_bits, cache.block_offset_bits, tag, set_index, block_offset);

    if (set_index >= cache.sets.size()) {
        std::cerr << "Error: Invalid set_index " << set_index << " for addr 0x" << std::hex << addr << "\n";
        std::exit(1);
    }

    auto& set = cache.sets[set_index];
    bool hit = false;
    int hit_index = -1;

    // Check for hit
    for (size_t i = 0; i < set.size(); ++i) {
        if (set[i].state != INVALID && set[i].tag == tag) {
            hit = true;
            hit_index = i;
            break;
        }
    }

    std::cout << "Core " << core << ": op=" << op << ", addr=0x" << std::hex << addr
              << ", hit=" << (hit ? "true" : "false") << ", state="
              << (hit ? (set[hit_index].state == MODIFIED ? "M" : set[hit_index].state == EXCLUSIVE ? "E" : set[hit_index].state == SHARED ? "S" : "I") : "N/A") << "\n";

    if (hit) {
        if (is_write) {
            if (set[hit_index].state == MODIFIED) {
                // Write Hit (M): Update value, no state change
                set[hit_index].dirty = true;
            } else if (set[hit_index].state == EXCLUSIVE) {
                // Write Hit (E): Update value, E -> M
                set[hit_index].dirty = true;
                set[hit_index].state = MODIFIED;
            } else if (set[hit_index].state == SHARED) {
                // Write Hit (S): Invalidate others, update value, S -> M
                bool shared = false;
                bool supplied = false;
                std::vector<uint32_t> data;
                snoopBus(core, addr, true, shared, supplied, data);
                set[hit_index].dirty = true;
                set[hit_index].state = MODIFIED;
            }
        } else {
            // Read Hit (M, E, S): No state change
            // Check if others have it to ensure SHARED state
            bool shared = false;
            bool supplied = false;
            std::vector<uint32_t> data;
            snoopBus(core, addr, false, shared, supplied, data);
            if (shared && set[hit_index].state == EXCLUSIVE) {
                set[hit_index].state = SHARED;
            }
        }
        updateLRU(set, hit_index);
    } else {
        handleMiss(core, addr, is_write, set_index, tag);
    }
}