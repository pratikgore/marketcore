#include "clsLogger.hpp"
#include <atomic>
#include <iostream>

std::atomic<int> ct = 0;

void LogInfo(int thred_id)
{
    while (ct.load() != 50)
    {
        eLogLevel currlevel = eLogLevel::INFO;
        Logger::GetInstance().Log(currlevel, "My first logger  from thread." + std::to_string(thred_id) + " count " + std::to_string(ct));

        ++ct;
    }
}

int main()
{
    int ct = 0;
    eLogLevel currlevel = eLogLevel::INFO;
    if (!Logger::GetInstance().InitLogger("/home/pgore/workspace/marketcore/Utils/Logger_test.txt", eLogLevel::INFO))
    {
        std::cout << "Failed to launch logger" << std::endl;
        return 1;
    }

    std::cout << "Logger started " << std::endl;
    Logger::GetInstance().Log(currlevel, "My first logger . ");

    std::thread t1(LogInfo, 1);
    std::thread t2(LogInfo, 2);
    std::thread t3(LogInfo, 3);

    t1.join();
    t2.join();
    t3.join();

    Logger::GetInstance().Shutdown();

    return 0;
}