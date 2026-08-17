#include <gtest/gtest.h>

#include <cstdint>
#include <random>

#include "lob/order_book.hpp"

TEST(
    InvariantTest,
    RemainsValidUnderRandomEventStream
) {
    lob::OrderBook book;

    // Fixed seed makes failures reproducible.
    std::mt19937_64 rng(0xC0FFEE);

    std::uniform_int_distribution<int>
        event_distribution(0, 99);

    std::uniform_int_distribution<int>
        side_distribution(0, 1);

    std::uniform_int_distribution<lob::Price>
        price_distribution(9900, 10100);

    std::uniform_int_distribution<lob::Quantity>
        quantity_distribution(1, 500);

    lob::OrderId next_order_id = 1;

    constexpr std::size_t event_count = 50'000;

    for (
        std::size_t event = 0;
        event < event_count;
        ++event
    ) {
        const int event_type =
            event_distribution(rng);

        if (event_type < 55) {
            // ADD

            const auto side =
                side_distribution(rng) == 0
                    ? lob::Side::Buy
                    : lob::Side::Sell;

            book.add_order(
                next_order_id++,
                side,
                price_distribution(rng),
                quantity_distribution(rng)
            );
        } else if (event_type < 75) {
            // CANCEL

            if (next_order_id > 1) {
                std::uniform_int_distribution<
                    lob::OrderId
                > id_distribution(
                    1,
                    next_order_id - 1
                );

                book.cancel_order(
                    id_distribution(rng)
                );
            }
        } else {
            // MODIFY

            if (next_order_id > 1) {
                std::uniform_int_distribution<
                    lob::OrderId
                > id_distribution(
                    1,
                    next_order_id - 1
                );

                lob::Quantity new_quantity =
                    quantity_distribution(rng);

                // Occasionally modify quantity to zero,
                // which should act as cancellation.
                if (
                    event_distribution(rng) < 10
                ) {
                    new_quantity = 0;
                }

                book.modify_order(
                    id_distribution(rng),
                    price_distribution(rng),
                    new_quantity
                );
            }
        }

        ASSERT_TRUE(
            book.validate_invariants()
        )
            << "Invariant violation after event "
            << event;
    }
}