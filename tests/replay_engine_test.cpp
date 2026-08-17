#include <gtest/gtest.h>

#include <vector>

#include "lob/replay_engine.hpp"

TEST(ReplayEngineTest, ReplaysMarketEvents) {
    const std::vector<lob::MarketEvent> events{
        {
            .type = lob::EventType::Add,
            .order_id = 1,
            .side = lob::Side::Buy,
            .price = 10050,
            .quantity = 100
        },
        {
            .type = lob::EventType::Add,
            .order_id = 2,
            .side = lob::Side::Sell,
            .price = 10060,
            .quantity = 200
        },
        {
            .type = lob::EventType::Modify,
            .order_id = 1,
            .price = 10060,
            .quantity = 100
        }
    };

    lob::ReplayEngine engine;

    const auto stats =
        engine.replay(events);

    EXPECT_EQ(stats.events_processed, 3);
    EXPECT_EQ(stats.trades_generated, 1);

    const auto& book =
        engine.order_book();

    EXPECT_EQ(book.order_count(), 1);
    EXPECT_EQ(book.ask_depth(10060), 100);

    EXPECT_TRUE(book.validate_invariants());
}

TEST(
    ReplayEngineTest,
    ProcessesSingleEvent
) {
    lob::ReplayEngine engine;

    const lob::MarketEvent event{
        .type = lob::EventType::Add,
        .order_id = 1,
        .side = lob::Side::Buy,
        .price = 10050,
        .quantity = 100
    };

    const auto trades_generated =
        engine.process_event(event);

    EXPECT_EQ(trades_generated, 0);

    const auto& book =
        engine.order_book();

    EXPECT_EQ(book.order_count(), 1);
    EXPECT_EQ(book.bid_depth(10050), 100);
    EXPECT_TRUE(book.validate_invariants());
}