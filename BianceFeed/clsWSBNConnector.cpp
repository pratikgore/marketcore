
#include "clsWSBNConnector.hpp"
#include "ErrorCode.hpp"
#include "clsFeedCommunicator.hpp"
#include "BianceJsonParser.hpp"
#include "Logging.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>

#define LOG_TAG "BIANCEFEED"

clsWSBNConnector::clsWSBNConnector(clsFeedCommunicator* communicator) 
{
    m_communicator = communicator;
}
clsWSBNConnector::~clsWSBNConnector()
{
    Stop();
}

void clsWSBNConnector::Init()
{
    CreateSessionSnap(); 
    CreateSessionDepth();
    m_running = true;
    m_stopped = false;
    
    // Start reader thread (continuous WebSocket reads)
    m_SnapReaderThread = std::thread(&clsWSBNConnector::SnapshotReaderThread, this);
    m_DepthReaderThread = std::thread(&clsWSBNConnector::DepthReaderThread, this);;
    
    // Start subscription thread (command handling)
    m_SnapThread = std::thread(&clsWSBNConnector::SnapshotThreadLoop, this);
    m_DepthThread = std::thread(&clsWSBNConnector::DepthThreadLoop, this);
}

void clsWSBNConnector::Stop()
{
    if (m_stopped.exchange(true))
    {
        return;
    }

    m_running = false;

    // Join command thread first to prevent concurrent writes during close.
    if (m_SnapThread.joinable())
    {
        m_SnapThread.join();
    }

    if (m_DepthThread.joinable())
    {
        m_DepthThread.join();
    }

    // Cancel/close transport so blocking reads unwind without cross-thread websocket::close.
    if (m_ptSnapWSession)
    {
        beast::error_code ec;
        beast::get_lowest_layer(*m_ptSnapWSession).cancel(ec);
        beast::get_lowest_layer(*m_ptSnapWSession).close(ec);
    }

    if (m_ptWSession)
    {
        beast::error_code ec;
        beast::get_lowest_layer(*m_ptWSession).cancel(ec);
        beast::get_lowest_layer(*m_ptWSession).close(ec);
    }

    if (m_SnapReaderThread.joinable())
    {
        m_SnapReaderThread.join();
    }

    if (m_DepthReaderThread.joinable())
    {
        m_DepthReaderThread.join();
    }
}

long clsWSBNConnector::CreateSessionDepth()
{
    MC_LOG(LOG_TAG) <<"Biance WS API session init .." << std::endl;

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
    MC_LOG(LOG_TAG) << "Connected to Binance\n";
    return SUCCESS;
}

void clsWSBNConnector::ReadFeed(beast::flat_buffer& buffer)
{
    // std::cout << "Reading WS feed ..." <<std::endl;
    // Receive messages forever
    try
    {
        if (!m_ptWSession)
        {
            MC_LOG_ERR(LOG_TAG) << "Error: WebSocket session is null!" << std::endl;
            return;
        }
    
        m_ptWSession->read(buffer);

        // std::cout << " ================== INCR UPDATE ================= " << std::endl;
        // std::cout
        //     << beast::make_printable(buffer.data())
        //     << std::endl;
        // std::cout << " ================= UPDATE END =================== " << std::endl;
         
    }
    catch (const std::exception& e)
    {
        // Socket cancel/close during shutdown is expected.
        if (m_running)
        {
            MC_LOG_ERR(LOG_TAG) << "Error reading depth: " << e.what() << std::endl;
        }
    }
}

void clsWSBNConnector::SubscribeDepth(std::string & symbols)
{
    // Build only the dynamic stream name and reuse fixed JSON fragments.
    constexpr std::string_view kPrefix = R"({"method":"SUBSCRIBE","params":[")";
    constexpr std::string_view kMid = R"(@depth"],"id":)";

    std::string streamName = symbols;
    std::transform(streamName.begin(), streamName.end(), streamName.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    const auto requestId = m_nextRequestId.fetch_add(1, std::memory_order_relaxed);
    std::string request;
    request.reserve(kPrefix.size() + streamName.size() + kMid.size() + 24);
    request.append(kPrefix);
    request.append(streamName);
    request.append(kMid);
    request.append(std::to_string(requestId));
    request.push_back('}');

    if (!m_ptWSession)
    {
        MC_LOG_ERR(LOG_TAG) << "Error: Depth WebSocket session is null!" << std::endl;
        return;
    }

    try
    {
        m_ptWSession->write(net::buffer(request));
    }
    catch (const std::exception& e)
    {
        MC_LOG_ERR(LOG_TAG) << "Error writing depth subscribe request: " << e.what() << std::endl;
        m_running = false;
        return;
    }

    MC_LOG(LOG_TAG) << " [Depth ] Shared request : " << request << std::endl;

}

long clsWSBNConnector::CreateSessionSnap()
{
    MC_LOG(LOG_TAG) << "Biance WS API snapshot session init .." << std::endl;

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

    MC_LOG(LOG_TAG) << "Connected to Binance snapshot endpoint\n";
    return SUCCESS;
}

