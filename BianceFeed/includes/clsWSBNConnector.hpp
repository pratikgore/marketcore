/**
 * @file clsWSBNConnector.hpp
 * @brief Connector for Binance WS API
 * @author Pratik Gore
 * @date 2026
 */

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

#include "clsFeedCommunicator.hpp"

class clsFeedCommunicator;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class clsWSBNConnector
{

    public:
        clsWSBNConnector(clsFeedCommunicator* communicator);
        ~clsWSBNConnector();

        void Init();
        void Stop();

        long CreateSessionDepth();
        void SubscribeDepth(std::string & symbols);
        long CreateSessionSnap();
        void SubscribeSnap(std::string & symbols, std::uint64_t requestId);
        void ReadSnap(beast::flat_buffer& buffer);
        void UnSubscribe(std::vector<std::string>& symbols);
        void ReadFeed(beast::flat_buffer& buffer);

    private:

        void SnapshotThreadLoop();
        void SnapshotReaderThread();  // Separate thread for continuous reading

        void DepthThreadLoop();
        void DepthReaderThread();
        
        std::thread m_SnapThread;
        std::thread m_SnapReaderThread;

        std::thread m_DepthThread;
        std::thread m_DepthReaderThread;

        std::atomic<bool> m_running = false; //used for snap / depth reader loop
        std::atomic<bool> m_stopped = false;
        std::atomic<uint64_t> m_nextRequestId {1};
        
        
        std::string hostDepth {"stream.binance.com"};
        std::string port {"443"};
        std::string targetDepth {"/ws"};

        std::string hostSnap {"ws-api.binance.com"};
        // std::string port {"443"};
        std::string targetSnap {"/ws-api/v3"};

        std::unique_ptr<net::io_context> m_ptIOC; // Depth IO operation
        std::unique_ptr<ssl::context> m_ptContext;
        std::unique_ptr<tcp::resolver> m_ptResolver;
        std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> m_ptWSession;

        std::unique_ptr<net::io_context> m_ptSnapIOC; // Snapshot IO operation
        std::unique_ptr<ssl::context> m_ptSnapContext;
        std::unique_ptr<tcp::resolver> m_ptSnapResolver;
        std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> m_ptSnapWSession;

        clsFeedCommunicator* m_communicator;
};
