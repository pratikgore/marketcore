//==================== Terminal =========================
g++ -std=c++17 -O2 -pthread main.cpp  -o ringbuffer

//==================== With cmake ========================
Configure
cmake -S . -B build

Build
cmake --build build -j

Run
./build/ring_buffer