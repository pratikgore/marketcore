/**
 * @file clsRESTBNConnect.hpp
 * @brief Connector for Binance API
 * @author Pratik Gore
 * @date 2024
 */
#pragma once


#include <iostream>
#include <curl/curl.h>
#include <string>

class clsRESTBNConnect
{   
    public:
        clsRESTBNConnect();
        ~clsRESTBNConnect();

        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
        std::string FetchOrderBook(const std::string& symbol, int limit = 20);


    private:
};  