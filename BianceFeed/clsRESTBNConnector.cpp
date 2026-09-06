#include "clsRESTBNConnector.hpp"
#include "Logging.hpp"

#define LOG_TAG "BIANCEFEED"

clsRESTBNConnector::clsRESTBNConnector()
{

}

clsRESTBNConnector::~clsRESTBNConnector()
{
    curl_easy_cleanup(m_ptrCurl);
}

//curl writecallback, should be static as per curl 
size_t clsRESTBNConnector::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void clsRESTBNConnector::CreateSession()
{
    m_ptrCurl = curl_easy_init();
    if(!m_ptrCurl) 
    {
        MC_LOG_ERR(LOG_TAG) << "Failed to initialize curl" << std::endl;
        throw std::runtime_error("Failed to initialize curl.");
    }
    curl_easy_setopt(m_ptrCurl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_ptrCurl, CURLOPT_WRITEDATA, &m_strReadBuffer);
    curl_easy_setopt(m_ptrCurl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(m_ptrCurl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_ptrCurl, CURLOPT_SSL_VERIFYHOST, 2L);

}

std::string clsRESTBNConnector::FetchSnapShot(const std::string& symbol, int limit)
{
    m_strReadBuffer.clear();
    
    std::string url = m_strURL + symbol + "&limit=" + std::to_string(m_iLimit);

    curl_easy_setopt(m_ptrCurl, CURLOPT_URL, url.c_str());

    res = curl_easy_perform(m_ptrCurl);

    if (res != CURLE_OK) 
    {
        MC_LOG_ERR(LOG_TAG) << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }
    return m_strReadBuffer;
}