#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdint>
#include <sched.h>
#include <pthread.h>

/**
 * @brief Set CPU affinity for a thread
 * @param th The thread to bind
 * @param core The CPU core number (0-indexed)
 * @return true if successful
 */
inline bool SetCPUAffinity(std::thread& th, int core)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);  // Clear
    CPU_SET(core, &cpuset);  // Set new core

    pthread_t handle = th.native_handle();
    int res = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);

    return res == 0;
}

/**
 * @brief Get current time in nanoseconds since epoch
 * @return Nanoseconds since steady_clock epoch
 */
inline std::uint64_t NowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}