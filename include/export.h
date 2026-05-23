#pragma once

// If we're building on Windows
#if defined(_WIN32)
    #ifdef BUILDING_ANTWORLD_DLL
        #define ANTWORLD_EXPORT __declspec(dllexport)
    #elif defined(LINKING_ANTWORLD_DLL)
        #define ANTWORLD_EXPORT __declspec(dllimport)
    #else
        #define ANTWORLD_EXPORT
    #endif
#else
    #define ANTWORLD_EXPORT
#endif