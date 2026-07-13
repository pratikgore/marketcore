/*
* File : clsQueue.hpp 
* Description : Header for threadsafe queue, safe for SPSC logging, data ingestion etc.
* Author : Pratik
* Date : 11/4/2026 
*/

#ifndef _CLS_QUEUE_HPP
#define _CLS_QUEUE_HPP

#include <cstdlib>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#include <vector>

template <typename T>
class queue
{
    public:
        queue(size_t capcity = 2);
        ~queue();

        void Push ( T value);
        T Pop ();

        // size_t Size(); 


    private:
        size_t m_capacity      {2};
        size_t m_head         {0};
        size_t m_tail         {0};
        std::mutex          m_lock;
        std::condition_variable m_cv;
        std::vector<T>      m_buffer;
};

#include "../clsQueue.tpp"

#endif //_CLS_QUEUE_HPP