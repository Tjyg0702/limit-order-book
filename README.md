# limit-order-book
High-performance limit order book and matching engine built with C++20.
# Limit Order Book & Matching Engine

A C++20 limit order book and matching engine focused on deterministic matching semantics, efficient order lookup, replayable market-event workloads, correctness validation, and profile-driven performance optimization.

The engine implements price-time priority, partial and multi-level fills, order cancellation/modification, deterministic synthetic event replay, throughput benchmarking, and per-event latency measurement.

## Highlights

- C++20 matching engine with price-time priority
- Bid/ask price levels with FIFO ordering
- Add, cancel, and modify order operations
- Partial fills and multi-price-level sweeps
- O(1)-average order-ID lookup through a hash index
- Deterministic synthetic market-event replay
- Runtime invariant validation
- GoogleTest correctness suite with randomized stress testing
- Google Benchmark throughput benchmarks
- Per-event latency measurements with percentile reporting
- Linux `perf` profiling and profile-driven optimization
- Python benchmark-summary tooling

## Matching Semantics

The order book follows price-time priority.

### Price Priority

For resting orders:

- Highest bid has priority on the buy side.
- Lowest ask has priority on the sell side.

### Time Priority

Orders at the same price level are matched FIFO using insertion sequence numbers.

### Execution Price

Trades execute at the resting maker order's price.

### Order Modification

Modification behavior follows these rules:

- Decreasing quantity at the same price preserves time priority.
- Increasing quantity loses time priority.
- Changing price loses time priority.
- A quantity of zero cancels the order.
- A price change may immediately cross the spread and generate trades.

## Architecture

```text
SyntheticEventGenerator
        |
        v
   MarketEvent
        |
        v
   ReplayEngine
        |
        v
    OrderBook
   /         \
BidBook     AskBook
   \         /
   Price Levels
        |
        v
   FIFO Orders
        |
        +--------> Trade Events

OrderId
   |
   v
order_index_
   |
   +--------> OrderLocator
              - side
              - price
              - list iterator
```

The main components are:

### `OrderBook`

Owns the bid and ask books, performs matching, maintains market depth, and manages resting orders.

### `ReplayEngine`

Processes a sequence of `Add`, `Cancel`, and `Modify` market events and records aggregate replay statistics.

### `SyntheticEventGenerator`

Generates deterministic market-event streams from a fixed random seed for repeatable testing and benchmarking.

## Data Structures

### Price Levels

Bids use:

```cpp
std::map<Price, PriceLevel, std::greater<Price>>
```

Asks use:

```cpp
std::map<Price, PriceLevel>
```

This keeps the best bid and best ask at `begin()` for their respective books.

### FIFO Orders

Each price level stores orders in:

```cpp
std::list<Order>
```

This provides:

- stable iterators
- FIFO ordering
- O(1) erase when the order iterator is known

The tradeoff is node allocation and lower cache locality relative to contiguous containers.

### Order Index

Resting orders are indexed using:

```cpp
std::unordered_map<OrderId, OrderLocator>
```

An `OrderLocator` stores the order side, price, and direct list iterator.

This enables average O(1) lookup by order ID and avoids scanning price levels during cancellation.

## Complexity

Let:

- `P` = number of active price levels
- `N` = number of resting orders
- `F` = number of maker orders filled by an incoming order

Typical operation costs:

| Operation | Complexity |
|---|---:|
| Best bid / ask | O(1) |
| Order-ID lookup | O(1) average |
| Price-level lookup | O(log P) |
| Rest non-crossing order | O(log P) average plus hash insertion |
| Cancel known order | O(log P) plus O(1) list erase |
| Match incoming order | O(F + crossed price-level operations) |
| Depth at a known price | O(log P) |
| Full invariant validation | O(N) |

## Correctness Testing

The project uses GoogleTest.

The test suite currently contains **30 tests** covering:

- empty book behavior
- resting order insertion
- best bid / ask
- spread calculation
- aggregated market depth
- invalid and duplicate orders
- cancellation
- FIFO matching
- partial fills
- full fills
- maker-price execution
- multi-level book sweeps
- residual taker quantity
- modify-order priority rules
- replay engine behavior
- deterministic event generation

A randomized invariant test also executes **50,000 deterministic events** and validates the entire book after each event.

The invariant checker verifies:

- no empty price levels remain
- map keys match stored price levels
- order side and price are consistent
- resting quantities are non-zero
- FIFO sequence numbers are strictly increasing
- resting order IDs are unique
- every resting order has a consistent index entry
- aggregated level quantities are correct
- index size matches resting order count
- the resting book is never crossed

AddressSanitizer and UndefinedBehaviorSanitizer builds are also supported.

## Building

Requirements:

- C++20 compiler
- CMake 3.20+
- Linux or WSL2

Configure and build:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build -j
```

Run the executable:

```bash
./build/limit_order_book
```

## Running Tests

```bash
ctest --test-dir build \
    --output-on-failure
```

### Sanitizer Build

```bash
cmake -S . -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_SANITIZERS=ON

cmake --build build-asan -j

ctest --test-dir build-asan \
    --output-on-failure
```

## Performance Benchmarks

Enable benchmark targets:

```bash
cmake -S . -B build-bench \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_BENCHMARKS=ON \
    -DENABLE_SANITIZERS=OFF

cmake --build build-bench \
    --target lob_benchmark lob_latency \
    -j
