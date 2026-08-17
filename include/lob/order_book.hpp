#pragma once

#include <cstddef>
#include "lob/trade.hpp"
#include <vector>
#include <optional>
#include <functional>
#include <list>
#include <map>
#include <unordered_map>

#include "lob/price_level.hpp"
#include "lob/types.hpp"

namespace lob {

class OrderBook {
public:

    explicit OrderBook(
        std::size_t expected_orders = 0
    );
    std::vector<Trade> add_order(
        OrderId id,
        Side side,
        Price price,
        Quantity quantity
    );
    
    bool cancel_order(OrderId id);

    std::vector<Trade> modify_order(
        OrderId id,
        Price new_price,
        Quantity new_quantity
    );
    [[nodiscard]]
    bool validate_invariants() const;
    [[nodiscard]]
    std::optional<Price> best_bid() const {
        if (bids_.empty()) {
            return std::nullopt;
        }

        return bids_.begin()->first;
    }

    [[nodiscard]]
    std::optional<Price> best_ask() const {
        if (asks_.empty()) {
            return std::nullopt;
        }

        return asks_.begin()->first;
    }

    [[nodiscard]]
    std::optional<Price> spread() const {
        const auto bid = best_bid();
        const auto ask = best_ask();

        if (!bid || !ask) {
            return std::nullopt;
        }

        return *ask - *bid;
    }
    [[nodiscard]]
    Quantity bid_depth(Price price) const {
        const auto it = bids_.find(price);

        if (it == bids_.end()) {
            return 0;
        }

        return it->second.total_quantity;
    }

    [[nodiscard]]
    Quantity ask_depth(Price price) const {
        const auto it = asks_.find(price);

        if (it == asks_.end()) {
            return 0;
        }

        return it->second.total_quantity;
    }
    std::size_t order_count() const noexcept {
        return order_index_.size();
    }

private:
    using BidBook =
        std::map<
            Price,
            PriceLevel,
            std::greater<Price>
        >;

    using AskBook =
        std::map<Price, PriceLevel>;

    using OrderIterator =
        std::list<Order>::iterator;

    struct OrderLocator {
        Side side{};
        Price price{};
        OrderIterator iterator;
    };

    using OrderIndex =
        std::unordered_map<OrderId, OrderLocator>;

    using OrderIndexIterator =
        OrderIndex::iterator;

    void rest_order(
        OrderId id,
        Side side,
        Price price,
        Quantity quantity
    );

    void cancel_order(
        OrderIndexIterator index_it
    );

    std::vector<Trade> add_order_unchecked(
        OrderId id,
        Side side,
        Price price,
        Quantity quantity
    );

    BidBook bids_;
    AskBook asks_;
    OrderIndex order_index_;
    SequenceNumber next_sequence_{0};
};

} // namespace lob