#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "lob/event.hpp"

namespace lob {

struct EventGeneratorConfig {
    std::size_t event_count{100'000};

    std::uint64_t seed{0xC0FFEE};

    Price min_price{9900};
    Price max_price{10100};

    Quantity min_quantity{1};
    Quantity max_quantity{500};

    std::uint32_t add_percentage{60};
    std::uint32_t cancel_percentage{20};
};

class SyntheticEventGenerator {
public:
    static std::vector<MarketEvent> generate(
        const EventGeneratorConfig& config
    );
};

} // namespace lob