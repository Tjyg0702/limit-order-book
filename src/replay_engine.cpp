#include "lob/replay_engine.hpp"

namespace lob {

ReplayEngine::ReplayEngine(
    std::size_t expected_orders
)
    : order_book_(expected_orders) {
}

std::size_t ReplayEngine::process_event(
    const MarketEvent& event
) {
    switch (event.type) {
        case EventType::Add: {
            const auto trades =
                order_book_.add_order(
                    event.order_id,
                    event.side,
                    event.price,
                    event.quantity
                );

            return trades.size();
        }

        case EventType::Cancel: {
            order_book_.cancel_order(
                event.order_id
            );

            return 0;
        }

        case EventType::Modify: {
            const auto trades =
                order_book_.modify_order(
                    event.order_id,
                    event.price,
                    event.quantity
                );

            return trades.size();
        }
    }

    return 0;
}

ReplayStats ReplayEngine::replay(
    const std::vector<MarketEvent>& events
) {
    ReplayStats stats;

    for (const auto& event : events) {
        ++stats.events_processed;

        stats.trades_generated +=
            process_event(event);
    }

    return stats;
}

} // namespace lob