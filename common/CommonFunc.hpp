#include <iostream>
#include <thread>
#include <sched.h>
#include <pthread.h>

bool SetCPUAffinity(std::thread &th, int core)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset); //clear
    CPU_SET( core , &cpuset);  //set new 

    pthread_t handle = th.native_handle();
    int res = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);

    return res == 0;

}

static std::uint64_t NowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}