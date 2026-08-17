#include <benchmark/benchmark.h>

#include <chrono>
#include <cstddef>
#include <memory>

#include "lob/event_generator.hpp"
#include "lob/replay_engine.hpp"

static void BM_ReplayEvents(
    benchmark::State& state
) {
    const auto event_count =
        static_cast<std::size_t>(
            state.range(0)
        );

    lob::EventGeneratorConfig config;
    config.event_count = event_count;
    config.seed = 0xC0FFEE;

    // Generate the workload once.
    // Event generation is intentionally excluded
    // from replay timing.
    const auto events =
        lob::SyntheticEventGenerator::generate(
            config
        );

    for (auto _ : state) {
        // Fresh book for every iteration so each replay
        // starts from identical state.
        constexpr std::size_t expected_orders = 80'000;

        auto engine =
            std::make_unique<lob::ReplayEngine>(
                expected_orders
            );

        const auto start =
            std::chrono::steady_clock::now();

        auto stats =
            engine->replay(events);

        const auto end =
            std::chrono::steady_clock::now();

        benchmark::DoNotOptimize(
            stats.events_processed
        );

        benchmark::DoNotOptimize(
            stats.trades_generated
        );

        const std::chrono::duration<double>
            elapsed = end - start;

        state.SetIterationTime(
            elapsed.count()
        );
    }

    state.SetItemsProcessed(
        static_cast<std::int64_t>(
            state.iterations()
        ) *
        static_cast<std::int64_t>(
            events.size()
        )
    );
}

BENCHMARK(BM_ReplayEvents)
    ->Arg(100'000)
    ->Arg(1'000'000)
    ->MinTime(1.0)
    ->UseManualTime();

BENCHMARK_MAIN();