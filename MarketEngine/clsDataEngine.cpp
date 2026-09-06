#include "clsDataEngine.hpp"
#include "BianceJsonParser.hpp"
#include "struct.hpp"
#include "Logging.hpp"

#define LOG_TAG "DATAENGINE"

static std::uint64_t symCount =  1;
clsDataEngine::clsDataEngine(clsFeedCommunicator * communicator, clsWSBNConnector * WSBNConnector,  
        std::atomic<bool>& g_stop_requested_ref)
:g_stop_requested(g_stop_requested_ref){
    m_ptCommunicator = communicator;
    m_ptWSBNConnector = WSBNConnector;
}

clsDataEngine::~clsDataEngine()
{
}

void clsDataEngine::Run()
{
    MC_LOG(LOG_TAG) << "Started market data run loop" << std::endl;
    while(!g_stop_requested.load(std::memory_order_relaxed))
    {
        stMarketDataMessage msgSnap;

        if (m_ptCommunicator->PopSnapshotData(msgSnap))
        {
            // std::string finalpop = MessageToString(msgSnap);
            // std::cout << finalpop << std::endl;
           

            std::string symbol = m_mpSymToRequestId[msgSnap.requestId];
            msgSnap.symbol = symbol; // snapshot response has no symbol field; resolved via requestId
            // MC_LOG(LOG_TAG) << "Snapshot is added for symbol : " << symbol << std::endl;

            m_mporderSymToOrderBook[symbol].ApplySnapshot(msgSnap);
            std::string finalOrderBook = m_mporderSymToOrderBook[symbol].ToString();
            MC_LOG(LOG_TAG) << "printing final snap: " << finalOrderBook << std::endl;
        }

        stMarketDataMessage msgDepth;
        if(m_ptCommunicator->PopDepthData(msgDepth))
        {
            // MC_LOG(LOG_TAG) << "Incremental is added for symbol " << msgDepth.symbol << std::endl;
            auto it = m_mporderSymToOrderBook.find(msgDepth.symbol);

            if(it != m_mporderSymToOrderBook.end())
            {
                m_mporderSymToOrderBook[msgDepth.symbol].ApplyIncremental(msgDepth);

                std::string finalOrderBook = m_mporderSymToOrderBook[msgDepth.symbol].ToString();
                MC_LOG(LOG_TAG) << finalOrderBook << std::endl;
            }
            else
            {
                MC_LOG_ERR(LOG_TAG) << "Symbol not found skipping update " << msgDepth.symbol << std::endl;
            }
            // std::string finalpop = MessageToString(msgDepth);
            // std::cout << finalpop << std::endl;
        }
    }

    MC_LOG(LOG_TAG) << "Stopped market data run loop" << std::endl;
}

void clsDataEngine::SendDemoRequest()
{

        std::vector<std::string> g_test_symbols = {
        "BTCUSDT",   // Bitcoin
        "ETHUSDT",   // Ethereum
        "BNBUSDT",   // Binance Coin
        "XRPUSDT",   // Ripple
        "ADAUSDT",   // Cardano
        "SOLUSDT",   // Solana
        "DOGEUSDT",  // Dogecoin
        "LINKUSDT",  // Chainlink
        // "LTCUSDT",   // Litecoin
        // "MATICUSDT"  // Polygon
    };

    for(auto sym : g_test_symbols)
    {
        m_mpSymToRequestId[symCount] =  sym;
        stFeedCommand snapCmd{eClientEvent::SNAPSHOT, sym, symCount++};
        stFeedCommand depthCmd{eClientEvent::SUBSCRIBE, sym};
        m_ptCommunicator->PushSnapShotCmd(snapCmd);
        m_ptCommunicator->PushDepthCmd(depthCmd);
        MC_LOG(LOG_TAG) << "Subscribing : " << sym << std::endl;
        // Keep subscribe control traffic under exchange websocket limits.
        std::this_thread::sleep_for(std::chrono::milliseconds(6000));
    }
}