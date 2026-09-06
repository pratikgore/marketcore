#pragma once

#include <iostream>

/*
 * @brief Tag-prefixed console logging, toggled at compile time via CMake.
 *
 * Usage: MC_LOG("DATAENGINE") << "message" << std::endl;
 *        MC_LOG_ERR("BIANCEFEED") << "error message" << std::endl;
 *
 * Disable per-target via CMake:
 *   target_compile_definitions(<target> PRIVATE MC_LOGGING_ENABLED=0)
 * or globally with the MC_ENABLE_LOGGING option in the top-level CMakeLists.txt.
 * When disabled, the stream expression is compiled but never executed (zero runtime cost).
 */

#ifndef MC_LOGGING_ENABLED
#define MC_LOGGING_ENABLED 1
#endif

#if MC_LOGGING_ENABLED
    #define MC_LOG(tag)     std::cout << "[" tag "] "
    #define MC_LOG_ERR(tag) std::cerr << "[" tag "] "
#else
    #define MC_LOG(tag)     if (true) {} else std::cout
    #define MC_LOG_ERR(tag) if (true) {} else std::cerr
#endif
