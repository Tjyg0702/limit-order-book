#include <gtest/gtest.h>

#include "lob/order_book.hpp"

TEST(OrderBookTest, StartsEmpty) {
    lob::OrderBook book;

    EXPECT_EQ(book.order_count(), 0);

    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_FALSE(book.spread().has_value());
}

TEST(OrderBookTest, AddsRestingOrders) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    book.add_order(
        2,
        lob::Side::Sell,
        10100,
        200
    );

    EXPECT_EQ(book.order_count(), 2);

    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());

    EXPECT_EQ(*book.best_bid(), 10050);
    EXPECT_EQ(*book.best_ask(), 10100);
}

TEST(OrderBookTest, SelectsCorrectBestBidAndAsk) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10020,
        100
    );

    book.add_order(
        2,
        lob::Side::Buy,
        10050,
        100
    );

    book.add_order(
        3,
        lob::Side::Buy,
        10030,
        100
    );

    book.add_order(
        4,
        lob::Side::Sell,
        10120,
        100
    );

    book.add_order(
        5,
        lob::Side::Sell,
        10100,
        100
    );

    book.add_order(
        6,
        lob::Side::Sell,
        10150,
        100
    );

    ASSERT_TRUE(book.best_bid().has_value());
    ASSERT_TRUE(book.best_ask().has_value());

    EXPECT_EQ(*book.best_bid(), 10050);
    EXPECT_EQ(*book.best_ask(), 10100);
}

TEST(OrderBookTest, CalculatesSpread) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    book.add_order(
        2,
        lob::Side::Sell,
        10100,
        100
    );

    ASSERT_TRUE(book.spread().has_value());

    EXPECT_EQ(*book.spread(), 50);
}

TEST(OrderBookTest, AggregatesMarketDepthAtPriceLevel) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    book.add_order(
        2,
        lob::Side::Buy,
        10050,
        200
    );

    book.add_order(
        3,
        lob::Side::Buy,
        10040,
        300
    );

    EXPECT_EQ(book.bid_depth(10050), 300);
    EXPECT_EQ(book.bid_depth(10040), 300);

    EXPECT_EQ(book.bid_depth(9999), 0);
}

TEST(OrderBookTest, RejectsDuplicateOrderId) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    book.add_order(
        1,
        lob::Side::Buy,
        10060,
        500
    );

    EXPECT_EQ(book.order_count(), 1);

    EXPECT_EQ(book.bid_depth(10050), 100);
    EXPECT_EQ(book.bid_depth(10060), 0);
}

TEST(OrderBookTest, RejectsInvalidOrders) {
    lob::OrderBook book;

    book.add_order(
        0,
        lob::Side::Buy,
        10050,
        100
    );

    book.add_order(
        1,
        lob::Side::Buy,
        0,
        100
    );

    book.add_order(
        2,
        lob::Side::Buy,
        -100,
        100
    );

    book.add_order(
        3,
        lob::Side::Buy,
        10050,
        0
    );

    EXPECT_EQ(book.order_count(), 0);
}

TEST(OrderBookTest, CancelsExistingOrder) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    book.add_order(
        2,
        lob::Side::Buy,
        10050,
        200
    );

    EXPECT_EQ(book.bid_depth(10050), 300);

    EXPECT_TRUE(book.cancel_order(1));

    EXPECT_EQ(book.order_count(), 1);
    EXPECT_EQ(book.bid_depth(10050), 200);
}

TEST(OrderBookTest, RemovingLastOrderRemovesPriceLevel) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    ASSERT_TRUE(book.best_bid().has_value());

    EXPECT_TRUE(book.cancel_order(1));

    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.bid_depth(10050), 0);
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST(OrderBookTest, CancelUnknownOrderReturnsFalse) {
    lob::OrderBook book;

    EXPECT_FALSE(book.cancel_order(999));
}