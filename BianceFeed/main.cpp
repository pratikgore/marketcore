#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <csignal>

#include "clsLogger.hpp"
#include "clsQueue.hpp"
#include "clsRESTBNConnector.hpp"
#include "clsWSBNConnector.hpp"
#include "clsFeedCommunicator.hpp"
#include "BianceJsonParser.hpp"

std::string symbol = "BTCUSDT";

std::atomic<bool> g_stop_requested{false};

void HandleSignal(int)
{
    g_stop_requested.store(true, std::memory_order_relaxed);
}

// Global test symbols - can be used across multiple functions
// Biance depth stream has limit of 5 symbol/sec
std::vector<std::string> g_test_symbols = {
    "BTCUSDT",   // Bitcoin
    "ETHUSDT",   // Ethereum
    "BNBUSDT",   // Binance Coin
    "XRPUSDT",   // Ripple
    "ADAUSDT",   // Cardano
    // "SOLUSDT",   // Solana
    // "DOGEUSDT",  // Dogecoin
    // "LINKUSDT",  // Chainlink
    // "LTCUSDT",   // Litecoin
    // "MATICUSDT"  // Polygon
};

//Single symbol websocket test snapshot
void TestWS_1()
{
    clsFeedCommunicator *feedCommObj = new clsFeedCommunicator;
    clsWSBNConnector obj(feedCommObj);
    obj.Init();
    
    // Send snapshot command
    stFeedCommand st{eClientEvent::SNAPSHOT, symbol};
    feedCommObj->PushSnapShotCmd(st);

    // Pop the snapshot data
    stMarketDataMessage msg;
    while(!g_stop_requested.load(std::memory_order_relaxed))
    {
        if(feedCommObj->PopSnapshotData(msg))
        {
            std::string finalpop = MessageToString(msg);
            std::cout << "POP : " << finalpop << "\n";
        } 
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // Optional will never hit
    obj.Stop();
    
    delete feedCommObj;
}

//Single symbol websocket test depth
void TestWS_2()
{
    clsFeedCommunicator *feedCommObj = new clsFeedCommunicator;
    clsWSBNConnector obj(feedCommObj);
    obj.Init();
    
    // Send snapshot command
    stFeedCommand st{eClientEvent::SUBSCRIBE, symbol};
    feedCommObj->PushDepthCmd(st);

    // Pop the snapshot data
    stMarketDataMessage msg;
    while(!g_stop_requested.load(std::memory_order_relaxed))
    {
        if(feedCommObj->PopDepthData(msg))
        {
            std::string finalpop = MessageToString(msg);
            std::cout << "POP : " << finalpop << "\n";
        } 
        else
        {
            // std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    // Optional will never hit
    obj.Stop();
    
    delete feedCommObj;
}

//REST api test
void RunREST()
{
    
    clsRESTBNConnector objConnect;
    objConnect.CreateSession();
    
    while(!g_stop_requested.load(std::memory_order_relaxed))
    {
        std::string response = objConnect.FetchSnapShot(symbol);

        std::cout << "================ PRITING SNAPSHOT ================ " << std::endl;
        std::cout << response << "\n";
        std::cout << " =============== SNAPSHOT END ==================== " << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

//Muti symbol websockert test snapshot 
void SimulateWS_1()
{

    clsFeedCommunicator *feedCommObj = new clsFeedCommunicator;
    clsWSBNConnector connector(feedCommObj);
    connector.Init();
    
    std::cout << "\n";
    std::cout << " =================================================\n";
    std::cout << "  REGRESSION TEST: WebSocket Snapshot Flow        \n";
    std::cout << " =================================================\n";

    auto subscribe = [&]()
    {
        stFeedCommand cmd ;
        cmd.event = eClientEvent::SNAPSHOT;

        for(auto x : g_test_symbols)
        {
            cmd.symbol = x;
            feedCommObj->PushSnapShotCmd(cmd);
            
            std::cout << "Subscribing : " << cmd.symbol << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    };

    auto fethSnapData = [&]()
    {
        //Practically should run infinitely, but for test purose we will close after 10 seconds 

        while(!g_stop_requested.load(std::memory_order_relaxed))
        {
            // std::this_thread::sleep_for(std::chrono::seconds(10));
            stMarketDataMessage msg;
            if(feedCommObj->PopSnapshotData(msg))
            {
                std::string finalpop = MessageToString(msg);
                std::cout << "POP : " << finalpop << "\n";
            } 
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
    };

    std::thread t1(subscribe);
    std::thread t2(fethSnapData);

    t1.join();
    t2.join();
    // Cleanup
    connector.Stop();
}

//Muti symbol websockert test depth 
void SimulateWS_2()
{

    clsFeedCommunicator *feedCommObj = new clsFeedCommunicator;
    clsWSBNConnector connector(feedCommObj);
    connector.Init();
    
    std::cout << "\n";
    std::cout << " =================================================\n";
    std::cout << "  REGRESSION TEST: WebSocket Depth Flow        \n";
    std::cout << " =================================================\n";

    auto subscribe = [&]()
    {
        stFeedCommand cmd ;
        cmd.event = eClientEvent::SUBSCRIBE;

        for(auto x : g_test_symbols)
        {
            cmd.symbol = x;
            feedCommObj->PushDepthCmd(cmd);
            
            std::cout << "Subscribing : " << cmd.symbol << std::endl;
            // Keep subscribe control traffic under exchange websocket limits.
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    };

    auto fethSnapData = [&]()
    {
        //Practically should run infinitely, but for test purose we will close after 10 seconds 

        while(!g_stop_requested.load(std::memory_order_relaxed))
        {
            // std::this_thread::sleep_for(std::chrono::seconds(10));
            stMarketDataMessage msg;
            if(feedCommObj->PopDepthData(msg))
            {
                std::string finalpop = MessageToString(msg);

                std::cout << "POP : " << finalpop << "\n";
            } 
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }
        }
    };

    std::thread t1(subscribe);
    std::thread t2(fethSnapData);

    t1.join();
    t2.join();
    // Cleanup
    connector.Stop();
}
int main()
{
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    eLogLevel setLogLevel = eLogLevel::INFO;
    // if(!Logger::GetInstance().InitLogger("../BianceFeed.log", setLogLevel))
    // {
    //     std::cerr << "Failed launch logger " << std::endl;
    // }

    // ============= Run mutiple symbol snapshot test =========== 
    std::thread t1(SimulateWS_1);

    // ============= Run mutiple symbol depth test =========== 
    std::thread t2(SimulateWS_2);

    // ============= Run single symbol snapshot test ============
    // TestWS_1();

    //============= Run single symbol depth test ============
    // TestWS_2();


    //Run REST api test
    // std::thread t2 (RunREST);

    t1.join();
    t2.join();


    return 0;
}