#pragma once

#include "lob/types.hpp"

namespace lob {

enum class EventType : std::uint8_t {
    Add,
    Cancel,
    Modify
};

struct MarketEvent {
    EventType type{};

    OrderId order_id{};
    Side side{};

    Price price{};
    Quantity quantity{};
};

} // namespace lob