#pragma once

#include "lob/types.hpp"

namespace lob {

struct Trade {
    OrderId maker_order_id{};
    OrderId taker_order_id{};
    Price price{};
    Quantity quantity{};
};

} // namespace lob