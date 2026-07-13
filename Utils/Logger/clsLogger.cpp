#include "clsLogger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

Logger Logger::m_pLogInstance{};

Logger::~Logger()
{
    Shutdown();
}

bool Logger::InitLogger(const std::string& filePath, eLogLevel level, bool enableTimeStamp, std::size_t queueSize)
{
    std::lock_guard<std::mutex> lock(m_logLock);

    if (m_isInitialized.load(std::memory_order_acquire))
    {
        return true;
    }

    if (queueSize == 0)
    {
        return false;
    }

    m_logQueue = std::make_unique<Queue<std::string>>(queueSize);
    m_logFile.open(filePath, std::ios::out | std::ios::app);

    if (!m_logFile.is_open())
    {
        m_logQueue.reset();
        return false;
    }

    m_iLogLevel = level;
    m_bEnableTimeStamp = enableTimeStamp;
    m_stopConsumer.store(false, std::memory_order_release);
    m_consumerThread = std::thread(&Logger::ConsumeLoop, this);
    m_isInitialized.store(true, std::memory_order_release);
    return true;
}

Logger& Logger::GetInstance()
{
    return m_pLogInstance;
}

bool Logger::Log(eLogLevel level, const std::string& log)
{
    if (!m_isInitialized.load(std::memory_order_acquire))
    {
        return false;
    }

    if (level < m_iLogLevel)
    {
        return true;
    }

    return EnqueueFormatted(FormatLine(level, log));
}

std::string Logger::BuildPrefix(const std::string& level) const
{
    if (!m_bEnableTimeStamp)
    {
        return "[" + level + "] ";
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
    localtime_r(&nowTime, &localTime);

    std::ostringstream stream;
    stream << '[' << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] "
           << '[' << level << "] ";
    return stream.str();
}

std::string Logger::FormatLine(eLogLevel level, const std::string& log) const
{
    switch (level)
    {
        case eLogLevel::INFO:
            return BuildPrefix("INFO") + log;
        case eLogLevel::WARN:
            return BuildPrefix("WARN") + log;
        case eLogLevel::DEBUG:
        default:
            return BuildPrefix("DEBUG") + log;
    }
}

bool Logger::EnqueueFormatted(const std::string& line)
{
    if (!m_logQueue)
    {
        return false;
    }

    return m_logQueue->Enqueue(line);
}

void Logger::ConsumeLoop()
{
    std::string line;

    while (!m_stopConsumer.load(std::memory_order_acquire) ||
           (m_logQueue && !m_logQueue->IsEmpty()))
    {
        if (m_logQueue && m_logQueue->Dequeue(line))
        {
            std::lock_guard<std::mutex> lock(m_logLock);
            if (m_logFile.is_open())
            {
                m_logFile << line << '\n';
            }
        }
        else
        {
            std::this_thread::yield();
        }
    }

    std::lock_guard<std::mutex> lock(m_logLock);
    if (m_logFile.is_open())
    {
        m_logFile.flush();
    }
}

void Logger::Shutdown()
{
    if (!m_isInitialized.load(std::memory_order_acquire))
    {
        return;
    }

    m_stopConsumer.store(true, std::memory_order_release);
    if (m_consumerThread.joinable())
    {
        m_consumerThread.join();
    }

    std::lock_guard<std::mutex> lock(m_logLock);
    if (m_logFile.is_open())
    {
        m_logFile.flush();
        m_logFile.close();
    }

    m_logQueue.reset();
    m_isInitialized.store(false, std::memory_order_release);
}