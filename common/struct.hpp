#pragma once    

#include <cstdint>
#include <string>
#include <array>

/*
* @brief Defines all common enums and structures used for utilties
*
* @author Pratik Gore
* @date  9-July-2026
*/

//Defines log level used by logger
enum class eLogLevel
{
    DEBUG,
    WARN,
    INFO
};

//Client events
enum class eClientEvent
{
    SUBSCRIBE,
    UNSUBSCRIBE,
    SNAPSHOT
};

enum class eAction
{
    ADD,
    MODIFY,
    DELETE,
    TRADE
};

//Price level for L2 book levels (numeric version)
struct stPriceLevel
{
    double price;
    uint32_t qty;
};

/**
 * @brief Market data message type discriminator
 * Used to distinguish snapshot from incremental depth updates
 */
enum class eMarketDataMessageType
{
    SNAPSHOT,
    INCREMENTAL_DEPTH_UPDATE
};

/**
 * @brief Price level representation using strings for precision
 * Prices and quantities are kept as strings from Binance to avoid precision loss
 * during intermediate processing. Conversion to numeric types deferred to order-book layer.
 */
struct stPriceLevelStr
{
    std::string price;   // e.g., "71925.99000000"
    std::string qty;     // e.g., "4.32910000"
};

/**
 * @brief Common market data message structure
 * 
 * This is the normalized internal message type used across SPSC queues.
 * A single message can represent either:
 *   - A snapshot with full order book state (lastUpdateId, bids, asks)
 *   - An incremental depth update (U, u, bid changes, ask changes)
 *
 * The discriminator is the messageType field.
 * 
 * Fixed-size arrays avoid heap allocations and reduce latency jitter.
 * Uses level counts to track how many of the 20 slots are actually populated.
 * 
 * Snapshots:
 *   - messageType = SNAPSHOT
 *   - symbol, lastUpdateId, bids, asks are populated
 *   - bid_count, ask_count indicate actual populated levels
 *   - U, u are set to 0 (unused)
 *
 * Incremental updates:
 *   - messageType = INCREMENTAL_DEPTH_UPDATE
 *   - symbol, U, u, bids, asks are populated (bids/asks are deltas)
 *   - bid_count, ask_count indicate number of changed levels
 *   - lastUpdateId is set to 0 (unused)
 */
struct stMarketDataMessage
{
    // Message metadata
    eMarketDataMessageType messageType;     // SNAPSHOT or INCREMENTAL_DEPTH_UPDATE
    std::string symbol;                     // e.g., "BTCUSDT"
    
    // Timestamps
    std::uint64_t receive_timestamp_ns;     // When received by feed handler (NowNs())
    std::uint64_t exchange_event_time_ms;   // Binance event timestamp (E field)
    
    // Snapshot-specific fields (used when messageType == SNAPSHOT)
    std::uint64_t lastUpdateId;             // Final update id of the snapshot
    
    // Incremental update-specific fields (used when messageType == INCREMENTAL_DEPTH_UPDATE)
    std::uint64_t U;                        // First update id in this event
    std::uint64_t u;                        // Final update id in this event
    
    // Order book levels (fixed-size array to avoid heap allocation)
    // Top 20 levels on each side (bids descending, asks ascending)
    static constexpr uint32_t MAX_LEVELS = 20;
    std::array<stPriceLevelStr, MAX_LEVELS> bids;   // Bid side price levels
    std::array<stPriceLevelStr, MAX_LEVELS> asks;   // Ask side price levels
    uint32_t bid_count{0};                  // Number of populated bid levels
    uint32_t ask_count{0};                  // Number of populated ask levels
    
    // Parse/ingestion status
    bool parse_success{true};               // true if parsing succeeded, false otherwise
    std::string parse_error_msg;            // Error message if parse_success == false
};