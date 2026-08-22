/**
 * @file clsRESTBNConnector.hpp
 * @brief Connector for Binance API
 * @author Pratik Gore
 * @date 2026
 */
#pragma once


#include <iostream>
#include <curl/curl.h>
#include <string>

class clsRESTBNConnector
{   
    public:
        clsRESTBNConnector();
        ~clsRESTBNConnector();

        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
        void CreateSession();
        std::string FetchSnapShot(const std::string& symbol, int limit = 20);


    private:
        std::string m_strURL {"https://api.binance.com/api/v3/depth?symbol="};
        std::string m_strReadBuffer{};
        int m_iLimit {20};
        

        CURLcode res{CURLE_OK};
        CURL* m_ptrCurl {nullptr};

};  