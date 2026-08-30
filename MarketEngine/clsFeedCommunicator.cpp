
#include <iostream>
#include "clsFeedCommunicator.hpp"

clsFeedCommunicator::clsFeedCommunicator(size_t queue_capacity)
{
    m_QSnapShotData = std::make_unique<DataQueue>(queue_capacity);
    m_QSnpaShotCmd = std::make_unique<CommandQueue>(queue_capacity);
    m_QDepthData = std::make_unique<DataQueue>(queue_capacity);
    m_QDataCmd = std::make_unique<CommandQueue>(queue_capacity);
}

clsFeedCommunicator::~clsFeedCommunicator()
{
    std::cout << "Cleaning Feed communicator" << std::endl;
    m_QSnapShotData.reset();
    m_QSnpaShotCmd.reset();
    m_QDepthData.reset();
    m_QDataCmd.reset();
}

bool clsFeedCommunicator::PushSnapShotCmd(stFeedCommand cmd)
{
    if (!m_QSnpaShotCmd->Enqueue(cmd)) {  // Fixed: removed stray semicolon
        std::cout << "WARN: Snapshot command enqueue failed for symbol: " << cmd.symbol << std::endl;
        return false;
    }
    return true;
}

bool clsFeedCommunicator::PushDepthCmd(stFeedCommand cmd)
{
    if (!m_QDataCmd->Enqueue(cmd)) {  // Fixed: removed stray semicolon
        std::cout << "WARN: depth command enqueue failed for symbol: " << cmd.symbol << std::endl;
        return false;
    }
    return true;
}

bool clsFeedCommunicator::PopSnapShotCmd(stFeedCommand& cmd)
{
    if (!m_QSnpaShotCmd->Dequeue(cmd)) {  // Fixed: removed stray semicolon
        return false;  // Queue empty, no warning needed
    }
    return true;
}

bool clsFeedCommunicator::PopDepthCmd(stFeedCommand& cmd)
{
    if (!m_QDataCmd->Dequeue(cmd)) {  // Fixed: removed stray semicolon
        return false;  // Queue empty, no warning needed
    }
    return true;
}

bool clsFeedCommunicator::PushSnapshotData(stMarketDataMessage data)
{
    if (!m_QSnapShotData->Enqueue(std::move(data))) {
        std::cout << "WARN: Snapshot data queue full, message dropped" << std::endl;
        return false;
    }
    return true;
}

bool clsFeedCommunicator::PushDepthData(stMarketDataMessage data)
{
    if (!m_QDepthData->Enqueue(std::move(data))) {
        std::cout << "WARN: Snapshot data queue full, message dropped" << std::endl;
        return false;
    }
    return true;
}

bool clsFeedCommunicator::PopSnapshotData(stMarketDataMessage& data)
{
    if (!m_QSnapShotData->Dequeue(data)) {
        return false;  // Queue empty
    }
    return true;
}

bool clsFeedCommunicator::PopDepthData(stMarketDataMessage& data)
{
    if (!m_QDepthData->Dequeue(data)) {
        return false;  // Queue empty
    }
    return true;
}
