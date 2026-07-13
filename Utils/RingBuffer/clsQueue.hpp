/*
* @brief Header for lock free single producer single consumer queue. Header only class
*
* @author Pratik Gore pratik.dev@gmail.com
* @date 8-July-2026
*/

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

template<typename T>
class Queue
{
public:
    explicit Queue(std::size_t capacity)
        : m_capacity(capacity + 1), m_data(std::make_unique<T[]>(m_capacity))
    {
        if (capacity == 0)
        {
            throw std::invalid_argument("Queue capacity must be > 0");
        }
    }

    ~Queue() = default;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue& operator=(Queue&&) = delete;

    std::size_t Size() const
    {
        const std::size_t currentFront = m_front.load(std::memory_order_relaxed);
        const std::size_t currentBack = m_back.load(std::memory_order_relaxed);

        if (currentBack >= currentFront)
        {
            return currentBack - currentFront;
        }
        return (m_capacity - currentFront) + currentBack;
    }

    bool IsEmpty() const
    {
        return m_front.load(std::memory_order_relaxed) == m_back.load(std::memory_order_relaxed);
    }

    bool IsFull() const
    {
        const std::size_t currentFront = m_front.load(std::memory_order_acquire);
        const std::size_t currentBack = m_back.load(std::memory_order_relaxed);
        const std::size_t nextBack = (currentBack + 1) % m_capacity;
        return nextBack == currentFront;
    }

    bool Enqueue(const T& element)
    {
        const std::size_t currentBack = m_back.load(std::memory_order_relaxed);
        const std::size_t nextBack = (currentBack + 1) % m_capacity;

        if (nextBack == m_front.load(std::memory_order_acquire))
        {
            return false;
        }

        m_data[currentBack] = element;
        m_back.store(nextBack, std::memory_order_release);
        return true;
    }

    bool Enqueue(T&& element)
    {
        const std::size_t currentBack = m_back.load(std::memory_order_relaxed);
        const std::size_t nextBack = (currentBack + 1) % m_capacity;

        if (nextBack == m_front.load(std::memory_order_acquire))
        {
            return false;
        }

        m_data[currentBack] = std::move(element);
        m_back.store(nextBack, std::memory_order_release);
        return true;
    }

    bool Dequeue(T& outElement)
    {
        const std::size_t currentFront = m_front.load(std::memory_order_relaxed);

        if (currentFront == m_back.load(std::memory_order_acquire))
        {
            return false;
        }

        outElement = std::move(m_data[currentFront]);
        const std::size_t nextFront = (currentFront + 1) % m_capacity;
        m_front.store(nextFront, std::memory_order_release);
        return true;
    }

private:
    const std::size_t m_capacity;
    std::unique_ptr<T[]> m_data;
    std::atomic<std::size_t> m_front{0};
    std::atomic<std::size_t> m_back{0};
};