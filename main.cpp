#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <getopt.h>
#include <iomanip>
#include "cache.hpp"
#include "bus.hpp"
using namespace std;
// Global variables
std::vector<Cache> caches(4); // Four cores
Stats global_stats;
uint32_t current_cycle = 0;
std::vector<std::vector<std::pair<char, uint32_t>>> traces(4);

// Main simulation loop
void simulate()
{
    std::vector<size_t> trace_indices(4, 0);
    bool all_done;

    while (true)
    {
        // Process cores
        all_done = true;
        for (int core = 0; core < 4; ++core)
        {
            if (trace_indices[core] < traces[core].size())
            {
                all_done = false;
                // Check if the core is ready to process a new reference
                if (caches[core].stall_cycles == 0)
                {
                    // Always try to process the reference - if it's a hit, it will complete
                    // If it's a miss and the bus is busy, it will be stalled
                    char op = traces[core][trace_indices[core]].first;
                    uint32_t addr = traces[core][trace_indices[core]].second;

                    // Parse the address to check if it's a hit
                    uint32_t tag, set_index, block_offset;
                    parseAddress(addr, caches[core].set_index_bits, caches[core].block_offset_bits, tag, set_index, block_offset);
                    auto &set = caches[core].sets[set_index];

                    // Check if it's a hit
                    bool hit = false;
                    int hit_index = -1;
                    for (size_t i = 0; i < set.size(); ++i)
                    {
                        if (set[i].state != INVALID && set[i].tag == tag)
                        {
                            hit = true;
                            hit_index = i;
                            break;
                        }
                    }

                    // If it's a hit, process it regardless of bus state
                    // If it's a write hit to SHARED, we need the bus, so check bus state
                    bool is_write = (op == 'W');
                    if (hit && !(is_write && set[hit_index].state == SHARED))
                    {
                        // Process the hit (not a write to SHARED state)
                        if (is_write)
                        {
                            caches[core].write_count++;
                            set[hit_index].state = MODIFIED;
                        }
                        else
                        {
                            caches[core].read_count++;
                        }
                        updateLRU(set, hit_index);
                        trace_indices[core]++;
                        // For a hit, execution takes just 1 cycle
                        caches[core].hit_cycles++;
                    }
                    // If it's a miss or a write hit to SHARED, we need the bus
                    else if (bus_busy_cycles == 0 && bus_queue.empty())
                    {
                        // For a miss or write hit to SHARED, execution will take additional cycles
                        // These cycles will be accounted for in processReference and handleMiss
                        processReference(core, op, addr);
                        trace_indices[core]++;
                    }
                    else
                    {
                        // Bus is busy but core has a pending request that needs the bus
                        // Count as idle cycle and stall the core until bus is available
                        caches[core].idle_cycles++;
                        // caches[core].stall_cycles = 1; // Stall for at least one cycle, will be reset when bus becomes available
                    }
                }
                else if (caches[core].stall_cycles > 0)
                { // the core is in undergoing a bus command.
                    caches[core].stall_cycles--;
                    // Count this as an idle cycle since the core is waiting for a request to complete
                    // caches[core].idle_cycles++;
                }
            }
        }
        if (!bus_queue.empty() || bus_busy_cycles > 0)
        {
            all_done = false;
        }

        if (all_done)
            break;

        // Process bus transactions first
        if (bus_busy_cycles == 0 && !bus_queue.empty())
        {
            BusRequest req = bus_queue.front();
            bus_queue.pop();
            bus_transactions++;
            // Special handling: if it is a writeback eviction request

            uint32_t tag, set_index, block_offset;
            parseAddress(req.addr, caches[req.core].set_index_bits, caches[req.core].block_offset_bits, tag, set_index, block_offset);
            auto &set = caches[req.core].sets[set_index];
            bool hit = false;
            for (size_t i = 0; i < set.size(); ++i)
            {
                if (set[i].state != INVALID && set[i].tag == tag)
                {
                    hit = true;
                    break;
                }
            }

            bool shared = hit ? true : false; // if a hit, it must be write hit at SHARED to be in the bus.
            bool supplied = false;
            snoopBus(req.core, req.addr, req.is_write, shared, supplied);

            caches[req.core].stall_cycles = 0;

            if (!hit)
                handleMiss(req.core, req.addr, req.is_write, set_index, tag);

            // bus_busy_cycles = supplied ? 2 * (caches[0].block_size/4) : 100;
            if (hit)
            { // If a write hit at SHARED then takes one cycle to invalidate (1 for hit)
                bus_busy_cycles = 1;
            }
            else if (supplied)
            { // if miss which gets data from cache, then 2N
                // Cache-to-cache transfer
                caches[req.core].stall_cycles += 2 * (caches[0].block_size / 4) - 1; //-1 because in this very cycle, it will start getting executed, so this cycle counts too
                bus_busy_cycles = 2 * (caches[0].block_size / 4);                    // For bus, we will subtract 1 before the end of this cycle (below), so we don't need to do -1 here
            }
            else
            { // miss with memory transfer, then 100
                // Memory access
                caches[req.core].stall_cycles += 100 - 1;
                bus_busy_cycles = 100;
            }

            current_initiator = req.core;
        }

        // Advance cycles
        current_cycle++;
        if (bus_busy_cycles > 0)
            bus_busy_cycles--;
    }

    global_stats.total_cycles = current_cycle;
}