void clsWSBNConnector::SubscribeSnap(std::string & symbols, std::uint64_t requestId)
{
    std::string symbolUpper = symbols;
    std::transform(symbolUpper.begin(), symbolUpper.end(), symbolUpper.begin(),
        [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

    constexpr std::string_view kPrefix = R"({"id":)";
    constexpr std::string_view kMid1 = R"(,"method":"depth","params":{"symbol":")";
    constexpr std::string_view kSuffix = R"(","limit":100}})";

    std::string request;
    request.reserve(kPrefix.size() + kMid1.size() + symbolUpper.size() + kSuffix.size() + 24);
    request.append(kPrefix);
    request.append(std::to_string(requestId));
    request.append(kMid1);
    request.append(symbolUpper);
    request.append(kSuffix);

    MC_LOG(LOG_TAG) << "Snapshot request : " << request << std::endl;

    if (!m_ptSnapWSession)
    {
        MC_LOG_ERR(LOG_TAG) << "Error: WebSocket session is null!" << std::endl;
        return;
    }
    try
    {
        m_ptSnapWSession->write(net::buffer(request));
    }
    catch (const std::exception& e)
    {
        MC_LOG_ERR(LOG_TAG) << "Error writing snapshot request: " << e.what() << std::endl;
        m_running = false;
        return;
    }

    MC_LOG(LOG_TAG) << "Request sent " << std::endl;
}

void clsWSBNConnector::ReadSnap(beast::flat_buffer& buffer)
{
    try
    {
        if (!m_ptSnapWSession)
        {
            MC_LOG_ERR(LOG_TAG) << "Error: WebSocket session is null!" << std::endl;
            return;
        }

        // Read WebSocket message (blocking)
        m_ptSnapWSession->read(buffer);

        // std::cout << " ================== SNAPSHOT ==================== " << std::endl;
        // std::cout << beast::make_printable(buffer.data()) << std::endl;
        // std::cout << " ================= SNAPSHOT END ================= " << std::endl;
    }
    catch (const std::exception& e)
    {
        // Socket cancel/close during shutdown is expected.
        if (m_running)
        {
            MC_LOG_ERR(LOG_TAG) << "Error reading snapshot: " << e.what() << std::endl;
        }
    }
}

void clsWSBNConnector::UnSubscribe(std::vector<std::string>& symbols)
{

}

void clsWSBNConnector::SnapshotReaderThread()
{
    MC_LOG(LOG_TAG) << "SnapshotReaderThread started - continuously reading..." << std::endl;
    
    while(m_running)
    {
        try
        {
            beast::flat_buffer buffer;
            ReadSnap(buffer);

            if (buffer.size() > 0)
            {    
                std::string json_str = beast::buffers_to_string(buffer.data());

                MC_LOG(LOG_TAG) << json_str << std::endl;
                stMarketDataMessage msg = ParseSnapshotJson(json_str);

                if (msg.parse_success)
                {
                    if(m_communicator->PushSnapshotData(msg))
                        MC_LOG(LOG_TAG) << "[Reader] Snapshot data enqueued for " << msg.symbol << std::endl;
                    else
                        MC_LOG(LOG_TAG) << " [Reader] Failed to enqueue snapshot data" << msg.symbol << std::endl;
                }
                else
                {
                    MC_LOG(LOG_TAG) << " [Reader] Snapshot parse failed: " << msg.parse_error_msg << std::endl;
                }
               
            }
        }
        catch (const std::exception& e)
        {
            MC_LOG_ERR(LOG_TAG) << "SnapshotReaderThread error: " << e.what() << std::endl;
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    MC_LOG(LOG_TAG) << "SnapshotReaderThread stopped" << std::endl;
}

void clsWSBNConnector::DepthReaderThread()
{
    MC_LOG(LOG_TAG) << "DepthReaderThread started - continuously reading..." << std::endl;
    
    while(m_running)
    {
        try
        {
            beast::flat_buffer buffer;
            ReadFeed(buffer);

            if (buffer.size() > 0)
            {    
                std::string json_str = beast::buffers_to_string(buffer.data());
                stMarketDataMessage msg = ParseDepthUpdateJson(json_str);

                if (msg.parse_success)
                {
                    if(m_communicator->PushDepthData(msg))
                        MC_LOG(LOG_TAG) << "[Reader] Depth data enqueued for symbol " << msg.symbol << std::endl;
                    else
                        MC_LOG(LOG_TAG) << " [Reader] Failed to enqueue depth data" << msg.symbol << std::endl;
                }
                else
                {
                    if (msg.parse_error_msg != "CONTROL_ACK")
                    {
                        MC_LOG(LOG_TAG) << " [Reader] depth parse failed: " << msg.parse_error_msg << std::endl;
                    }
                }
               
            }
        }
        catch (const std::exception& e)
        {
            MC_LOG_ERR(LOG_TAG) << "DepthReaderThread error: " << e.what() << std::endl;
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    MC_LOG(LOG_TAG) << "DepthReaderThread stopped" << std::endl;
}

void clsWSBNConnector::SnapshotThreadLoop()
{
    while(m_running)
    {
        stFeedCommand cmd;
        
        // Check for new subscription commands (non-blocking)
        if(m_communicator->PopSnapShotCmd(cmd))
        {
            MC_LOG(LOG_TAG) << "[Command] Subscription command received for symbol: " << cmd.symbol << std::endl;
            
            switch(cmd.event)
            {
                case eClientEvent::SNAPSHOT:
                    SubscribeSnap(cmd.symbol, cmd.requestId);
                    break;
                default:
                    MC_LOG(LOG_TAG) << "unhandled event" << std::endl;
            }
        }
        else
        {
            // No command, sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}

void clsWSBNConnector::DepthThreadLoop()
{
    while(m_running)
    {
        stFeedCommand cmd;
        
        // Check for new subscription commands (non-blocking)
        if(m_communicator->PopDepthCmd(cmd))
        {
            MC_LOG(LOG_TAG) << "[Command] Subscription command received for symbol: " << cmd.symbol << std::endl;
            
            switch(cmd.event)
            {
                case eClientEvent::SUBSCRIBE:
                    SubscribeDepth(cmd.symbol);
                    break;
                default:
                    MC_LOG(LOG_TAG) << "unhandled event" << std::endl;
            }
        }
        else
        {
            // No command, sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
}