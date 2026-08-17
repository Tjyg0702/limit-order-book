#include <iostream>

#include "lob/order_book.hpp"

int main() {
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

    std::cout << "Depth after modify: "
              << book.ask_depth(10050)
              << '\n';

    const auto trades = book.add_order(
        10,
        lob::Side::Buy,
        10050,
        100
    );

    for (const auto& trade : trades) {
        std::cout
            << "Maker=" << trade.maker_order_id
            << " Taker=" << trade.taker_order_id
            << " Price=" << trade.price
            << " Quantity=" << trade.quantity
            << '\n';
    }

    return 0;
}