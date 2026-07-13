#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>

#include "clsQueue.hpp"

struct Tick
{
    std::size_t seq{0};
    int bid{0};
    int ask{0};
    int ltp{0};

    Tick() = default;

    Tick(std::size_t seqValue, int bidValue, int askValue, int ltpValue)
        : seq(seqValue), bid(bidValue), ask(askValue), ltp(ltpValue)
    {
    }
};

int main()
{
    constexpr std::size_t qSize = 64;
    constexpr std::size_t totalTicks = 100;

    Queue<Tick> spscQ(qSize);
    std::atomic<bool> producerDone{false};

    std::thread producer([&]() {
        for (std::size_t i = 1; i <= totalTicks; ++i)
        {
            Tick t(i, 100 + static_cast<int>(i), 101 + static_cast<int>(i), 100 + static_cast<int>(i));
            while (!spscQ.Enqueue(t))
            {
                std::this_thread::yield();
            }

            std::cout << "Produced tick seq=" << t.seq
                      << " bid=" << t.bid
                      << " ask=" << t.ask
                      << " ltp=" << t.ltp << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        producerDone.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        Tick out;
        std::size_t expected = 1;

        while (!producerDone.load(std::memory_order_acquire) || !spscQ.IsEmpty())
        {
            if (spscQ.Dequeue(out))
            {
                std::cout << "Consumed tick seq= " << out.seq
                          << " bid=" << out.bid
                          << " ask=" << out.ask
                          << " ltp=" << out.ltp << std::endl;

                if (out.seq != expected)
                {
                    std::cout << "Order mismatch expected=" << expected
                              << " got=" << out.seq << std::endl;
                    expected = out.seq + 1;
                }
                else
                {
                    ++expected;
                }
            }
            else
            {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    std::cout << "Done. Queue size=" << spscQ.Size() << '\n';
    return 0;
}
