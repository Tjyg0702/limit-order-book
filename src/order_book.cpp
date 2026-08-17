#include "lob/order_book.hpp"

#include <unordered_set>
#include <algorithm>
#include <iterator>

namespace lob {

std::vector<Trade> OrderBook::add_order(
    OrderId id,
    Side side,
    Price price,
    Quantity quantity
) {
    if (id == 0 || price <= 0 || quantity == 0) {
        return {};
    }

    if (order_index_.contains(id)) {
        return {};
    }

    std::vector<Trade> trades;
    Quantity remaining = quantity;

    if (side == Side::Buy) {
        while (
            remaining > 0 &&
            !asks_.empty()
        ) {
            auto level_it = asks_.begin();

            if (price < level_it->first) {
                break;
            }

            auto& level = level_it->second;

            while (
                remaining > 0 &&
                !level.orders.empty()
            ) {
                auto& maker = level.orders.front();

                const Quantity traded_quantity =
                    std::min(
                        remaining,
                        maker.remaining_quantity
                    );

                trades.push_back({
                    .maker_order_id = maker.id,
                    .taker_order_id = id,
                    .price = maker.price,
                    .quantity = traded_quantity
                });

                remaining -= traded_quantity;
                maker.remaining_quantity -= traded_quantity;
                level.total_quantity -= traded_quantity;

                if (maker.remaining_quantity == 0) {
                    order_index_.erase(maker.id);
                    level.orders.pop_front();
                }
            }

            if (level.orders.empty()) {
                asks_.erase(level_it);
            }
        }
    } else {
        while (
            remaining > 0 &&
            !bids_.empty()
        ) {
            auto level_it = bids_.begin();

            if (price > level_it->first) {
                break;
            }

            auto& level = level_it->second;

            while (
                remaining > 0 &&
                !level.orders.empty()
            ) {
                auto& maker = level.orders.front();

                const Quantity traded_quantity =
                    std::min(
                        remaining,
                        maker.remaining_quantity
                    );

                trades.push_back({
                    .maker_order_id = maker.id,
                    .taker_order_id = id,
                    .price = maker.price,
                    .quantity = traded_quantity
                });

                remaining -= traded_quantity;
                maker.remaining_quantity -= traded_quantity;
                level.total_quantity -= traded_quantity;

                if (maker.remaining_quantity == 0) {
                    order_index_.erase(maker.id);
                    level.orders.pop_front();
                }
            }

            if (level.orders.empty()) {
                bids_.erase(level_it);
            }
        }
    }

    if (remaining > 0) {
        rest_order(
            id,
            side,
            price,
            remaining
        );
    }

    return trades;
}

bool OrderBook::validate_invariants() const {
    std::size_t counted_orders = 0;
    std::unordered_set<OrderId> seen_ids;

    auto validate_side =
        [&](const auto& book, Side expected_side) {
            for (const auto& [price, level] : book) {
                // Empty price levels must never remain in the book.
                if (level.orders.empty()) {
                    return false;
                }

                // Map key and PriceLevel price must agree.
                if (level.price != price) {
                    return false;
                }

                Quantity calculated_quantity = 0;

                SequenceNumber previous_sequence = 0;
                bool first_order = true;

                for (
                    auto order_it = level.orders.begin();
                    order_it != level.orders.end();
                    ++order_it
                ) {
                    const auto& order = *order_it;

                    if (order.side != expected_side) {
                        return false;
                    }

                    if (order.price != price) {
                        return false;
                    }

                    if (order.remaining_quantity == 0) {
                        return false;
                    }

                    // FIFO order should correspond to increasing
                    // sequence numbers.
                    if (
                        !first_order &&
                        order.sequence <= previous_sequence
                    ) {
                        return false;
                    }

                    previous_sequence = order.sequence;
                    first_order = false;

                    // No duplicate resting OrderIds.
                    if (!seen_ids.insert(order.id).second) {
                        return false;
                    }

                    const auto index_it =
                        order_index_.find(order.id);

                    if (index_it == order_index_.end()) {
                        return false;
                    }

                    const auto& locator =
                        index_it->second;

                    if (locator.side != expected_side) {
                        return false;
                    }

                    if (locator.price != price) {
                        return false;
                    }

                    if (locator.iterator != order_it) {
                        return false;
                    }

                    calculated_quantity +=
                        order.remaining_quantity;

                    ++counted_orders;
                }

                if (
                    calculated_quantity !=
                    level.total_quantity
                ) {
                    return false;
                }
            }

            return true;
        };

    if (!validate_side(bids_, Side::Buy)) {
        return false;
    }

    if (!validate_side(asks_, Side::Sell)) {
        return false;
    }

    // Every resting order must have exactly one index entry.
    if (counted_orders != order_index_.size()) {
        return false;
    }

    // After matching finishes, the resting book must never
    // remain crossed.
    if (!bids_.empty() && !asks_.empty()) {
        if (
            bids_.begin()->first >=
            asks_.begin()->first
        ) {
            return false;
        }
    }

    return true;
}

bool OrderBook::cancel_order(OrderId id) {
    const auto index_it = order_index_.find(id);

    if (index_it == order_index_.end()) {
        return false;
    }

    const OrderLocator locator = index_it->second;

    if (locator.side == Side::Buy) {
        auto level_it = bids_.find(locator.price);

        if (level_it == bids_.end()) {
            return false;
        }

        auto& level = level_it->second;

        level.total_quantity -=
            locator.iterator->remaining_quantity;

        level.orders.erase(locator.iterator);

        if (level.orders.empty()) {
            bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(locator.price);

        if (level_it == asks_.end()) {
            return false;
        }

        auto& level = level_it->second;

        level.total_quantity -=
            locator.iterator->remaining_quantity;

        level.orders.erase(locator.iterator);

        if (level.orders.empty()) {
            asks_.erase(level_it);
        }
    }

    order_index_.erase(index_it);

    return true;
}

std::vector<Trade> OrderBook::modify_order(
    OrderId id,
    Price new_price,
    Quantity new_quantity
) {
    const auto index_it = order_index_.find(id);

    if (index_it == order_index_.end()) {
        return {};
    }

    if (new_price <= 0) {
        return {};
    }

    if (new_quantity == 0) {
        cancel_order(id);
        return {};
    }

    const OrderLocator locator = index_it->second;
    const Quantity current_quantity =
        locator.iterator->remaining_quantity;

    if (
        new_price == locator.price &&
        new_quantity <= current_quantity
    ) {
        const Quantity quantity_reduction =
            current_quantity - new_quantity;

        if (locator.side == Side::Buy) {
            auto level_it = bids_.find(locator.price);

            if (level_it == bids_.end()) {
                return {};
            }

            level_it->second.total_quantity -=
                quantity_reduction;
        } else {
            auto level_it = asks_.find(locator.price);

            if (level_it == asks_.end()) {
                return {};
            }

            level_it->second.total_quantity -=
                quantity_reduction;
        }

        locator.iterator->remaining_quantity =
            new_quantity;

        return {};
    }

    const Side side = locator.side;

    if (!cancel_order(id)) {
        return {};
    }

    return add_order(
        id,
        side,
        new_price,
        new_quantity
    );
}

void OrderBook::rest_order(
    OrderId id,
    Side side,
    Price price,
    Quantity quantity
) {
    const SequenceNumber sequence = ++next_sequence_;

    Order order{
        .id = id,
        .side = side,
        .price = price,
        .remaining_quantity = quantity,
        .sequence = sequence
    };

    if (side == Side::Buy) {
        auto [level_it, inserted] =
            bids_.try_emplace(
                price,
                PriceLevel{
                    .price = price
                }
            );

        auto& level = level_it->second;

        level.orders.push_back(order);
        level.total_quantity += quantity;

        auto order_it = std::prev(level.orders.end());

        order_index_.emplace(
            id,
            OrderLocator{
                .side = side,
                .price = price,
                .iterator = order_it
            }
        );

        return;
    }

    auto [level_it, inserted] =
        asks_.try_emplace(
            price,
            PriceLevel{
                .price = price
            }
        );

    auto& level = level_it->second;

    level.orders.push_back(order);
    level.total_quantity += quantity;

    auto order_it = std::prev(level.orders.end());

    order_index_.emplace(
        id,
        OrderLocator{
            .side = side,
            .price = price,
            .iterator = order_it
        }
    );
}

} // namespace lob