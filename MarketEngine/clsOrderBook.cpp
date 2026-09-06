#include "clsOrderBook.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

clsOrderBook::clsOrderBook(std::string symbol)
    : m_symbol(std::move(symbol))
{
}

template<typename MapT>
void clsOrderBook::ApplyLevels(const stPriceLevelStr* levels, uint32_t count, MapT& side)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        const double price = std::stod(levels[i].price);
        const double qty = std::stod(levels[i].qty);

        if (qty == 0.0)
        {
            side.erase(price);
        }
        else
        {
            side[price] = qty;
        }
    }
}

void clsOrderBook::ApplySnapshot(const stMarketDataMessage& msg)
{
    // operator[] on the owning map default-constructs with an empty symbol; sync it here.
    if (m_symbol.empty())
    {
        m_symbol = msg.symbol;
    }

    m_bids.clear();
    m_asks.clear();

    // Snapshot levels always represent absolute state, never zero-qty deletions.
    for (uint32_t i = 0; i < msg.bid_count; ++i)
    {
        m_bids[std::stod(msg.bids[i].price)] = std::stod(msg.bids[i].qty);
    }
    for (uint32_t i = 0; i < msg.ask_count; ++i)
    {
        m_asks[std::stod(msg.asks[i].price)] = std::stod(msg.asks[i].qty);
    }

    m_lastAppliedUpdateId = msg.lastUpdateId;
}

void clsOrderBook::ApplyIncremental(const stMarketDataMessage& msg)
{
    ApplyLevels(msg.bids.data(), msg.bid_count, m_bids);
    ApplyLevels(msg.asks.data(), msg.ask_count, m_asks);

    m_lastAppliedUpdateId = msg.u;
}

std::string clsOrderBook::ToString(uint32_t topN) const
{
    // Maps are already sorted (bids descending, asks ascending); just cap to topN.
    std::vector<std::pair<double, double>> bidLevels(m_bids.begin(),
        m_bids.size() > topN ? std::next(m_bids.begin(), topN) : m_bids.end());
    std::vector<std::pair<double, double>> askLevels(m_asks.begin(),
        m_asks.size() > topN ? std::next(m_asks.begin(), topN) : m_asks.end());

    std::ostringstream oss;
    oss << "\n========== ORDER BOOK (" << m_symbol << ") ==========\n";
    oss << "LastAppliedUpdateId: " << m_lastAppliedUpdateId << "\n\n";
    oss << std::fixed << std::setprecision(8); // avoid scientific notation for small qty/price values

    oss << std::left << std::setw(15) << "BID Qty"
        << std::setw(15) << "BID Price"
        << std::setw(15) << "ASK Price"
        << std::setw(15) << "ASK Qty" << "\n";
    oss << std::string(60, '-') << "\n";

    const std::size_t maxLevels = std::max(bidLevels.size(), askLevels.size());
    for (std::size_t i = 0; i < maxLevels; ++i)
    {
        if (i < bidLevels.size())
        {
            oss << std::left << std::setw(15) << bidLevels[i].second
                << std::setw(15) << bidLevels[i].first;
        }
        else
        {
            oss << std::left << std::setw(15) << "" << std::setw(15) << "";
        }

        if (i < askLevels.size())
        {
            oss << std::left << std::setw(15) << askLevels[i].first
                << std::setw(15) << askLevels[i].second;
        }
        else
        {
            oss << std::left << std::setw(15) << "" << std::setw(15) << "";
        }
        oss << "\n";
    }

    oss << "==========================================\n";
    return oss.str();
}
