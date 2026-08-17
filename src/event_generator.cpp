#include "lob/event_generator.hpp"

#include <random>
#include <utility>
#include <vector>

namespace lob {

std::vector<MarketEvent> SyntheticEventGenerator::generate(
    const EventGeneratorConfig& config
) {
    std::vector<MarketEvent> events;
    events.reserve(config.event_count);

    if (
        config.event_count == 0 ||
        config.min_price > config.max_price ||
        config.min_quantity > config.max_quantity ||
        config.add_percentage +
                config.cancel_percentage >
            100
    ) {
        return events;
    }

    std::mt19937_64 rng(config.seed);

    std::uniform_int_distribution<int>
        event_distribution(0, 99);

    std::uniform_int_distribution<int>
        side_distribution(0, 1);

    std::uniform_int_distribution<Price>
        price_distribution(
            config.min_price,
            config.max_price
        );

    std::uniform_int_distribution<Quantity>
        quantity_distribution(
            config.min_quantity,
            config.max_quantity
        );

    OrderId next_order_id = 1;

    std::vector<OrderId> candidate_order_ids;
    candidate_order_ids.reserve(
        config.event_count
    );

    for (
        std::size_t i = 0;
        i < config.event_count;
        ++i
    ) {
        const int event_roll =
            event_distribution(rng);

        const bool must_add =
            candidate_order_ids.empty();

        if (
            must_add ||
            event_roll <
                static_cast<int>(
                    config.add_percentage
                )
        ) {
            const Side side =
                side_distribution(rng) == 0
                    ? Side::Buy
                    : Side::Sell;

            const OrderId id =
                next_order_id++;

            events.push_back({
                .type = EventType::Add,
                .order_id = id,
                .side = side,
                .price = price_distribution(rng),
                .quantity =
                    quantity_distribution(rng)
            });

            candidate_order_ids.push_back(id);

            continue;
        }

        std::uniform_int_distribution<std::size_t>
            id_distribution(
                0,
                candidate_order_ids.size() - 1
            );

        const std::size_t id_index =
            id_distribution(rng);

        const OrderId id =
            candidate_order_ids[id_index];

        const auto cancel_boundary =
            config.add_percentage +
            config.cancel_percentage;

        if (
            event_roll <
            static_cast<int>(cancel_boundary)
        ) {
            events.push_back({
                .type = EventType::Cancel,
                .order_id = id
            });

            candidate_order_ids[id_index] =
                candidate_order_ids.back();

            candidate_order_ids.pop_back();

            continue;
        }

        events.push_back({
            .type = EventType::Modify,
            .order_id = id,
            .price = price_distribution(rng),
            .quantity =
                quantity_distribution(rng)
        });
    }

    return events;
}

} // namespace lob