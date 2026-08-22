
#include "clsWSBNConnector.hpp"
#include "ErrorCode.hpp"
#include <algorithm>
#include <cctype>

clsWSBNConnector::clsWSBNConnector() {}
clsWSBNConnector::~clsWSBNConnector() {}

long clsWSBNConnector::CreateSessionDepth()
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
    auto const results = m_ptResolver->resolve(hostDepth, port);

    // Make the connection on the IP address we get from a lookup
    auto ep = net::connect(m_ptWSession->next_layer().next_layer(), results);

    if(!SSL_set_tlsext_host_name(m_ptWSession->next_layer().native_handle(), hostDepth.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()));
    }

    m_ptWSession->next_layer().handshake(ssl::stream_base::client);
    std::string hostWithPort = hostDepth + ':' + std::to_string(ep.port());

    // Set a decorator to change the User-Agent of the handshake
    m_ptWSession->set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

    // Perform the websocket handshake
    m_ptWSession->handshake(hostWithPort, targetDepth);

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

        std::cout << " ================== INCR UPDATE ================= " << std::endl;
        std::cout
            << beast::make_printable(buffer.data())
            << std::endl;
        std::cout << " ================= UPDATE END =================== " << std::endl;
        
    }
}

void clsWSBNConnector::SubscribeDepth(std::string & symbols)
{
    // Build only the dynamic stream name and reuse fixed JSON fragments.
    constexpr std::string_view kPrefix = R"({"method":"SUBSCRIBE","params":[")";
    constexpr std::string_view kSuffix = R"(@depth"],"id":1})";

    std::string streamName = symbols;
    std::transform(streamName.begin(), streamName.end(), streamName.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    std::string request;
    request.reserve(kPrefix.size() + streamName.size() + kSuffix.size());
    request.append(kPrefix);
    request.append(streamName);
    request.append(kSuffix);

    m_ptWSession->write(net::buffer(request));

}

long clsWSBNConnector::CreateSessionSnap()
{
    std::cout << "Biance WS API snapshot session init .." << std::endl;

    m_ptSnapIOC = std::make_unique<net::io_context>();
    m_ptSnapContext = std::make_unique<ssl::context>(ssl::context::tlsv12_client);
    m_ptSnapResolver = std::make_unique<tcp::resolver>(*m_ptSnapIOC);
    m_ptSnapWSession = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(*m_ptSnapIOC, *m_ptSnapContext);

    m_ptSnapContext->set_default_verify_paths();
    m_ptSnapWSession->next_layer().set_verify_mode(ssl::verify_peer);

    auto const results = m_ptSnapResolver->resolve(hostSnap, port);
    auto ep = net::connect(m_ptSnapWSession->next_layer().next_layer(), results);

    if(!SSL_set_tlsext_host_name(m_ptSnapWSession->next_layer().native_handle(), hostSnap.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()));
    }

    m_ptSnapWSession->next_layer().handshake(ssl::stream_base::client);
    std::string hostWithPort = hostSnap + ':' + std::to_string(ep.port());

    m_ptSnapWSession->set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

    m_ptSnapWSession->handshake(hostWithPort, targetSnap);

    std::cout << "Connected to Binance snapshot endpoint\n";
    return SUCCESS;
}

void clsWSBNConnector::SubscribeSnap(std::string & symbols)
{
    std::string symbolUpper = symbols;
    std::transform(symbolUpper.begin(), symbolUpper.end(), symbolUpper.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

    constexpr std::string_view kPrefix = R"({"id":1,"method":"depth","params":{"symbol":")";
    constexpr std::string_view kSuffix = R"(","limit":100}})";

    std::string request;
    request.reserve(kPrefix.size() + symbolUpper.size() + kSuffix.size());
    request.append(kPrefix);
    request.append(symbolUpper);
    request.append(kSuffix);

    std::cout << "Snapshot request : " << request << std::endl;

    m_ptSnapWSession->write(net::buffer(request));
}

void clsWSBNConnector::ReadSnap()
{
    std::cout << "Reading WS snapshot response ..." << std::endl;
    beast::flat_buffer buffer;
    m_ptSnapWSession->read(buffer);

    std::cout << " ================== SNAPSHOT ==================== " << std::endl;
    std::cout
        << beast::make_printable(buffer.data())
        << std::endl;
    std::cout << " ================= SNAPSHOT END ================= " << std::endl;
}

void clsWSBNConnector::UnSubscribe(std::vector<std::string>& symbols)
{

}
