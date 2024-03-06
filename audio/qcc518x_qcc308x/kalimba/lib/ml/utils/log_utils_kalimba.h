/*======================= COPYRIGHT NOTICE ==================================*]
[* Copyright (c) 2019-2020 Qualcomm Technologies, Inc.                       *]
[* All Rights Reserved.                                                      *]
[* Confidential and Proprietary - Qualcomm Technologies, Inc.                *]
[*===========================================================================*/

#ifndef _KALIMBA_LOG_UTILS_H_
#define _KALIMBA_LOG_UTILS_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __WIN32__
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

/**
 * @struct tlog_event
 * @brief structure to hold log info
 */
typedef struct {
    va_list ap;
    const char *fmt;
    const char *file;
    void *udata;
    int line;
    int level;
} tlog_event;

/*! \enum eLevel
    \brief Log level
 */

typedef enum { KLOG_ALL, KLOG_TRACE, KLOG_DEBUG, KLOG_INFO, KLOG_WARN, KLOG_ERROR, KLOG_FATAL, KLOG_NONE } eLevel;

/* In general, enable all logs */
#ifdef ENABLE_LOGS_ALL
#define ENABLE_LOGS_FATAL
#define ENABLE_LOGS_ERROR
#define ENABLE_LOGS_WARNING
#define ENABLE_LOGS_INFO
#define ENABLE_LOGS_DEBUG
#define ENABLE_LOGS_TRACE
#endif /* ENABLE_LOGS */

#ifdef ENABLE_LOGS_FATAL
/*! \def klogf(...)
    \brief A log wrapper to add a fatal info.
 */
#define klogf(...) klog(KLOG_FATAL, __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define klogf(...) (void)0
#endif

#ifdef ENABLE_LOGS_ERROR
/*! \def kloge(...)
    \brief A log wrapper to add an error info.
 */
#define kloge(...) klog(KLOG_ERROR, __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define kloge(...) (void)0
#endif

#ifdef ENABLE_LOGS_WARNING
/*! \def klogw(...)
    \brief A log wrapper to add a warning info.
 */
#define klogw(...) klog(KLOG_WARN, __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define klogw(...) (void)0
#endif

#ifdef ENABLE_LOGS_INFO
/*! \def klogi(...)
    \brief A log wrapper to add a general info.
 */
#define klogi(...) klog(KLOG_INFO, __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define klogi(...) (void)0
#endif

#ifdef ENABLE_LOGS_DEBUG
/*! \def klogd(...)
    \brief A log wrapper to add a debug info.
 */
#define klogd(...) klog(KLOG_DEBUG, __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define klogd(...) (void)0
#endif

#ifdef ENABLE_LOGS_TRACE
/*! \def klogt(...)
    \brief A log wrapper to add a trace info.
 */
#define klogt(...) klog(KLOG_TRACE, __FILENAME__, __LINE__, __VA_ARGS__)
#else
#define klogt(...) (void)0
#endif

#ifdef NO_ADK
/*! \def klogx(...)
    \brief A log wrapper over printf.
 */
#define klogx(format, ...) printf(format, ##__VA_ARGS__)
#else
#define klogx(...) (void)0
#endif

/**
 * @brief Setting log level during runtime
 * @param level (eLevel) log level
 * @return void
 */
void klog_set_level(int level);

/**
 * @brief Pause/Resume logging
 * @param enable (int) 1=pause logging, 0=enable logging
 * @return void
 */
void klog_set_mute(int enable);

/**
 * @brief Main log function
 * @param level (int)
 * @param file (string) file name of source file
 * @param line (int) line number of source file
 * @param fmt (string) log format string
 * @param ... (args) variadic arguments for the specified format
 * @return void
 */

void klog(int level, const char *file, int line, const char *fmt, ...);

#endif /*_KALIMBA_LOG_UTILS_H_ */
