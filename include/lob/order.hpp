#pragma once

#include "lob/types.hpp"

namespace lob {

struct Order {
    OrderId id{};
    Side side{};
    Price price{};
    Quantity remaining_quantity{};
    SequenceNumber sequence{};
};

} // namespace lob