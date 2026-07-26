#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <thread>
#include <functional>
#include <cstdint>

#include "clsQueue.hpp"
// #include "../Logger/clsLogger.hpp"
#include "../../common/CommonFunc.hpp"
#include "../../common/struct.hpp"

std::atomic<bool> startRun = false;
std::size_t qSize = 128;
std::size_t totalTicks = 1000;
eLogLevel lgLevl = eLogLevel::DEBUG;

//only for consume thread
std::vector<uint64_t>latencies(totalTicks,0);

struct Tick
{
    std::size_t seq{0};
    int bid{0};
    int ask{0};
    int ltp{0};
    std::uint64_t timeStamp{0};

    Tick() = default;

    Tick(std::size_t seqValue, int bidValue, int askValue, int ltpValue, std::uint64_t time)
        : seq(seqValue), bid(bidValue), ask(askValue), ltp(ltpValue), timeStamp(time)
    {
    }
};

void Producer(std::atomic<bool>& producerDone ,  Queue<Tick>& spscQ)
{
    //Forcefully added loop for benchmarking
    while(startRun.load() == false)
        continue;
    
    for (std::size_t i = 1; i <= totalTicks; ++i)
    {   
        auto ts = NowNs();

        Tick t(i, 100 + static_cast<int>(i), 101 + static_cast<int>(i), 100 + static_cast<int>(i), ts);
        while (!spscQ.Enqueue(t))
        {
            std::this_thread::yield();
        }

        std::string lgMsg =  "Produced tick seq=" + std::to_string( t.seq)
                  + " bid="  +  std::to_string(t.bid)
                  + " ask="  + std::to_string(t.ask)
                  + " ltp="  + std::to_string(t.ltp) + "\n";
        
        // Logger::GetInstance().Log(lgLevl , lgMsg);

    }

    producerDone.store(true, std::memory_order_release);
}

void Consumer(std::atomic<bool>& producerDone ,  Queue<Tick>& spscQ)
{
    while(startRun.load() == false)
        continue;

    Tick out;
    std::size_t expected = 1;

    while (!producerDone.load(std::memory_order_acquire) || !spscQ.IsEmpty())
    {
        if (spscQ.Dequeue(out))
        {
            std::string logMsg = "Consumed tick seq= "  + std::to_string(out.seq)
            + " bid=" + std::to_string(out.bid)
            +" ask="  + std::to_string (out.ask)
            +" ltp="  + std::to_string(out.ltp) + "\n";

            // Logger::GetInstance().Log( lgLevl , logMsg);
 

            if (out.seq != expected)
            {
                std::cout << "Tick mismatch expected= " << expected
                            << " got= " << out.seq << std::endl;
                expected = out.seq + 1;
            }
            else
            {
                auto ts = NowNs();
                latencies[expected - 1] = ts - out.timeStamp;
                ++expected;
            }
        }
        else
        {
            std::this_thread::yield();
        }
    }
}
int main()
{
    //Init logger
    // if(!Logger::GetInstance().InitLogger("/home/pgore/workspace/marketcore/Utils/SPSC_Queue.log", lgLevl))
    // {
    //     std::cerr << "Log init failed " << std::endl;
    //     return 0;
    // }

    Queue<Tick> spscQ(qSize);
    std::atomic<bool> producerDone{false};
    bool isAffinitySet {true};

    std::thread t1(Producer, std::ref(producerDone), std::ref(spscQ));
    std::thread t2(Consumer, std::ref(producerDone), std::ref(spscQ));

    isAffinitySet = SetCPUAffinity(t1, 1);
    isAffinitySet = SetCPUAffinity(t2, 2);

    if(!isAffinitySet)
        std::cerr << "FAILED TO SET AFFINITY ... !!!\n";

    //Start time
    std::chrono::steady_clock::time_point t_init = std::chrono::steady_clock::now();

    //After setting affinity start run
    startRun.store(true);

    t1.join();
    t2.join();

    // Logger::GetInstance().Shutdown();
    //End time
    std::chrono::steady_clock::time_point t_end = std::chrono::steady_clock::now();

    //Throughput
    double sec = std::chrono::duration<double>(t_end - t_init).count();
    double msgperSec =  static_cast<double> (totalTicks/sec);

    //Latency
    std::sort(latencies.begin(), latencies.end());
    auto percetile = [&](double p )
    {
        std::size_t indx = static_cast<size_t>(p * totalTicks -1 );
        return latencies[indx];
    };

    std::cout << "Done. Queue size = " << spscQ.Size() << '\n';
    std::cout << "Throughput : Messges per second = : " << msgperSec << "\n";
    //Perfomace of 50% messages
    std::cout << "lat_p50_ns : " << percetile(0.50) << "\n";
    std::cout << "lat_p99_ns  : " << percetile(0.99) << "\n";
    return 0;
}
