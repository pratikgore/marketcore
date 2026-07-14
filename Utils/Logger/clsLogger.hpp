#pragma once 

/**
 * @brief Singleton class provides single logger instance at a time
 * 
 * @author Pratik Gore
 */

#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>

#include "../RingBuffer/clsQueue.hpp"
#include "../../common/struct.hpp"

class Logger
{
    public:
        ~Logger();

        bool InitLogger(const std::string& filePath, eLogLevel level = eLogLevel::DEBUG, bool enableTimeStamp = true, std::size_t queueSize = 1024);
        static Logger& GetInstance();
        bool Log(eLogLevel level, const std::string& log);
        void Shutdown();

        //delete copy and move sematics
        Logger(const Logger &) = delete;
        Logger& operator=(const Logger& ) = delete;
        Logger( Logger&&) = delete;
        Logger& operator=(Logger&& ) = delete;

    private:
        Logger() = default;
        void ConsumeLoop();
        bool EnqueueFormatted(const std::string& line);
        std::string FormatLine(eLogLevel level, const std::string& log) const;
        std::string BuildPrefix(const std::string& level) const;

        static Logger  m_pLogInstance ;

        eLogLevel   m_iLogLevel{eLogLevel::DEBUG};
        bool        m_bEnableTimeStamp {false};
        std::ofstream m_logFile;
        std::mutex m_logLock;
        std::atomic<bool> m_isInitialized{false};
        std::atomic<bool> m_stopConsumer{false};
        std::unique_ptr<Queue<std::string>> m_logQueue;
        std::thread m_consumerThread;

};