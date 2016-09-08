#ifndef __QN_MACROS_H__
#define __QN_MACROS_H__

#if defined(_MSC_VER)

    #if defined(QN_CC_LIBRARY)
        #define QN_API
    #else
        #if defined(QN_CC_COMPILING)
            #define QN_API __declspec(dllexport)
        #else
            #define QN_API __declspec(dllimport)
        #endif
    #endif

    #define restrict __restrict

#else

    #define QN_API

#endif

#endif // __QN_MACROS_H__

