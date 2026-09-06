#pragma once

#include "clsFeedCommunicator.hpp"
#include "clsWSBNConnector.hpp"
#include "clsOrderBook.hpp"

class clsDataEngine
{
    public:
        clsDataEngine(clsFeedCommunicator * communicator, clsWSBNConnector * clsWSBNConnector ,  std::atomic<bool>& g_stop_requested_ref);
        ~clsDataEngine();

        void Run();
        void SendDemoRequest();

    private:
        clsFeedCommunicator *m_ptCommunicator;
        clsWSBNConnector   *m_ptWSBNConnector;

        std::atomic<bool>& g_stop_requested; //ref to global signle handler

        std::unordered_map<std::string , clsOrderBook> m_mporderSymToOrderBook;
        std::unordered_map<int, std::string> m_mpSymToRequestId;
};