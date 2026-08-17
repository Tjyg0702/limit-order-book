#include <gtest/gtest.h>

#include "lob/order_book.hpp"

TEST(MatchingTest, MatchesOrdersUsingFifoAtSamePrice) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Sell,
        10050,
        100
    );

    book.add_order(
        2,
        lob::Side::Sell,
        10050,
        100
    );

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10050,
        150
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].taker_order_id, 10);
    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(trades[1].maker_order_id, 2);
    EXPECT_EQ(trades[1].taker_order_id, 10);
    EXPECT_EQ(trades[1].quantity, 50);

    EXPECT_EQ(book.ask_depth(10050), 50);
}

TEST(MatchingTest, HandlesPartialFill) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Sell,
        10050,
        200
    );

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10050,
        50
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(book.ask_depth(10050), 150);
    EXPECT_EQ(book.order_count(), 1);
}

TEST(MatchingTest, RemovesFullyFilledOrders) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Sell,
        10050,
        100
    );

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10050,
        100
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.ask_depth(10050), 0);
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST(MatchingTest, ExecutesAtRestingMakerPrice) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Sell,
        10050,
        100
    );

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10100,
        100
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].price, 10050);
    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].taker_order_id, 10);
}

TEST(MatchingTest, BuySweepsMultipleAskLevels) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Sell,
        10050,
        100
    );

    book.add_order(
        2,
        lob::Side::Sell,
        10060,
        200
    );

    book.add_order(
        3,
        lob::Side::Sell,
        10070,
        300
    );

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10060,
        400
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].price, 10050);
    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(trades[1].maker_order_id, 2);
    EXPECT_EQ(trades[1].price, 10060);
    EXPECT_EQ(trades[1].quantity, 200);

    EXPECT_EQ(book.ask_depth(10050), 0);
    EXPECT_EQ(book.ask_depth(10060), 0);
    EXPECT_EQ(book.ask_depth(10070), 300);

    EXPECT_EQ(book.bid_depth(10060), 100);

    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());

    EXPECT_EQ(*book.best_bid(), 10060);
    EXPECT_EQ(*book.best_ask(), 10070);
}

TEST(MatchingTest, SellSweepsMultipleBidLevels) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10070,
        100
    );

    book.add_order(
        2,
        lob::Side::Buy,
        10060,
        200
    );

    book.add_order(
        3,
        lob::Side::Buy,
        10040,
        300
    );

    const auto trades = book.add_order(
        10,
        lob::Side::Sell,
        10060,
        400
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].price, 10070);
    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(trades[1].maker_order_id, 2);
    EXPECT_EQ(trades[1].price, 10060);
    EXPECT_EQ(trades[1].quantity, 200);

    EXPECT_EQ(book.bid_depth(10070), 0);
    EXPECT_EQ(book.bid_depth(10060), 0);
    EXPECT_EQ(book.bid_depth(10040), 300);

    EXPECT_EQ(book.ask_depth(10060), 100);

    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());

    EXPECT_EQ(*book.best_bid(), 10040);
    EXPECT_EQ(*book.best_ask(), 10060);
}

TEST(MatchingTest, NonCrossingOrdersRemainResting) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Sell,
        10100,
        100
    );

    const auto trades = book.add_order(
        2,
        lob::Side::Buy,
        10090,
        200
    );

    EXPECT_TRUE(trades.empty());

    EXPECT_EQ(book.order_count(), 2);

    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());

    EXPECT_EQ(*book.best_bid(), 10090);
    EXPECT_EQ(*book.best_ask(), 10100);

    ASSERT_TRUE(book.spread().has_value());
    EXPECT_EQ(*book.spread(), 10);
}

TEST(MatchingTest, ResidualTakerQuantityRestsInBook) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Sell,
        10050,
        100
    );

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10060,
        150
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(book.ask_depth(10050), 0);
    EXPECT_EQ(book.bid_depth(10060), 50);

    EXPECT_EQ(book.order_count(), 1);

    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(*book.best_bid(), 10060);
}