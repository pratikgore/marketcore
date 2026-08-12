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


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class clsWSBNConnector
{

    public:
        clsWSBNConnector();
        ~clsWSBNConnector();

        long CreateSession();
        void Subscribe(std::vector<std::string>& symbols);
        void UnSubscribe(std::vector<std::string>& symbols);
        void ReadFeed();

    private:
        std::string host {"stream.binance.com"};
        std::string port {"443"};
        std::string target {"/ws"};

        std::unique_ptr<net::io_context> m_ptIOC; //IO operation
        std::unique_ptr<ssl::context> m_ptContext;
        std::unique_ptr<tcp::resolver> m_ptResolver;
        std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> m_ptWSession;
};
