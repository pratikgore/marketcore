#include <iostream>

#include "../Utils/Logger/clsLogger.hpp"
#include "../Utils/RingBuffer/clsQueue.hpp"

int main()
{
    eLogLevel setLogLevel = eLogLevel::INFO;
    if(!Logger::GetInstance().InitLogger("../Logs/marketsim.log", setLogLevel))
    {
        std::cerr << "Failed launch logger " << std::endl;
    }

    return 0;
}