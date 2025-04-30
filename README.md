# COL216_Assignment3

# MESI Cache Coherence Simulator

This project contains a C++ simulator for the MESI (Modified, Exclusive, Shared, Invalid) cache coherence protocol, modeling a multicore system (quad-core) with private L1 caches and a shared bus. The simulator processes memory access traces for four cores, accurately tracks bus contention and cache state transitions, and outputs detailed performance statistics.

---

## Overview

This project simulates a snooping-based MESI cache coherence protocol for a multicore processor system. Each core has a private L1 cache, and all caches communicate over a shared, blocking bus. The simulator processes memory access traces, modeling cache hits, misses, evictions, writebacks, and all MESI protocol transitions. The bus is fully serialized: only one bus transaction can occur at a time, and cores stall as needed.

**Features:**
- Accurate MESI protocol implementation (including invalidations, cache-to-cache transfers, memory fetches, and writebacks)
- Blocking cache model: cores stall on misses or write hits to SHARED state until the bus is available
- Per-core and global statistics: idle cycles, hit/miss counts, writebacks, bus traffic, invalidations, and more
- Trace-driven simulation using user-provided memory access traces

---

## Building

To build the simulator, simply run:
- `make` - it makes the executable, named `L1simulate'
- `make clean`

This creates an executable, that can be run with commands similar to the following:
`./L1simulate -t <trace_prefix> -s 6 -E 2 -b 5 -o <output_file>`
The flags hold the following meanings:
- -t : Name prefix of the parallel application 
- -s : Number of set index bits
- -E : Associativity. Number of cache lines per set.
- -b : Number of block bits
- -o : Output file for simulation statistics
- -h : help

