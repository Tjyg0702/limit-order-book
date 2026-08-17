#include <gtest/gtest.h>

#include "lob/order_book.hpp"

TEST(ModifyOrderTest, QuantityDecreasePreservesPriority) {
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

    book.modify_order(
        1,
        10050,
        50
    );

    EXPECT_EQ(book.ask_depth(10050), 150);

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10050,
        60
    );

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].maker_order_id, 1);
    EXPECT_EQ(trades[0].quantity, 50);

    EXPECT_EQ(trades[1].maker_order_id, 2);
    EXPECT_EQ(trades[1].quantity, 10);

    EXPECT_EQ(book.ask_depth(10050), 90);
}

TEST(ModifyOrderTest, QuantityIncreaseLosesPriority) {
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

    book.modify_order(
        1,
        10050,
        150
    );

    EXPECT_EQ(book.ask_depth(10050), 250);

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10050,
        100
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].maker_order_id, 2);
    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(book.ask_depth(10050), 150);
}

TEST(ModifyOrderTest, PriceChangeLosesPriority) {
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
        100
    );

    book.modify_order(
        1,
        10060,
        100
    );

    EXPECT_EQ(book.ask_depth(10050), 0);
    EXPECT_EQ(book.ask_depth(10060), 200);

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10060,
        100
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].maker_order_id, 2);
    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(book.ask_depth(10060), 100);
}

TEST(ModifyOrderTest, PriceChangeCanCrossMarket) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10040,
        100
    );

    book.add_order(
        2,
        lob::Side::Sell,
        10050,
        100
    );

    const auto trades = book.modify_order(
        1,
        10060,
        100
    );

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].maker_order_id, 2);
    EXPECT_EQ(trades[0].taker_order_id, 1);
    EXPECT_EQ(trades[0].price, 10050);
    EXPECT_EQ(trades[0].quantity, 100);

    EXPECT_EQ(book.order_count(), 0);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST(ModifyOrderTest, ZeroQuantityCancelsOrder) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    book.modify_order(
        1,
        10050,
        0
    );

    EXPECT_EQ(book.order_count(), 0);
    EXPECT_EQ(book.bid_depth(10050), 0);
    EXPECT_FALSE(book.best_bid().has_value());
}

TEST(ModifyOrderTest, UnknownOrderDoesNothing) {
    lob::OrderBook book;

    book.add_order(
        1,
        lob::Side::Buy,
        10050,
        100
    );

    const auto trades = book.modify_order(
        999,
        10100,
        500
    );

    EXPECT_TRUE(trades.empty());

    EXPECT_EQ(book.order_count(), 1);
    EXPECT_EQ(book.bid_depth(10050), 100);
    EXPECT_EQ(book.bid_depth(10100), 0);
}