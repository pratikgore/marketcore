#include <iostream>

#include "clsFeedCommunicator.hpp"
#include "clsWSBNConnector.hpp"
#include "clsDataEngine.hpp"
#include "Logging.hpp"

#define LOG_TAG "DATAENGINE"

std::atomic<bool> g_stop_requested{false};

void HandleSignal(int)
{
    g_stop_requested.store(true, std::memory_order_relaxed);
    MC_LOG(LOG_TAG) << "STOPPING MARKET FEED ...." << std::endl;
}

int main()
{
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    clsFeedCommunicator *feedCommObj = new clsFeedCommunicator;
    clsWSBNConnector *bianceConnectorObj = new clsWSBNConnector(feedCommObj);
    bianceConnectorObj->Init();
    
    clsDataEngine *subMgrObj = new clsDataEngine(feedCommObj, bianceConnectorObj, g_stop_requested);
    // subMgrObj->SendDemoRequest();
    std::thread t1(&clsDataEngine::SendDemoRequest, subMgrObj);
    subMgrObj->Run();
    t1.join();

};