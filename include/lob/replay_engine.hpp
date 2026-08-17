#pragma once

#include <cstddef>
#include <vector>

#include "lob/event.hpp"
#include "lob/order_book.hpp"

namespace lob {

struct ReplayStats {
    std::size_t events_processed{};
    std::size_t trades_generated{};
};

class ReplayEngine {
public:
    explicit ReplayEngine(
        std::size_t expected_orders = 0
    );

    std::size_t process_event(
        const MarketEvent& event
    );

    ReplayStats replay(
        const std::vector<MarketEvent>& events
    );

    [[nodiscard]]
    const OrderBook& order_book() const noexcept {
        return order_book_;
    }

private:
    OrderBook order_book_;
};

} // namespace lob