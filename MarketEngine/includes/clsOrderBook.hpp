#pragma once

#include <string>
#include <map>
#include <functional>
#include <cstdint>

#include "struct.hpp"

/**
 * @brief Per-symbol in-memory L2 order book built from Binance snapshot + incremental deltas
 */
class clsOrderBook
{
    public:
        explicit clsOrderBook(std::string symbol = "");

        void ApplySnapshot(const stMarketDataMessage& msg);
        void ApplyIncremental(const stMarketDataMessage& msg);

        std::string ToString(uint32_t topN = 20) const;

        const std::string& Symbol() const { return m_symbol; }
        std::uint64_t LastAppliedUpdateId() const { return m_lastAppliedUpdateId; }

    private:
        // Bids sorted descending (best bid first), asks ascending (best ask first).
        using BidMap = std::map<double, double, std::greater<double>>;
        using AskMap = std::map<double, double>;

        template<typename MapT>
        void ApplyLevels(const stPriceLevelStr* levels, uint32_t count, MapT& side);

        std::string m_symbol;
        BidMap m_bids; // price -> qty
        AskMap m_asks; // price -> qty
        std::uint64_t m_lastAppliedUpdateId{0};
};
