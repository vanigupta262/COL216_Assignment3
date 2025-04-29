#include <iostream>
#include <fstream>
#include <vector>
#include <string>
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
void simulate() {
    std::vector<size_t> trace_indices(4, 0);
    bool all_done;
    
    while (true) {
        // Process cores
        all_done = true;
        for (int core = 0; core < 4; ++core) {
            if (trace_indices[core] < traces[core].size()) {
                all_done = false;
                if (caches[core].stall_cycles == 0) {
                    processReference(core, traces[core][trace_indices[core]].first,
                        traces[core][trace_indices[core]].second);
                    trace_indices[core]++;
                } else if (caches[core].stall_cycles > 0) { // the core is in undergoing a bus command.
                    caches[core].stall_cycles--;
                }
            }
        }
        if(!bus_queue.empty() || bus_busy_cycles>0){
            all_done = false;
        }
        
        if (all_done) break;
        
        // Process bus transactions first
        if (bus_busy_cycles == 0 && !bus_queue.empty()) {
            BusRequest req = bus_queue.front();
            bus_queue.pop();
            // Special handling: if it is a writeback eviction request
                    
            uint32_t tag, set_index, block_offset;
            parseAddress(req.addr, caches[req.core].set_index_bits, caches[req.core].block_offset_bits, tag, set_index, block_offset);
            auto& set = caches[req.core].sets[set_index];
            bool hit = false;
            int hit_index = -1;
            for (size_t i = 0; i < set.size(); ++i) {
                if (set[i].state != INVALID && set[i].tag == tag) {
                    hit = true;
                    hit_index = i;
                    break;
                }
            }

            bool shared = hit ? true : false; //if a hit, it must be write hit at SHARED to be in the bus.
            bool supplied = false;
            std::vector<uint32_t> data;
            snoopBus(req.core, req.addr, req.is_write, shared, supplied, data);
            
            caches[req.core].stall_cycles = 0;

            if (!hit) handleMiss(req.core, req.addr, req.is_write, set_index, tag);

            // bus_busy_cycles = supplied ? 2 * (caches[0].block_size/4) : 100;
            if (hit) { //If a write hit at SHARED then takes one cycle to invalidate (1 for hit)
                bus_busy_cycles = 1;
            } else if (supplied) { //if miss which gets data from cache, then 2N
                // Cache-to-cache transfer
                caches[req.core].stall_cycles += 2*(caches[0].block_size/4) - 1; //-1 because in this very cycle, it will start getting executed, so this cycle counts too
                bus_busy_cycles = 2 * (caches[0].block_size / 4); //For bus, we will subtract 1 before the end of this cycle (below), so we don't need to do -1 here
            } else { //miss with memory transfer, then 100
                // Memory access
                caches[req.core].stall_cycles += 100-1;
                bus_busy_cycles = 100;
            }
            
            current_initiator = req.core;
        }
        
        // Advance cycles
        current_cycle++;
        if (bus_busy_cycles > 0) bus_busy_cycles--;
        
    }
    
    global_stats.total_cycles = current_cycle;
}

int main(int argc, char* argv[]) {
    std::string trace_name, outfilename;
    int set_index_bits = 0, assoc = 0, block_bits = 0;

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "t:s:E:b:o:h")) != -1) {
        switch (opt) {
            case 't': trace_name = optarg; break;
            case 's': set_index_bits = atoi(optarg); break;
            case 'E': assoc = atoi(optarg); break;
            case 'b': block_bits = atoi(optarg); break;
            case 'o': outfilename = optarg; break;
            case 'h':
                std::cout << "./L1simulate -t <tracefile> -s <set_index_bits> -E <associativity> -b <block_bits> -o <outfilename>\n";
                return 0;
            default:
                std::cerr << "Invalid option\n";
                return 1;
        }
    }

    // Initialize caches
    for (int i = 0; i < 4; ++i) {
        caches[i].num_sets = 1 << set_index_bits;
        caches[i].assoc = assoc;
        caches[i].block_size = 1 << block_bits;
        caches[i].set_index_bits = set_index_bits;
        caches[i].block_offset_bits = block_bits;
        caches[i].tag_bits = 32 - set_index_bits - block_bits;
        caches[i].sets.resize(caches[i].num_sets, std::vector<CacheLine>(assoc));
    }

    // Read trace files
    
    for (int i = 0; i < 4; ++i) {
        std::string filename = trace_name + "_proc" + std::to_string(i) + ".trace";
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "Cannot open " << filename << "\n";
            return 1;
        }
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            char op;
            uint32_t addr;
            if (!(iss >> op >> std::hex >> addr)) {
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
    std::ofstream outfile(outfilename);
    for (int i = 0; i < 4; ++i) {
        outfile << "Core " << i << ":\n";
        outfile << "Reads: " << caches[i].read_count << "\n";
        outfile << "Writes: " << caches[i].write_count << "\n";
        outfile << "Total cycles: " << global_stats.total_cycles << "\n";
        outfile << "Idle cycles: " << caches[i].idle_cycles << "\n";
        outfile << "Miss rate: " << (double)caches[i].miss_count / (caches[i].read_count + caches[i].write_count) * 100 << "%\n";
        outfile << "Evictions: " << caches[i].eviction_count << "\n";
        outfile << "Writebacks: " << caches[i].writeback_count << "\n";
    }
    outfile << "Bus invalidations: " << global_stats.invalidations << "\n";
    outfile << "Bus data traffic (bytes): " << global_stats.bus_data_traffic << "\n";

    return 0;
}
