#include "clsQueue.hpp"

#include <thread>
#include <string>
#include <iostream>

int main()
{
    queue<std::string> q1;

    std::thread t1([&](){

        for(int i = 0 ; i < 100 ; i++)
        {
            std::string val =   std::to_string(i);
            q1.Push(val);
            std::cout << "Pushed : " << val << std::endl;
        }
    });

    std::thread t2([&](){

    for(int i = 0 ; i < 100 ; i++)
    {
        std::string val = q1.Pop();
        std::cout << "Popped : " << val << std::endl;
    }
    });

    t1.join();
    t2.join();

    return 0;
}
