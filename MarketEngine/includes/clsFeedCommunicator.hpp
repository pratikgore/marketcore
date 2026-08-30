#pragma once 

//ownership :  connector beteen Feed <--> Marketengine 

#include <memory>
#include <cstdint>
#include "struct.hpp"
#include "clsQueue.hpp"

/**
 * @brief Feed Communicator - facade for snapshot SPSC queues
 * 
 * Owns 2 SPSC queues:
 * - SnapshotCommandQ: Engine -> Snapshot Connector
 * - SnapshotDataQ: Snapshot Connector -> Engine
 */
class clsFeedCommunicator
{
public:
    explicit clsFeedCommunicator(size_t queue_capacity = 5000);
    ~clsFeedCommunicator();

    // Commands: Engine -> Snapshot Connector
    bool PushSnapShotCmd(stFeedCommand cmd);
    bool PushDepthCmd(stFeedCommand cmd);

    bool PopSnapShotCmd(stFeedCommand& cmd);
    bool PopDepthCmd(stFeedCommand& cmd);


    // Data: Snapshot Connector -> Engine
    bool PushSnapshotData(stMarketDataMessage data);
    bool PushDepthData(stMarketDataMessage data);

    bool PopSnapshotData(stMarketDataMessage& data);
    bool PopDepthData(stMarketDataMessage& data);

private:
    // Queue types
    using DataQueue = Queue<stMarketDataMessage>;
    using CommandQueue = Queue<stFeedCommand>;

    // Snapshot queues
    std::unique_ptr<CommandQueue> m_QSnpaShotCmd;    // Engine -> Connector
    std::unique_ptr<DataQueue> m_QSnapShotData;      // Connector -> Engine

    //Data queues
    std::unique_ptr<CommandQueue> m_QDataCmd;    // Engine -> Connector
    std::unique_ptr<DataQueue> m_QDepthData;      // Connector -> Engine
};