g++ -std=c++17 -O2 -pthread main.cpp clsLogger.cpp -o logger_app


Configure
cmake -S . -B build

Build
cmake --build build -j

Run
./build/ring_buffer