```

### Throughput

```bash
./build-bench/lob_benchmark \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true
```

### Instrumented Per-Event Latency

```bash
./build-bench/lob_latency
```

The latency harness measures every event with `std::chrono::steady_clock`, records the latency distribution, and reports:

- mean
- p50
- p95
- p99
- p99.9
- maximum
- median clock-measurement overhead

The measurement overhead is reported separately and is not mechanically subtracted from the latency percentiles.

## Benchmark Environment

Measurements below were collected on:

- WSL2 Ubuntu 24.04
- GCC 13.3
- C++20
- Release build
- 16 logical CPUs
- deterministic synthetic workload
- fixed seed `0xC0FFEE`

The 1M-event workload generated:

```text
Events: 1,000,000
Trades: 496,864
Peak resting orders: 67,919
```

All reported results should be interpreted as measurements from this environment rather than hardware-independent latency guarantees.

## Performance Results

### Replay Throughput

Before profile-driven optimization:

```text
1M-event median throughput:
8.8364 million events / second
```

Final measured result:

```text
1M-event median throughput:
10.7469 million events / second
```

Overall improvement:

```text
+21.6%
```

For the 100K workload:

```text
Initial median:
9.9129 million events / second

Final median:
14.4927 million events / second
```

### Final Latency Distribution

Five independent 1M-event runs were collected and summarized using the median across runs.

```text
Mean:      135.323 ns
p50:        96 ns
p95:       343 ns
p99:       570 ns
p99.9:   1,420 ns
```

Median clock measurement overhead:

```text
13 ns
```

The tail measurements are sensitive to WSL2 and operating-system scheduling effects, so the benchmark reports repeated-run medians rather than selecting the best individual run.

## Profile-Driven Optimization

Performance optimization was guided by Linux `perf` rather than by changing containers based only on intuition.

Initial profiling showed the order-ID hash index as the dominant CPU hotspot, with `std::unordered_map` lookup and mutation accounting for a large fraction of sampled cycles.

### Optimization 1 — Remove Redundant Hash Lookups

The original modify path could perform repeated order-ID lookups:

```text
modify_order
    |
    +-- find(id)
    |
    +-- cancel_order
    |      |
    |      +-- find(id)
    |
    +-- add_order
           |
           +-- contains(id)
```

The implementation was refactored so internal operations reuse the existing hash-table iterator and use an unchecked internal add path after cancellation.

This reduced the modify/replace path from as many as three order-index lookups to one.

Measured 1M-event throughput:

```text
Before: 8.8364 M events/s
After:  9.4624 M events/s

Improvement: +7.1%
```

### Optimization 2 — Trade Vector Capacity

The trade vector uses a small initial reservation:

```cpp
trades.reserve(4);
```

This reduces repeated small allocations when a crossing order generates multiple fills.

The measured improvement was small relative to benchmark variance, so it is treated as a minor implementation optimization rather than a primary performance claim.

### Optimization 3 — Size the Order Index from Measurement

Instead of reserving capacity using the total event count, the benchmark first measured the actual maximum number of simultaneously resting orders.

Observed peak:

```text
67,919 resting orders
```

The benchmark then provides an expected capacity of:

```text
80,000 orders
```

to the order-ID index.

This reduces hash-table growth and rehashing while avoiding an unnecessarily large one-million-entry reservation.

Measured 1M-event throughput:

```text
Before index reservation:
9.5598 M events/s

After index reservation:
10.7469 M events/s

Improvement: +12.4%
```

## Latency Analysis Script

Latency output from multiple runs can be summarized with:

```bash
python3 scripts/summarize_latency.py \
    /path/to/latency-output.txt
```

The script extracts each run and reports median-of-runs values for:

- clock overhead
- mean latency
- p50
- p95
- p99
- p99.9
- peak resting order count

## Profiling

A profiling build with debug symbols can be generated using:

```bash
cmake -S . -B build-profile \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DENABLE_BENCHMARKS=ON

cmake --build build-profile \
    --target lob_benchmark \
    -j
```

Example sampling workflow:

```bash
perf record \
    -F 999 \
    -e cycles:u \
    -- \
    ./build-profile/lob_benchmark \
    --benchmark_filter='BM_ReplayEvents/1000000'
```

Then inspect hotspots:

```bash
perf report \
    --stdio \
    --sort=dso,symbol
```

## Project Structure

```text
limit-order-book/
├── benchmarks/
│   ├── latency_benchmark.cpp
│   └── replay_benchmark.cpp
├── include/
│   └── lob/
│       ├── event.hpp
│       ├── event_generator.hpp
│       ├── order.hpp
│       ├── order_book.hpp
│       ├── price_level.hpp
│       ├── replay_engine.hpp
│       ├── trade.hpp
│       └── types.hpp
├── scripts/
│   └── summarize_latency.py
├── src/
│   ├── event_generator.cpp
│   ├── main.cpp
│   ├── order_book.cpp
│   └── replay_engine.cpp
├── tests/
│   ├── event_generator_test.cpp
│   ├── invariant_test.cpp
│   ├── matching_test.cpp
│   ├── modify_order_test.cpp
│   ├── order_book_test.cpp
│   └── replay_engine_test.cpp
├── CMakeLists.txt
└── README.md
```

## Design Tradeoffs

The current implementation intentionally uses standard-library containers so the baseline design is understandable and measurable.

Potential future experiments include:

- pooled allocation for resting orders
- alternative hash-table implementations
- cache-friendlier price-level storage
- fixed or bounded price ladders
- binary market-event replay
- reduced allocation on generated trade paths

These are intentionally left as future work rather than claimed as implemented optimizations.

## Summary

This project demonstrates more than matching-engine functionality alone. It combines:

- modern C++20
- trading-system data structures
- deterministic event processing
- correctness and invariant testing
- Linux performance tooling
- latency and throughput benchmarking
- profiler-driven optimization
- Python-based benchmark analysis

The resulting implementation processes approximately **10.75 million deterministic market events per second** in the measured 1M-event workload, representing a **21.6% throughput improvement** over the initial profiled implementation.