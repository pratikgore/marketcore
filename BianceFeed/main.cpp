#include <iostream>
#include <thread>
#include <chrono>

#include "clsLogger.hpp"
#include "clsQueue.hpp"
#include "clsRESTBNConnector.hpp"
#include "clsWSBNConnector.hpp"

std::string symbol = "BTCUSDT";

void RunWS()
{
    clsWSBNConnector obj;
    obj.CreateSessionSnap();
    obj.SubscribeSnap(symbol);
    obj.ReadSnap();

    obj.CreateSessionDepth();
    obj.SubscribeDepth(symbol);
    obj.ReadFeed();

}

void RunREST()
{
    
    clsRESTBNConnector objConnect;
    objConnect.CreateSession();
    
    while(true)
    {
        std::string response = objConnect.FetchSnapShot(symbol);

        std::cout << "================ PRITING SNAPSHOT ================ " << std::endl;
        std::cout << response << "\n";
        std::cout << " =============== SNAPSHOT END ==================== " << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main()
{
    eLogLevel setLogLevel = eLogLevel::INFO;
    // if(!Logger::GetInstance().InitLogger("../BianceFeed.log", setLogLevel))
    // {
    //     std::cerr << "Failed launch logger " << std::endl;
    // }

    std::thread t1(RunWS);
    // std::thread t2 (RunREST);

    t1.join();
    // t2.join();

    return 0;
}