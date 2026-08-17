#include <iostream>

#include "lob/event_generator.hpp"
#include "lob/replay_engine.hpp"

int main() {
    lob::EventGeneratorConfig config;

    config.event_count = 100'000;
    config.seed = 0xC0FFEE;

    const auto events =
        lob::SyntheticEventGenerator::generate(
            config
        );

    lob::ReplayEngine engine;

    const auto stats =
        engine.replay(events);

    std::cout
        << "Events processed: "
        << stats.events_processed
        << '\n';

    std::cout
        << "Trades generated: "
        << stats.trades_generated
        << '\n';

    std::cout
        << "Resting orders: "
        << engine.order_book().order_count()
        << '\n';

    std::cout
        << "Book valid: "
        << std::boolalpha
        << engine.order_book().validate_invariants()
        << '\n';

    return 0;
}