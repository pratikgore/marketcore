#include "clsLogger.hpp"
#include "../common/struct.hpp"

void LogInfo(int thred_id)
{
    eLogLevel currlevel = eLogLevel::INFO;
    Logger::GetInstance().Log(currlevel , "My first logger  from thread." + std::to_string(thred_id));
}

int main()
{
    eLogLevel currlevel = eLogLevel::INFO;
    if (!Logger::GetInstance().InitLogger("./LOG_1.txt", eLogLevel::INFO))
    {
        return 1;
    }

    Logger::GetInstance().Log(currlevel , "My first logger . ");

    std::thread t1(LogInfo , 1);
    std::thread t2(LogInfo , 2);
    std::thread t3(LogInfo , 3);

    t1.join();
    t2.join();
    t3.join();

    Logger::GetInstance().Shutdown();
    
    return 0;

}