#pragma once

#include <list>

#include "lob/order.hpp"

namespace lob {

struct PriceLevel {
    Price price{};
    Quantity total_quantity{};
    std::list<Order> orders;
};

} // namespace lob