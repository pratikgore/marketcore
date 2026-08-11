#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "includes/clsWSBNConnect.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


clsWSBNConnect::clsWSBNConnect() {}
clsWSBNConnect::~clsWSBNConnect() {}


void clsWSBNConnect::HandleSession()
{
    // These objects perform our I/O
    // The io_context is required for all I/O
    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};
    tcp::resolver resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};

    std::string host = "stream.binance.com";
    std::string port = "443";
    std::string target = "/ws";

    ctx.set_default_verify_paths();
    ws.next_layer().set_verify_mode(ssl::verify_peer);
    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    auto ep = net::connect(ws.next_layer().next_layer(), results);

    if(!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()));
    }

    ws.next_layer().handshake(ssl::stream_base::client);

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host += ':' + std::to_string(ep.port());

    // Set a decorator to change the User-Agent of the handshake
    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

    // Perform the websocket handshake
    ws.handshake(host, target);

    // Send the message
    std::cout << "Connected to Binance\n";

    // Subscribe to BTCUSDT trades
    std::string request = R"({
        "method": "SUBSCRIBE",
        "params": [
            "btcusdt@trade"
        ],
        "id": 1
    })";

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
        ws.write(net::buffer(request));

    // Receive messages forever
    for (;;)
    {
        beast::flat_buffer buffer;

        ws.read(buffer);

        std::cout
            << beast::make_printable(buffer.data())
            << '\n';
    }
}


