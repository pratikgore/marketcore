# Queue Utilities Context

This file provides quick project context for the two queue implementations under `Utils/`:

- `Utils/RingBuffer`: lock-free single-producer/single-consumer (SPSC) ring buffer.
- `Utils/ThreadSafeQueue`: mutex + condition-variable based blocking queue.

## 1) RingBuffer (`Utils/RingBuffer`)

### Intent
- Fast SPSC queue for low-latency pipelines (for example market tick handoff).
- Non-blocking API: producer/consumer retry when full/empty.

### Key Type
- `Queue<T>` in `clsQueue.hpp`.
- Header-only implementation.

### Core Behavior
- Constructor takes logical `capacity`; internal storage uses `capacity + 1` to disambiguate full vs empty.
- `Enqueue(const T&)` and `Enqueue(T&&)`:
  - Return `false` when full.
  - Return `true` and publish element otherwise.
- `Dequeue(T&)`:
  - Return `false` when empty.
  - Return `true` and move value to output otherwise.
- `IsEmpty()`, `IsFull()`, `Size()` are available for queue state checks.

### Concurrency Model
- Designed for exactly one producer thread and one consumer thread.
- Uses atomic front/back indices with acquire/release memory ordering.
- Wait strategy in sample app is spin/yield (`std::this_thread::yield()`).

### Demo
- `main.cpp` produces 100 `Tick` objects and consumes them in order.
- Verifies sequence ordering and prints final queue size.

### Build
```bash
g++ -std=c++17 -O2 -pthread main.cpp -o ringbuffer
```

## 2) ThreadSafeQueue (`Utils/ThreadSafeQueue`)

### Intent
- Blocking queue using mutex and condition variables.
- Suitable for producer/consumer patterns where waiting is acceptable.

### Key Type
- `queue<T>` declared in `includes/clsQueue.hpp`.
- Template implementation in `clsQueue.tpp` (included by header).

### Core Behavior
- Queue uses a fixed-size circular buffer (`std::vector<T>`).
- Minimum effective capacity is 2.
- `Push(T value)`:
  - Blocks when queue is full.
  - Moves value into buffer and notifies a waiting thread.
- `Pop()`:
  - Blocks when queue is empty.
  - Moves value out, advances head, and notifies a waiting thread.

### Concurrency Model
- Synchronization via `std::mutex` + `std::condition_variable`.
- Current implementation is safe for SPSC usage demonstrated in `main.cpp`.
- Because one mutex protects both ends, this is simpler but may have lower throughput than lock-free SPSC at high contention.

### Demo
- `main.cpp` launches one producer and one consumer thread.
- Producer pushes strings `0..99`; consumer pops and prints `0..99`.

### Build
```bash
g++ -std=c++17 -Iincludes -pthread main.cpp -o thread_safe_queue
```

## 3) Logger (`Utils/Logger`)

### Intent
- Provide a single process-level logging utility with a simple API for components.
- Keep file I/O off producer call paths by using an internal queue + consumer thread.

### Key Type
- `Logger` in `clsLogger.hpp`.
- Singleton access via `Logger::GetInstance()`.

### Current Public API
- `InitLogger(filePath, level, enableTimeStamp, queueSize)`:
  - Opens log file in append mode.
  - Creates internal queue and starts consumer thread.
- `Log(level, message)`:
  - Applies log-level filtering.
  - Formats line with optional timestamp.
  - Enqueues formatted line.
- `Shutdown()`:
  - Stops consumer thread.
  - Flushes and closes file.

### Internal Design (Current)
- One queue instance (`Queue<std::string>`) inside logger.
- One consumer thread (`ConsumeLoop`) dequeues lines and writes to file.
- Producer side is non-blocking in API shape: enqueue returns `false` when queue is full.

### Usage Example
- `main.cpp` initializes logger, spawns worker threads that call `Log`, joins workers, then calls `Shutdown`.
- Correct lifecycle ordering is important: join producer threads before `Shutdown`.

### Build
From `Utils/Logger`:
```bash
g++ -std=c++17 -O2 -pthread main.cpp clsLogger.cpp -o logger_app
```

From workspace root:
```bash
g++ -std=c++17 -O2 -pthread Utils/Logger/main.cpp Utils/Logger/clsLogger.cpp -o Utils/Logger/logger_app
```

### Important Concurrency Note
- The current ring buffer type (`Queue<T>`) is SPSC by design.
- Current logger shares one queue across potentially multiple producer threads.
- This works in light tests but is not a strict many-producer-safe design under heavy concurrency.
- Planned direction for robust multi-thread logging: per-thread SPSC channels drained by one consumer thread.

## 4) Practical Differences

- RingBuffer:
  - Lock-free, non-blocking API.
  - Caller handles retries on full/empty.
  - Better fit for low-latency SPSC pipelines.
- ThreadSafeQueue:
  - Blocking API with condition variables.
  - Simpler usage for workflows that naturally wait.
  - General synchronization overhead from locking.

## 5) Notes

- Naming differs intentionally in current code: `Queue<T>` (RingBuffer) vs `queue<T>` (ThreadSafeQueue).
- Both sample programs currently demonstrate one producer and one consumer thread.
- Logger now exists under `Utils/Logger` and currently uses a singleton with background consumer thread.