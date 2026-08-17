#include <gtest/gtest.h>

#include "lob/event_generator.hpp"

TEST(EventGeneratorTest, GeneratesRequestedEventCount) {
    lob::EventGeneratorConfig config;

    config.event_count = 10'000;

    const auto events =
        lob::SyntheticEventGenerator::generate(
            config
        );

    EXPECT_EQ(events.size(), 10'000);
}

TEST(EventGeneratorTest, IsDeterministicForSameSeed) {
    lob::EventGeneratorConfig config;

    config.event_count = 1'000;
    config.seed = 12345;

    const auto first =
        lob::SyntheticEventGenerator::generate(
            config
        );

    const auto second =
        lob::SyntheticEventGenerator::generate(
            config
        );

    ASSERT_EQ(first.size(), second.size());

    for (std::size_t i = 0; i < first.size(); ++i) {
        EXPECT_EQ(first[i].type, second[i].type);
        EXPECT_EQ(
            first[i].order_id,
            second[i].order_id
        );
        EXPECT_EQ(first[i].side, second[i].side);
        EXPECT_EQ(first[i].price, second[i].price);
        EXPECT_EQ(
            first[i].quantity,
            second[i].quantity
        );
    }
}

TEST(EventGeneratorTest, AddEventsHaveValidValues) {
    lob::EventGeneratorConfig config;

    config.event_count = 10'000;
    config.min_price = 9900;
    config.max_price = 10100;
    config.min_quantity = 1;
    config.max_quantity = 500;

    const auto events =
        lob::SyntheticEventGenerator::generate(
            config
        );

    for (const auto& event : events) {
        if (event.type != lob::EventType::Add) {
            continue;
        }

        EXPECT_GT(event.order_id, 0);

        EXPECT_GE(event.price, 9900);
        EXPECT_LE(event.price, 10100);

        EXPECT_GE(event.quantity, 1);
        EXPECT_LE(event.quantity, 500);
    }
}