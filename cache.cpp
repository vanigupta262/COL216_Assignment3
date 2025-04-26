#include "cache.hpp"
#include "bus.hpp"

// Parse memory address
void parseAddress(uint32_t addr, uint32_t set_index_bits, uint32_t block_offset_bits,
                 uint32_t& tag, uint32_t& set_index, uint32_t& block_offset) {
    block_offset = addr & ((1 << block_offset_bits) - 1);
    addr >>= block_offset_bits;
    set_index = addr & ((1 << set_index_bits) - 1);
    addr >>= set_index_bits;
    tag = addr;
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

    if (hit) {
        if (is_write) {
            set[hit_index].dirty = true;
            set[hit_index].state = MODIFIED;
            bool shared = false;
            snoopBus(core, addr, true, shared);
        } else {
            bool shared = false;
            snoopBus(core, addr, false, shared);
            if (shared) {
                set[hit_index].state = SHARED;
            }
        }
        updateLRU(set, hit_index);
    } else {
        handleMiss(core, addr, is_write, set_index, tag);
    }
}