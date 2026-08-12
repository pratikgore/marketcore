#include <iostream>
#include <thread>
#include <chrono>

#include "../Utils/Logger/clsLogger.hpp"
#include "../Utils/RingBuffer/clsQueue.hpp"
#include "includes/clsRESTBNConnector.hpp"
#include "includes/clsWSBNConnector.hpp"


int main()
{
    eLogLevel setLogLevel = eLogLevel::INFO;
    // if(!Logger::GetInstance().InitLogger("../BianceFeed.log", setLogLevel))
    // {
    //     std::cerr << "Failed launch logger " << std::endl;
    // }

    std::vector<std::string>sym {"abc"};
    std::string symbol = "SOLUSDT";
    clsRESTBNConnector objConnect;

    clsWSBNConnector obj;
    obj.CreateSession();
    obj.Subscribe(sym);
    obj.ReadFeed();

    while(true)
    {
        std::string response = objConnect.FetchOrderBook(symbol);
        std::cout << response << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}