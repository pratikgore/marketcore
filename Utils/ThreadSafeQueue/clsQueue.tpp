template <typename T>
queue<T>::queue(size_t capacity)
{
    if(capacity < 2)
        capacity = 2;
    
    m_buffer.resize(capacity);
    m_head = 0;
    m_tail = 0;
    m_capacity = capacity;
}

template <typename T>
queue<T>::~queue()
{
}

/*
* Push waits if m_buffer is full
*/
template <typename T>
void queue<T>::Push(T value)
{
    std::unique_lock<std::mutex> lock(m_lock);
    //Wait until we get some space in queue
    m_cv.wait(lock, [this](){return (m_tail + 1) % m_capacity != m_head; });

    m_buffer[m_tail] = std::move(value);
    m_tail = (m_tail + 1) % m_capacity;

    lock.unlock();
    m_cv.notify_one();
}

/*
* Pop waits if m_buffer is empty
*/
template <typename T>
T queue<T>::Pop()
{
    std::unique_lock<std::mutex> lock(m_lock);
    //wait until there is element in m_buffer
    m_cv.wait(lock, [this](){ return m_head != m_tail; });

    T value = std::move(m_buffer[m_head]);
    m_head = (m_head + 1) % m_capacity;

    lock.unlock();
    m_cv.notify_one();

    return value;
}