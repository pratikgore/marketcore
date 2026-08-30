#pragma once

#include "struct.hpp"
#include "CommonFunc.hpp"
#include "nlohmann/json.hpp"
#include <string>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;



/**
 * @brief Parse Binance WebSocket snapshot response into stMarketDataMessage
 * Note: Symbol is NOT in the snapshot response JSON; connector must set it
 * based on its subscription context.
 */
inline stMarketDataMessage ParseSnapshotJson(const std::string& json_str)
{
    stMarketDataMessage msg;
    msg.messageType = eMarketDataMessageType::SNAPSHOT;
    // Symbol will be set by connector thread after parsing
    msg.symbol = "";  // Empty; connector sets this
    msg.receive_timestamp_ns = NowNs();
    msg.exchange_event_time_ms = 0;
    msg.parse_success = true;
    msg.bid_count = 0;
    msg.ask_count = 0;

    try {
        json j = json::parse(json_str);

        // Extract lastUpdateId from result.lastUpdateId
        if (j.contains("result") && j["result"].contains("lastUpdateId")) {
            msg.lastUpdateId = j["result"]["lastUpdateId"].get<uint64_t>();
        } else {
            msg.parse_success = false;
            msg.parse_error_msg = "Missing result.lastUpdateId";
            return msg;
        }

        // Parse bids (up to MAX_LEVELS = 20)
        if (j["result"].contains("bids") && j["result"]["bids"].is_array()) {
            const auto& bids_array = j["result"]["bids"];
            msg.bid_count = std::min(static_cast<uint32_t>(bids_array.size()), 
                                     stMarketDataMessage::MAX_LEVELS);
            
            for (uint32_t i = 0; i < msg.bid_count; ++i) {
                const auto& level = bids_array[i];
                if (level.is_array() && level.size() >= 2) {
                    msg.bids[i].price = level[0].get<std::string>();
                    msg.bids[i].qty = level[1].get<std::string>();
                } else {
                    msg.parse_success = false;
                    msg.parse_error_msg = "Invalid bid level format at index " + std::to_string(i);
                    return msg;
                }
            }
        }

        // Parse asks (up to MAX_LEVELS = 20)
        if (j["result"].contains("asks") && j["result"]["asks"].is_array()) {
            const auto& asks_array = j["result"]["asks"];
            msg.ask_count = std::min(static_cast<uint32_t>(asks_array.size()), 
                                     stMarketDataMessage::MAX_LEVELS);
            
            for (uint32_t i = 0; i < msg.ask_count; ++i) {
                const auto& level = asks_array[i];
                if (level.is_array() && level.size() >= 2) {
                    msg.asks[i].price = level[0].get<std::string>();
                    msg.asks[i].qty = level[1].get<std::string>();
                } else {
                    msg.parse_success = false;
                    msg.parse_error_msg = "Invalid ask level format at index " + std::to_string(i);
                    return msg;
                }
            }
        }

        msg.parse_success = true;
        msg.parse_error_msg = "";

    } catch (const json::exception& e) {
        msg.parse_success = false;
        msg.parse_error_msg = std::string("JSON parse error: ") + e.what();
    } catch (const std::exception& e) {
        msg.parse_success = false;
        msg.parse_error_msg = std::string("Exception: ") + e.what();
    }

    return msg;
}
/**
 * @brief Parse Binance WebSocket depth update (incremental) into stMarketDataMessage
 */
