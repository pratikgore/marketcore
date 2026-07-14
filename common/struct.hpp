#pragma once    

/*
* @brief Defines all common enums and structures used for utilties
*
* @author Pratik Gore
* @date  9-July-2026
*/

//Defines log level used by logger
enum class eLogLevel
{
    DEBUG,
    WARN,
    INFO
};

//Client events
enum class eClientEvent
{
    SUBSCRIBE,
    UNSUBSCRIBE,
    SNAPSHOT
};

enum class eAction
{
    ADD,
    MODIFY,
    DELETE,
    TRADE
};

//Price level for L2 book levels
struct stPriceLevel
{
    double price,
    uint32_t  qut,
};