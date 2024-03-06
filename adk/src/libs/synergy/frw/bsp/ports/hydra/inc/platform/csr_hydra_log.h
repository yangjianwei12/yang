#ifndef CSR_HYDRA_LOG__
#define CSR_HYDRA_LOG__

/*****************************************************************************
Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #60 $
*****************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_LEVEL_CRITICAL (0)
#define LOG_LEVEL_ERROR    (1)
#define LOG_LEVEL_WARN     (2)
#define LOG_LEVEL_INFO     (3)
#define LOG_LEVEL_VERBOSE  (4)
#define LOG_LEVEL_TASK     (5)

typedef enum
{
    LOG_CRITICAL = LOG_LEVEL_CRITICAL,
    LOG_ERROR = LOG_LEVEL_ERROR,
    LOG_WARN = LOG_LEVEL_WARN,
    LOG_INFO = LOG_LEVEL_INFO,
    LOG_VERBOSE = LOG_LEVEL_VERBOSE,
    LOG_TASK = LOG_LEVEL_TASK
} synergy_debug_log_level_t;

#if defined(CSR_LOG_ENABLE) && defined(CSR_TARGET_PRODUCT_VM)
#ifdef BUILD_LOG_LEVEL
#define SYNERGY_DEFAULT_LOG_LEVEL BUILD_LOG_LEVEL
#else /*!BUILD_LOG_LEVEL*/
#define SYNERGY_DEFAULT_LOG_LEVEL LOG_INFO
#endif /*BUILD_LOG_LEVEL*/

#if SYNERGY_DEFAULT_LOG_LEVEL < LOG_LEVEL_VERBOSE
#define CSR_LOG_LEVEL_TEXT_DEBUG_DISABLE
#endif

/* SYNERGY_LOGGING_EXCLUDE_SEQUENCE: This should be in-line with LOGGING_EXCLUDE_SEQUENCE */
#ifndef SYNERGY_LOGGING_EXCLUDE_SEQUENCE

#define SYNERGY_EXTRA_LOGGING_STRING        "%04X: "
#define SYNERGY_EXTRA_LOGGING_NUM_PARAMS    1
#define SYNERGY_EXTRA_LOGGING_PARAMS        , globalDebugLineCount++

extern uint16 globalDebugLineCount;

#else

#define SYNERGY_EXTRA_LOGGING_STRING
#define SYNERGY_EXTRA_LOGGING_NUM_PARAMS
#define SYNERGY_EXTRA_LOGGING_PARAMS

#endif /*  LOGGING_EXCLUDE_SEQUENCE */

#define SYNERGY_FMT(fmt,...) fmt

#define SYNERGY_REST_OF_ARGS_NONE(fmt)
#define SYNERGY_REST_OF_ARGS_SOME(fmt,...)  , __VA_ARGS__

#define _SYNERGY_MAKE_REST_OF_ARGS(a,b)     a##b
#define SYNERGY_MAKE_REST_OF_ARGS(_num)     _SYNERGY_MAKE_REST_OF_ARGS(SYNERGY_REST_OF_ARGS, _num)

#define SYNERGY_VA_NARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, N, ...)       N
#define SYNERGY_VA_NARGS(...)               SYNERGY_VA_NARGS_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7,     \
                                                                  6, 5, 4, 3, 2, 1, 0, _bonus_as_no_ellipsis)

#define SYNERGY_VA_ANY_ARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, SN, ...)   SN
#define SYNERGY_VA_ANY_ARGS(...)            SYNERGY_VA_ANY_ARGS_IMPL(__VA_ARGS__, _SOME, _SOME, _SOME, _SOME, _SOME,    \
                                                                     _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME,   \
                                                                     _SOME, _SOME, _SOME, _SOME, _NONE, _bonus_as_no_ellipsis)

#ifndef HAVE_COMBINED_APPS_IMAGE
#define SYNERGY_HYDRA_LOG_STRING_ATTR       _Pragma("datasection DBG_STRING")
#else
#define SYNERGY_HYDRA_LOG_STRING_ATTR __attribute__((section("DBG_STRING")))
#endif

#define SYNERGY_HYDRA_LOG_STRING(label, text)  \
    SYNERGY_HYDRA_LOG_STRING_ATTR static const char label[] = text

extern synergy_debug_log_level_t debug_log_level_synergy;
extern void hydra_log_firm_variadic(const char *event_key, size_t n_args, ...);
extern void SynergyLogPrint(int level, void* handle, const char *fmt, int nargs, ...);

#ifdef SYNERGY_LOG_FUNC_ENABLED
#define SYNERGY_DEBUG_LOG(level, handle, subOrigin, ...) \
    do{\
          SYNERGY_HYDRA_LOG_STRING(log_fmt, #level " (" #handle "): " SYNERGY_FMT(__VA_ARGS__,bonus_arg)); \
          SynergyLogPrint(level, handle, log_fmt, SYNERGY_VA_NARGS(__VA_ARGS__)\
                          SYNERGY_MAKE_REST_OF_ARGS(SYNERGY_VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__));\
} while(0)

#define SYNERGY_DEBUG_LOG_GENERIC(level, ...) \
    do { \
        if (debug_log_level_synergy >= level) \
        { \
            SYNERGY_HYDRA_LOG_STRING(log_fmt, #level ": " SYNERGY_FMT(__VA_ARGS__,bonus_arg)); \
            SynergyLogPrint(level, NULL, log_fmt, SYNERGY_VA_NARGS(__VA_ARGS__) \
                            SYNERGY_MAKE_REST_OF_ARGS(SYNERGY_VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__));\
        } \
    } while(0)
#else /* !SYNERGY_LOG_FUNC_ENABLED */

#define SYNERGY_DEBUG_LOG(level, handle, subOrigin, ...) \
    do {\
        synergy_debug_log_level_t modlevel = debug_log_level_synergy; \
        if(handle != 0) modlevel = ((CsrLogTextHandle*)handle)->logLevel; \
        if (debug_log_level_synergy >= level && modlevel >= level)\
        { \
            SYNERGY_HYDRA_LOG_STRING(log_fmt, #level " (" #handle "): " SYNERGY_FMT(__VA_ARGS__,bonus_arg)); \
            hydra_log_firm_variadic(log_fmt, SYNERGY_VA_NARGS(__VA_ARGS__) \
                                    SYNERGY_MAKE_REST_OF_ARGS(SYNERGY_VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__)); \
        } \
    } while(0)

#define SYNERGY_DEBUG_LOG_GENERIC(level, ...) \
    do { \
        if (debug_log_level_synergy >= level) \
        { \
            SYNERGY_HYDRA_LOG_STRING(log_fmt, #level ": " SYNERGY_FMT(__VA_ARGS__,bonus_arg)); \
            hydra_log_firm_variadic(log_fmt, SYNERGY_VA_NARGS(__VA_ARGS__) \
                                    SYNERGY_MAKE_REST_OF_ARGS(SYNERGY_VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__)); \
        } \
    } while(0)
#endif /* SYNERGY_LOG_FUNC_ENABLED */

#else /* defined(CSR_LOG_ENABLE) && defined(CSR_TARGET_PRODUCT_VM) */

#define SYNERGY_DEBUG_LOG(...)
#define SYNERGY_HYDRA_LOG_STRING(...)

#endif /* defined(CSR_LOG_ENABLE) && defined(CSR_TARGET_PRODUCT_VM) */

#ifdef __cplusplus
}
#endif

#endif /* CSR_HYDRA_LOG__ */