inline stMarketDataMessage ParseDepthUpdateJson(const std::string& json_str)
{
    stMarketDataMessage msg;
    msg.messageType = eMarketDataMessageType::INCREMENTAL_DEPTH_UPDATE;
    msg.receive_timestamp_ns = NowNs();
    msg.lastUpdateId = 0;
    msg.parse_success = true;
    msg.bid_count = 0;
    msg.ask_count = 0;

    try {
        json j = json::parse(json_str);

        // Binance can send subscribe ACK/control payloads like:
        // {"result":null,"id":1}
        // These are not market-data updates.
        if (j.contains("result") && j.contains("id")) {
            msg.parse_success = false;
            msg.parse_error_msg = "CONTROL_ACK";
            return msg;
        }

        // Combined stream format wraps event under "data":
        // {"stream":"btcusdt@depth","data":{...depthUpdate...}}
        if (j.contains("data") && j["data"].is_object()) {
            j = j["data"];
        }

        // Extract symbol (s field)
        if (j.contains("s")) {
            msg.symbol = j["s"].get<std::string>();
        } else {
            msg.parse_success = false;
            msg.parse_error_msg = "Missing symbol field (s)";
            return msg;
        }

        // Exchange event time (E field, milliseconds)
        if (j.contains("E")) {
            msg.exchange_event_time_ms = j["E"].get<uint64_t>();
        }

        // Extract U (first update id)
        if (j.contains("U")) {
            msg.U = j["U"].get<uint64_t>();
        } else {
            msg.parse_success = false;
            msg.parse_error_msg = "Missing U (first update id)";
            return msg;
        }

        // Extract u (final update id)
        if (j.contains("u")) {
            msg.u = j["u"].get<uint64_t>();
        } else {
            msg.parse_success = false;
            msg.parse_error_msg = "Missing u (final update id)";
            return msg;
        }

        // Parse bid changes (b field)
        if (j.contains("b") && j["b"].is_array()) {
            const auto& bids_array = j["b"];
            msg.bid_count = std::min(static_cast<uint32_t>(bids_array.size()), 
                                     stMarketDataMessage::MAX_LEVELS);
            
            for (uint32_t i = 0; i < msg.bid_count; ++i) {
                const auto& level = bids_array[i];
                if (level.is_array() && level.size() >= 2) {
                    msg.bids[i].price = level[0].get<std::string>();
                    msg.bids[i].qty = level[1].get<std::string>();
                } else {
                    msg.parse_success = false;
                    msg.parse_error_msg = "Invalid bid change format at index " + std::to_string(i);
                    return msg;
                }
            }
        }

        // Parse ask changes (a field)
        if (j.contains("a") && j["a"].is_array()) {
            const auto& asks_array = j["a"];
            msg.ask_count = std::min(static_cast<uint32_t>(asks_array.size()), 
                                     stMarketDataMessage::MAX_LEVELS);
            
            for (uint32_t i = 0; i < msg.ask_count; ++i) {
                const auto& level = asks_array[i];
                if (level.is_array() && level.size() >= 2) {
                    msg.asks[i].price = level[0].get<std::string>();
                    msg.asks[i].qty = level[1].get<std::string>();
                } else {
                    msg.parse_success = false;
                    msg.parse_error_msg = "Invalid ask change format at index " + std::to_string(i);
                    return msg;
                }
            }
        }

        msg.parse_success = true;
        msg.parse_error_msg = "";

    } catch (const json::exception& e) {
        msg.parse_success = false;
        msg.parse_error_msg = std::string("JSON parse error: ") + e.what();
    } catch (const std::exception& e) {
        msg.parse_success = false;
        msg.parse_error_msg = std::string("Exception: ") + e.what();
    }

    return msg;
}

/**
 * @brief Utility to print stMarketDataMessage for debugging
 */
inline std::string MessageToString(const stMarketDataMessage& msg)
{
    std::ostringstream oss;
    
    oss << "\n========== MARKET DATA MESSAGE ==========\n";
    oss << "Symbol: " << msg.symbol << "\n";
    oss << "Type: " << (msg.messageType == eMarketDataMessageType::SNAPSHOT ? "SNAPSHOT" : "INCREMENTAL_UPDATE") << "\n";
    oss << "Parse Success: " << (msg.parse_success ? "YES" : "NO") << "\n";
    
    if (!msg.parse_success) {
        oss << "Error: " << msg.parse_error_msg << "\n";
        oss << "==========================================\n";
        return oss.str();
    }
    
    if (msg.messageType == eMarketDataMessageType::SNAPSHOT) {
        oss << "LastUpdateId: " << msg.lastUpdateId << "\n";
    } else {
        oss << "First UpdateId (U): " << msg.U << "\n";
        oss << "Final UpdateId (u): " << msg.u << "\n";
    }
    
    // Header
    oss << "\n";
    oss << std::left << std::setw(15) << "BID Qty" 
        << std::setw(15) << "BID Price" 
        << std::setw(15) << "ASK Price" 
        << std::setw(15) << "ASK Qty" << "\n";
    
    oss << std::string(60, '-') << "\n";
    
    // Display bid/ask pairs side by side
    uint32_t max_levels = std::max(msg.bid_count, msg.ask_count);
    
    for (uint32_t i = 0; i < max_levels; ++i) {
        // BID side (Qty, Price)
        if (i < msg.bid_count) {
            oss << std::left << std::setw(15) << msg.bids[i].qty 
                << std::setw(15) << msg.bids[i].price;
        } else {
            oss << std::left << std::setw(15) << "" 
                << std::setw(15) << "";
        }
        
        // ASK side (Price, Qty)
        if (i < msg.ask_count) {
            oss << std::left << std::setw(15) << msg.asks[i].price 
                << std::setw(15) << msg.asks[i].qty;
        } else {
            oss << std::left << std::setw(15) << "" 
                << std::setw(15) << "";
        }
        
        oss << "\n";
    }
    
    oss << "==========================================\n";
    return oss.str();
}