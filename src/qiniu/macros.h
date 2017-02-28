#ifndef __QN_MACROS_H__
#define __QN_MACROS_H__

#if defined(_MSC_VER)

    #if defined(QN_CC_STATIC_LIBRARY)
        #define QN_SDK
    #else
        #if defined(QN_CC_COMPILING)
            #define QN_SDK __declspec(dllexport)
        #else
            #define QN_SDK __declspec(dllimport)
        #endif
    #endif

    #define restrict __restrict

#else

    #define QN_SDK

#endif

#endif // __QN_MACROS_H__

