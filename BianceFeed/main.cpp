#include <iostream>
#include <thread>
#include <chrono>

#include "../Utils/Logger/clsLogger.hpp"
#include "../Utils/RingBuffer/clsQueue.hpp"
#include "includes/clsRESTBNConnect.hpp"
#include "includes/clsWSBNConnect.hpp"


int main()
{
    eLogLevel setLogLevel = eLogLevel::INFO;
    // if(!Logger::GetInstance().InitLogger("../BianceFeed.log", setLogLevel))
    // {
    //     std::cerr << "Failed launch logger " << std::endl;
    // }

    std::string symbol = "SOLUSDT";
    clsRESTBNConnect objConnect;

    clsWSBNConnect obj;
    obj.HandleSession();

    while(true)
    {
        std::string response = objConnect.FetchOrderBook(symbol);
        std::cout << response << "\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}