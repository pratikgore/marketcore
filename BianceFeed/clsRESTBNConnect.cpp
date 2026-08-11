#include "includes/clsRESTBNConnect.hpp"

clsRESTBNConnect::clsRESTBNConnect()
{

}

clsRESTBNConnect::~clsRESTBNConnect()
{

}

//curl writecallback, should be static as per curl 
size_t clsRESTBNConnect::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string clsRESTBNConnect::FetchOrderBook(const std::string& symbol, int limit)
{
    CURL* curl = curl_easy_init();
    if(!curl) 
    {
        std::cerr << "Failed to initialize curl" << std::endl;
        throw std::runtime_error("Failed to initialize curl.");
    }

    CURLcode res;
    std::string readBuffer;
    std::string url = "https://api.binance.com/api/v3/depth?symbol=" + symbol + "&limit=" + std::to_string(limit);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) 
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }

    curl_easy_cleanup(curl);

    return readBuffer;
}