int main(int argc, char *argv[])
{
    std::string trace_name, outfilename;
    int set_index_bits = 0, assoc = 0, block_bits = 0;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "t:s:E:b:o:h")) != -1)
    {
        switch (opt)
        {
        case 't':
            trace_name = optarg;
            break;
        case 's':
            set_index_bits = atoi(optarg);
            break;
        case 'E':
            assoc = atoi(optarg);
            break;
        case 'b':
            block_bits = atoi(optarg);
            break;
        case 'o':
            outfilename = optarg;
            break;
        case 'h':
            std::cout << "./L1simulate -t <tracefile> -s <set_index_bits> -E <associativity> -b <block_bits> -o <outfilename>\n";
            return 0;
        default:
            std::cerr << "Invalid option\n";
            return 1;
        }
    }

    // Initialize caches
    for (int i = 0; i < 4; ++i)
    {
        caches[i].num_sets = 1 << set_index_bits;
        caches[i].assoc = assoc;
        caches[i].block_size = 1 << block_bits;
        caches[i].set_index_bits = set_index_bits;
        caches[i].block_offset_bits = block_bits;
        caches[i].tag_bits = 32 - set_index_bits - block_bits;
        caches[i].sets.resize(caches[i].num_sets, std::vector<CacheLine>(assoc));
    }

    // Read trace files

    for (int i = 0; i < 4; ++i)
    {
        std::string filename = trace_name + "_proc" + std::to_string(i) + ".trace";
        std::ifstream file(filename);
        if (!file)
        {
            std::cerr << "Cannot open " << filename << "\n";
            return 1;
        }
        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            char op;
            uint32_t addr;
            if (!(iss >> op >> std::hex >> addr))
            {
                std::cerr << "Invalid trace entry in " << filename << ": " << line << "\n";
                return 1;
            }
            // std::cout << "Core " << i << ": op=" << op << ", addr=0x" << std::hex << addr << "\n";
            traces[i].emplace_back(op, addr);
        }
    }
    cout << "Trace files loaded successfully.\n";
    // Run simulation
    simulate();
    cout << "Simulation completed.\n";
    // Write output
    cout << "Simulation completed.\n";

    //---------------------------------- writing back M states
    for (int core=0 ; core<4 ; core++) {
        Cache &cache = caches[core];
        for(uint32_t i=0 ; i < caches[core].assoc ; i++) {
            for (uint32_t j=0 ; j<caches[core].num_sets ; j++) {
                if (caches[core].sets[i][j].state == MODIFIED) {
                    caches[core].memory_cycles += 100;
                    global_stats.total_cycles += 100;
                }
            }
        }
    }
    //----------------------------------


    // Write output

    std::ofstream outfile(outfilename);

    // Print simulation parameters
    outfile << "Simulation Parameters:\n";
    outfile << "Trace Prefix: " << trace_name << "\n";
    outfile << "Set Index Bits: " << set_index_bits << "\n";
    outfile << "Associativity: " << assoc << "\n";
    outfile << "Block Bits: " << block_bits << "\n";
    outfile << "Block Size (Bytes): " << (1 << block_bits) << "\n";
    outfile << "Number of Sets: " << (1 << set_index_bits) << "\n";
    outfile << "Cache Size (KB per core): " << std::fixed << std::setprecision(2) << ((1 << set_index_bits) * assoc * (1 << block_bits)) / 1024.0 << "\n";
    outfile << "MESI Protocol: Enabled\n";
    outfile << "Write Policy: Write-back, Write-allocate\n";
    outfile << "Replacement Policy: LRU\n";
    outfile << "Bus: Central snooping bus\n\n";

    // Print per-core statistics
    for (int i = 0; i < 4; ++i)
    {
        outfile << "Core " << i << " Statistics:\n";
        outfile << "Total Instructions: " << (caches[i].read_count + caches[i].write_count) << "\n";
        outfile << "Total Reads: " << caches[i].read_count << "\n";
        outfile << "Total Writes: " << caches[i].write_count << "\n";
        outfile << "Total Execution Cycles: " << (caches[i].hit_cycles + caches[i].memory_cycles) << "\n";
        outfile << "Idle Cycles: " << caches[i].idle_cycles << "\n";
        outfile << "Cache Misses: " << caches[i].miss_count << "\n";
        outfile << "Cache Miss Rate: " << std::fixed << std::setprecision(5) << (double)caches[i].miss_count / (caches[i].read_count + caches[i].write_count) * 100 << "%\n";
        outfile << "Cache Evictions: " << caches[i].eviction_count << "\n";
        outfile << "Writebacks: " << caches[i].writeback_count << "\n";
        outfile << "Bus Invalidations: " << caches[i].invalidation_count << "\n";
        outfile << "Data Traffic (Bytes): " << caches[i].data_traffic << "\n\n";
    }

    // Print overall bus summary
    outfile << "Overall Bus Summary:\n";
    outfile << "Total Bus Transactions: " << bus_transactions << "\n";
    outfile << "Total Bus Traffic (Bytes): " << global_stats.bus_data_traffic << "\n";
    outfile << "Maximum Execution Time (cycles): " << global_stats.total_cycles << "\n";

    return 0;
}
