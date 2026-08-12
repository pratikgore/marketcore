
#include "includes/clsWSBNConnector.hpp"
#include "../common/ErrorCode.hpp"

clsWSBNConnector::clsWSBNConnector() {}
clsWSBNConnector::~clsWSBNConnector() {}

long clsWSBNConnector::CreateSession()
{
    std::cout <<"Biance WS API session init .." << std::endl;

    // Initialize transport objects for this session using member-owned pointers.
    m_ptIOC = std::make_unique<net::io_context>();
    m_ptContext = std::make_unique<ssl::context>(ssl::context::tlsv12_client);
    m_ptResolver = std::make_unique<tcp::resolver>(*m_ptIOC);
    m_ptWSession = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(*m_ptIOC, *m_ptContext);

    m_ptContext->set_default_verify_paths();
    m_ptWSession->next_layer().set_verify_mode(ssl::verify_peer);
    // Look up the domain name
    auto const results = m_ptResolver->resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    auto ep = net::connect(m_ptWSession->next_layer().next_layer(), results);

    if(!SSL_set_tlsext_host_name(m_ptWSession->next_layer().native_handle(), host.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()));
    }

    m_ptWSession->next_layer().handshake(ssl::stream_base::client);
    std::string hostWithPort = host + ':' + std::to_string(ep.port());

    // Set a decorator to change the User-Agent of the handshake
    m_ptWSession->set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

    // Perform the websocket handshake
    m_ptWSession->handshake(hostWithPort, target);

    // Send the message
    std::cout << "Connected to Binance\n";
    return SUCCESS;
}

void clsWSBNConnector::ReadFeed()
{
    std::cout << "Reading WS feed ..." <<std::endl;
    // Receive messages forever
    for (;;)
    {
        beast::flat_buffer buffer;

        m_ptWSession->read(buffer);

        std::cout
            << beast::make_printable(buffer.data())
            << '\n';
    }
}

void clsWSBNConnector::Subscribe(std::vector<std::string>& symbols)
{
    // Subscribe to BTCUSDT trades
    std::string request = R"({
        "method": "SUBSCRIBE",
        "params": [
            "btcusdt@depth"
        ],
        "id": 1
    })";

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
    m_ptWSession->write(net::buffer(request));

}

void clsWSBNConnector::UnSubscribe(std::vector<std::string>& symbols)
{

}
