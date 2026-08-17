#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

#include "lob/event_generator.hpp"
#include "lob/replay_engine.hpp"

namespace {

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

std::uint64_t percentile(
    const std::vector<std::uint64_t>& values,
    double p
) {
    if (values.empty()) {
        return 0;
    }

    const auto index =
        static_cast<std::size_t>(
            p * static_cast<double>(values.size() - 1)
        );

    return values[index];
}

std::uint64_t measure_clock_overhead(
    std::size_t iterations
) {
    std::vector<std::uint64_t> samples;
    samples.reserve(iterations);

    for (std::size_t i = 0; i < iterations; ++i) {
        const auto start = Clock::now();
        const auto end = Clock::now();

        const auto ns =
            std::chrono::duration_cast<Nanoseconds>(
                end - start
            ).count();

        samples.push_back(
            static_cast<std::uint64_t>(ns)
        );
    }

    std::sort(
        samples.begin(),
        samples.end()
    );

    return percentile(samples, 0.50);
}

} // namespace

int main() {
    constexpr std::size_t event_count =
        1'000'000;

    lob::EventGeneratorConfig config;
    config.event_count = event_count;
    config.seed = 0xC0FFEE;

    const auto events =
        lob::SyntheticEventGenerator::generate(
            config
        );

    // Warm up code paths and caches using a separate engine.
    {
        lob::ReplayEngine warmup_engine(80'000);

        const std::size_t warmup_count =
            std::min<std::size_t>(
                100'000,
                events.size()
            );

        for (
            std::size_t i = 0;
            i < warmup_count;
            ++i
        ) {
            warmup_engine.process_event(
                events[i]
            );
        }
    }

    lob::ReplayEngine engine(80'000);

    std::vector<std::uint64_t> latencies_ns;
    latencies_ns.reserve(event_count);

    std::size_t total_trades = 0;
    std::size_t peak_order_count = 0;

    for (const auto& event : events) {
        const auto start = Clock::now();

        total_trades +=
            engine.process_event(event);

        const auto end = Clock::now();

        const auto latency =
            std::chrono::duration_cast<Nanoseconds>(
                end - start
            ).count();

        latencies_ns.push_back(
            static_cast<std::uint64_t>(latency)
        );

        peak_order_count =
            std::max(
                peak_order_count,
                engine.order_book().order_count()
            );
    }

    const auto clock_overhead =
        measure_clock_overhead(100'000);

    std::sort(
        latencies_ns.begin(),
        latencies_ns.end()
    );

    const auto total =
        std::accumulate(
            latencies_ns.begin(),
            latencies_ns.end(),
            std::uint64_t{0}
        );

    const double mean =
        static_cast<double>(total) /
        static_cast<double>(
            latencies_ns.size()
        );

    std::cout
        << "Events: "
        << event_count
        << '\n';

    std::cout
        << "Trades: "
        << total_trades
        << '\n';

    std::cout
        << "Clock overhead median: "
        << clock_overhead
        << " ns\n";

    std::cout
        << "Mean: "
        << mean
        << " ns\n";

    std::cout
        << "p50: "
        << percentile(latencies_ns, 0.50)
        << " ns\n";

    std::cout
        << "p95: "
        << percentile(latencies_ns, 0.95)
        << " ns\n";

    std::cout
        << "p99: "
        << percentile(latencies_ns, 0.99)
        << " ns\n";

    std::cout
        << "p99.9: "
        << percentile(latencies_ns, 0.999)
        << " ns\n";

    std::cout
        << "Max: "
        << latencies_ns.back()
        << " ns\n";

    std::cout
        << "Book valid: "
        << std::boolalpha
        << engine.order_book().validate_invariants()
        << '\n';

    std::cout
        << "Peak resting orders: "
        << peak_order_count
        << '\n';

    return 0;
}