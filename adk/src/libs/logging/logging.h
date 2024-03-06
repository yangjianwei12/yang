/*!
\copyright  Copyright (c) 2008 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Header file for logging macro's

    Two styles of logging are supported.

    Both styles can be accessed from the pydbg tool using the
    log commands. The two forms of logging will be displayed
    in the order they were logged. Although the two styles of logging
    can be combined, the recommendation is to use the DEBUG_LOG()
    functions.

    It is recommended to use the following macros for logging so that the
    amount of information displayed can be changed at run-time:
        DEBUG_LOG_ERROR(...)
        DEBUG_LOG_WARN(...)
        DEBUG_LOG_INFO(...)
        DEBUG_LOG_DEBUG(...)
        DEBUG_LOG_VERBOSE(...)
        DEBUG_LOG_V_VERBOSE(...)
    By default the amount of information logged at run time is controlled by
    the global variable \c debug_log_level__global. It is initialised at boot
    to \c DEFAULT_LOG_LEVEL which is normally \c DEBUG_LOG_LEVEL_INFO so any
    INFO, WARNING and ERROR messages will be displayed. That can be changed
    within this file.

    Debug included in this way can be viewed using the pydbg debugging tool,
    and also from within the Qualcomm Multicore Debug Environment. These tools
    can help decode some of the values in log messages to show the NAME of a
    value rather than the ID. This is enabled for enumerated types and, as
    a special case, message IDs.

    In case of a Panic it is recommended to use the following macro to display a useful message
    about it.
        DEBUG_LOG_PANIC(...)

    \par Decoding enumerated types.
    \parblock
    Use the name of the enumerated type in the debug in the format
    enum:<enumeration_name>:%d (or 0x%x)

    e.g. Using an enum in debug

        DEBUG_LOG("appLinkPolicyHandleRole peer enum:hci_status:%d enum:hci_role:%d", status, role);
    \endparblock
    
    \par Making types available for debug
    \parblock
    The identifiers for the enumerated type will normally be available for decoding.
    If the type is never used directly as a variable, the information may not be 
    saved. This can be done using the macros PRESERVE_ENUM_FOR_DEBUGGING() and 
    PRESERVE_TYPE_FOR_DEBUGGING().

    \note These macros cannot be used in a header file. This causes the link stage 
    of the build to report multiple definitions. The recommendation is to add these
    to the main file of the module the message type is identified with.

    e.g. Preserving types

        PRESERVE_ENUM_FOR_DEBUGGING(message_groups)
        PRESERVE_TYPE_FOR_DEBUGGING(message_base_t)
    \endparblock

    \par Using message identifiers in debug
    \parblock
    Use the tag MESSAGE. If messages can come from an internal message type,
    include that also

    Messages identifiers can only be decoded in this way if the message enumeration
    has been recorded as a message during compilation. Do this using the two macros
    LOGGING_PRESERVE_MESSAGE_TYPE and LOGGING_PRESERVE_MESSAGE_ENUM. These macros
    need to be placed in a C source file rather than a header file. This is because
    they define a variable to preserve the type information. The linker will remove
    the variable, but would also report a duplicate symbol if defined in multiple places.

    e.g. For global messages only

        DEBUG_LOG_V_VERBOSE("appHandleClMessage called, MESSAGE:0x%x", id);

    e.g. For a handler with both global and local messages

        DEBUG_LOG("peer_find_role_handler. Unhandled message MESSAGE:peer_find_role_internal_message_t:0x%x", id);
    \endparblock

    \par Setting log level for sets of files
    \parblock
    If required, the level can
    be controlled per module by defining the macro as DEBUG_LOG_MODULE_NAME
    just before the include of <logging.h> and adding a line
    "DEBUG_LOG_DEFINE_LEVEL_VAR" after completion of all the header files
    in the C file.
    \note due to the nature of header file(s) inclusion strict recommendation
    is to define DEBUG_LOG_MODULE_NAME macro before including any header files.

    E.g. for main.c this would look something like:

        #define DEBUG_LOG_MODULE_NAME main
        \#include <boot.h>
        \#include <os.h>
        \#include <logging.h>
        \#include <app/message/system_message.h>

        DEBUG_LOG_DEFINE_LEVEL_VAR

        appTaskData globalApp;

    That would create a global variable \c debug_log_level_main which can
    be set at run-time to control the level of messages output by the
    statements in this file.

    Some modules may want to include the \c DEBUG_LOG_MODULE_NAME <module_name>
    via a module's private header.
    In that case they would have define of the module name in the
    private header just before the include of \c logging.h and have the
    \c DEBUG_LOG_DEFINE_LEVEL_VAR statement in one of the C files of the module.

    \note as mentioned above due to the nature of header file(s) inclusion
    strict recommendation is to include module's private header file before
    including any header file(s).
    \note Make sure \c DEBUG_LOG_DEFINE_LEVEL_VAR statement won't be included
    in multiple C files of the module. Compilation will fail otherwise.
    Like multiple definition of \c debug_log_level_<module_name> .

    \note To see the example how to use it, please check \c ui_log_level.h

    E.g. for UI module this would look something like:

    ui_indicator_log_level.h
        #define DEBUG_LOG_MODULE_NAME ui
        \#include <logging.h>

    ui.c
        \#include "ui_indicator_log_level.h"
        \#include "ui.h"
        \#include "ui_inputs.h"
        \#include "ui_indicator_prompts.h"
        \#include "ui_indicator_tones.h"
        \#include "ui_indicator_leds.h"
        \#include "adk_log.h"

        \#include <stdlib.h>
        \#include <stdio.h>
        \#include <logging.h>
        \#include <panic.h>
        \#include <task_list.h>

        DEBUG_LOG_DEFINE_LEVEL_VAR

    ui_indicator_prompts.c
        \#include "ui_indicator_log_level.h"
        ....

    ui_indicator_tones.c
        \#include "ui_indicator_log_level.h"
        ....

    \note The global variable defined \c debug_log_level_<module_name>
    to hold the log level for respective module(s) may get removed by linker
    if respective module(s) is not used by application.
    Trying to access and modify such global variable \c debug_log_level_<module_name>
    from pydbg will result in error.

    \endparblock

    \par Disabling logging
    \parblock
    All of these macros are enabled by default, but logging can be disabled
    by use of the define DISABLE_LOG. Defining or undefining this
    at the top of a particular source file would allow logging to be completely
    disabled at compile-time on a module basis.

    The per-module level logging can be disabled by defining
    \c DISABLE_PER_MODULE_LOG_LEVELS which will save the data memory
    consumed by the global variables for each module.

    The log levels can be disabled globally by defining
    \c DISABLE_DEBUG_LOG_LEVELS which will cause all the debug statements
    of \c DEFAULT_LOG_LEVEL (normally \c DEBUG_LOG_LEVEL_INFO) and more
    important to always produce output and the less important levels to be
    ignored at build time. This will save some code size from the
    comparison with the log level on each debug statement and from removing
    the less important levels.
    \endparblock

    \par Implementation notes
    \parblock
    \note The DEBUG_LOG() macros write condensed information
    to a logging area and <b>can only be decoded if the original
    application image file (.elf) is available</b>.

    This macro is quicker since the format string isn't parsed by the
    firmware and by virtue of being condensed there
    is less chance of losing information if monitoring information
    in real time.

    The DEBUG_PRINT() macros write the complete string to a different
    logging area, a character buffer, and can be decoded even if the
    application image file is not available.

    The use of DEBUG_PRINT() is not recommended apart from, for instance,
    printing the contents of a message or buffer. Due to memory
    constraints the available printf buffer is relatively small
    and information can be lost. When paramaters are used the DEBUG_PRINT()
    functions can also use a lot of memory on the software stack which can cause
    a Panic() due to a stack overflow.

    Five functions has been defined, by DEBUG_LOG_FUNC_FOR_LEVEL macro, one per log level.
    Having a function per log level is more efficient in terms of program size
    than calling single function inside if condition which was done before.

    Note that DEBUG_LOG_WITH_LEVEL has been removed,
    thus macros with the fixed log levels, like DEBUG_LOG_VERBOSE, must used everywhere.

    \endparblock
*/

#ifndef LOGGING_H
#define LOGGING_H

#include <hydra_log.h>
#include <panic.h>
#include <csrtypes.h>
#include <stdarg.h>

#if defined(DESKTOP_BUILD) || !defined(INSTALL_HYDRA_LOG)
#include <stdio.h>
#endif

/*! From ADK22.2 size of the logging sequence counter has changed from 16 to 8 bit.
  Reason for that change is conserving program space.
  Default is to use 8 bit which match P0 implementation in ADK22.2.
  In case backward compatibility (use of 16 bit counter) is required then LOGGING_INCLUDE_SEQUENCE should be defined.
*/
#ifdef LOGGING_INCLUDE_SEQUENCE
#define EXTRA_LOGGING_STRING        "%04X: "
#define EXTRA_LOGGING_NUM_PARAMS    1
#define EXTRA_LOGGING_PARAMS        , globalDebugLineCount++
#else
#define EXTRA_LOGGING_STRING
#define EXTRA_LOGGING_NUM_PARAMS 0
#define EXTRA_LOGGING_PARAMS
#endif /*  LOGGING_INCLUDE_SEQUENCE */


/*! \cond internals

    This is some cunning macro magic that is able to count the
    number of arguments supplied to DEBUG_LOG

    If the arguments were "Hello", 123, 456 then VA_NARGS returns 2

                Hello 123 456 16  15  14  13  12  11  10   9   8   7   6   5   4   3  2  1 0 _bonus
    VA_NARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, N, ...) N


 */
#define VA_NARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, _bonus_as_no_ellipsis)

    /* Equivalent macro that just indicates if there are SOME arguments
       after the format string, or NONE

       if the argument is just "hello" then VA_ANY_ARGS_IMPL returns _NONE, which is
       used to form a macro REST_OF_ARGS_NONE

                      hello _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _S, _NONE, _bonus
       VA_ANY_ARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, SN, ...) SN

       */

#define VA_ANY_ARGS_IMPL(_a, _b, _c, _d, _e, _f, _g, _h, _i, _j, _k, _l, _m, _n, _o, _p, _q, SN, ...) SN
#define VA_ANY_ARGS(...) VA_ANY_ARGS_IMPL(__VA_ARGS__, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _SOME, _NONE, _bonus_as_no_ellipsis)

    /* Retrieve just the format string */
#define FMT(fmt,...) fmt

    /* macros that return additional parameters past the format string
        The _NONE variant is needed as (fmt, ...) will not match if just passed "hello"
     */
#define REST_OF_ARGS_NONE(fmt)
#define REST_OF_ARGS_SOME(fmt,...) ,__VA_ARGS__
#define _MAKE_REST_OF_ARGS(a,b) a##b
#define MAKE_REST_OF_ARGS(_num) _MAKE_REST_OF_ARGS(REST_OF_ARGS,_num)


extern void hydra_log_firm_variadic(const char *event_key, size_t n_args, ...);
/*! \endcond internals */

extern uint16 globalDebugLineCount;

typedef enum {
    DEBUG_LOG_LEVEL_ERROR,
    DEBUG_LOG_LEVEL_WARN,
    DEBUG_LOG_LEVEL_INFO,
    DEBUG_LOG_LEVEL_DEBUG,
    DEBUG_LOG_LEVEL_VERBOSE,
    DEBUG_LOG_LEVEL_V_VERBOSE,
} debug_log_level_t;

#define DEBUG_LOG_LEVEL_ERROR       DEBUG_LOG_LEVEL_ERROR
#define DEBUG_LOG_LEVEL_WARN        DEBUG_LOG_LEVEL_WARN
#define DEBUG_LOG_LEVEL_INFO        DEBUG_LOG_LEVEL_INFO
#define DEBUG_LOG_LEVEL_DEBUG       DEBUG_LOG_LEVEL_DEBUG
#define DEBUG_LOG_LEVEL_VERBOSE     DEBUG_LOG_LEVEL_VERBOSE
#define DEBUG_LOG_LEVEL_V_VERBOSE   DEBUG_LOG_LEVEL_V_VERBOSE

#define DEBUG_LOG_ERROR(...)        _DEBUG_LOG_L(DEBUG_LOG_LEVEL_ERROR,     __VA_ARGS__)
#define DEBUG_LOG_WARN(...)         _DEBUG_LOG_L(DEBUG_LOG_LEVEL_WARN,      __VA_ARGS__)
#define DEBUG_LOG_INFO(...)         _DEBUG_LOG_L(DEBUG_LOG_LEVEL_INFO,      __VA_ARGS__)
#define DEBUG_LOG_DEBUG(...)        _DEBUG_LOG_L(DEBUG_LOG_LEVEL_DEBUG,     __VA_ARGS__)
#define DEBUG_LOG_VERBOSE(...)      _DEBUG_LOG_L(DEBUG_LOG_LEVEL_VERBOSE,   __VA_ARGS__)
#define DEBUG_LOG_V_VERBOSE(...)    _DEBUG_LOG_L(DEBUG_LOG_LEVEL_V_VERBOSE, __VA_ARGS__)
#define DEBUG_LOG   DEBUG_LOG_VERBOSE
#define DEBUG_LOG_ALWAYS DEBUG_LOG_ERROR
#define DEBUG_LOG_STATE DEBUG_LOG_INFO
#define DEBUG_LOG_FN_ENTRY DEBUG_LOG_DEBUG
#define DEBUG_LOG_PANIC(...) do{DEBUG_LOG_ERROR(__VA_ARGS__); Panic();}while(0)

extern void debugLogData(const uint8 *data, uint16 data_size);

#define DEBUG_LOG_DATA_ERROR(...)        _DEBUG_LOG_DATA_L(DEBUG_LOG_LEVEL_ERROR,     __VA_ARGS__)
#define DEBUG_LOG_DATA_WARN(...)         _DEBUG_LOG_DATA_L(DEBUG_LOG_LEVEL_WARN,      __VA_ARGS__)
#define DEBUG_LOG_DATA_INFO(...)         _DEBUG_LOG_DATA_L(DEBUG_LOG_LEVEL_INFO,      __VA_ARGS__)
#define DEBUG_LOG_DATA_DEBUG(...)        _DEBUG_LOG_DATA_L(DEBUG_LOG_LEVEL_DEBUG,     __VA_ARGS__)
#define DEBUG_LOG_DATA_VERBOSE(...)      _DEBUG_LOG_DATA_L(DEBUG_LOG_LEVEL_VERBOSE,   __VA_ARGS__)
#define DEBUG_LOG_DATA_V_VERBOSE(...)    _DEBUG_LOG_DATA_L(DEBUG_LOG_LEVEL_V_VERBOSE, __VA_ARGS__)
#define DEBUG_LOG_DATA   DEBUG_LOG_DATA_VERBOSE
#define DEBUG_LOG_DATA_ALWAYS DEBUG_LOG_DATA_ERROR
#define DEBUG_LOG_DATA_PANIC(...) do{DEBUG_LOG_DATA_ERROR(__VA_ARGS__); Panic();}while(0)

/*! \brief Callback type for DEBUG_LOG_XXX type of functions.

    This callback will be called for every instance of DEBUG_LOG_XXX
    regardless of the currently selected logging level.
    This callback is called before normal debug log handling,
    which allows for suppressing debug log output on individual basis.

    Note that callbacks are only implemented when the debug log levels are enabled
    i.e. when DISABLE_DEBUG_LOG_LEVELS is not defined.

    \param current_log_level_symbol Pointer to a variable containing currently selected log level.
                                    That is the log level below which the logs are ignored.
    \param log_level                Log level used a particular log the callback is called for.
    \param event_key                By default it is address of debug text in the elf file.
                                    If the configuration would be changed then it would be address of debug text on chip.
    \param n_args                   Number of arguments following the debug text.
    \param va_argp                  A list of arguments.

    \return                         Callback should return TRUE if normal debug log handling is desired,
                                    or return FALSE if normal debug log should be suppressed.

*/
typedef bool (*debug_log_callback_t)(debug_log_level_t *current_log_level_symbol, debug_log_level_t log_level, const char *event_key, size_t n_args, va_list va_argp);

/*! \brief Callback type for DEBUG_LOG_DATA_XXX type of functions.

    See debug_log_callback_t description.

    \param current_log_level_symbol Pointer to a variable containing currently selected log level.
                                    That is the log level below which the logs are ignored.
    \param log_level                Log level used a particular log the callback is called for.
    \param data                     Pointer to byte array of data to be logged.
    \param data_size                Size of the data to be logged.

    \return                         Callback should return TRUE if normal debug log handling is desired,
                                    or return FALSE if normal debug log should be suppressed.

*/
typedef bool (*debug_log_data_callback_t)(debug_log_level_t *current_log_level_symbol, debug_log_level_t log_level, const uint8 *data, uint16 data_size);

/* \brief A type containing all the debug log callback variants. */
typedef struct
{
    debug_log_callback_t LogCallback;
    debug_log_data_callback_t LogDataCallback;
} debug_log_callbacks_t;

/*  \brief Registers the debug log callbacks.

    Note that only one set of callback is supported at one time.

    \param callbacks    Pointer to a structure containing the debug log callbacks.
                        Individual callbacks that are not needed should be set to NULL.
                        Setting callbacks to NULL is deregistration.

    \return             Previously registered set of callbacks or NULL if there weren't any.
*/
const debug_log_callbacks_t *DebugLog_RegisterCallbacks(const debug_log_callbacks_t *callbacks);

/*! \brief Internal function. DO NOT USE.

    Implementation of debug_log_callback_t.
    It is exposed only because it needs to be included in the macros in this file.
*/
bool DebugLog_Callback(debug_log_level_t *current_log_level_symbol, debug_log_level_t log_level, const char *event_key, size_t n_args, va_list va_argp);

/*! \brief Internal function. DO NOT USE.

    Implementation of debug_log_data_callback_t.
    It is exposed only because it needs to be included in the macros in this file.
*/
bool DebugLog_DataCallback(debug_log_level_t *current_log_level_symbol, debug_log_level_t log_level, const uint8 *data, uint16 data_size);

/*! Level at which logging defaults on boot. All messages with
 * this level or higher (numerically lower) will appear in the
 * log. The value can be changed on-the-fly by changing either
 * the global log level \c debug_log_level__global or a module
 * level variable where it exists (see below).
 */
#define DEFAULT_LOG_LEVEL           DEBUG_LOG_LEVEL_INFO

/*! It is used instead of DEFAULT_LOG_LEVEL when DISABLE_DEBUG_LOG_LEVELS
 * is defined. A side-effect of defining DISABLE_DEBUG_LOG_LEVELS is that
 * all logs below DEFAULT_LOG_FOR_DISABLED_LEVELS are removed from the build.
 * Reason for DEFAULT_LOG_FOR_DISABLED_LEVELS is allow for setting that level
 * independently from DEFAULT_LOG_LEVEL.
 */
#define DEFAULT_LOG_FOR_DISABLED_LEVELS DEBUG_LOG_LEVEL_VERBOSE

#define UNUSED0(a)                      (void)(a)
#define UNUSED1(a,b)                    (void)(a),UNUSED0(b)
#define UNUSED2(a,b,c)                  (void)(a),UNUSED1(b,c)
#define UNUSED3(a,b,c,d)                (void)(a),UNUSED2(b,c,d)
#define UNUSED4(a,b,c,d,e)              (void)(a),UNUSED3(b,c,d,e)
#define UNUSED5(a,b,c,d,e,f)            (void)(a),UNUSED4(b,c,d,e,f)
#define UNUSED6(a,b,c,d,e,f,g)          (void)(a),UNUSED5(b,c,d,e,f,g)
#define UNUSED7(a,b,c,d,e,f,g,h)        (void)(a),UNUSED6(b,c,d,e,f,g,h)
#define UNUSED8(a,b,c,d,e,f,g,h,i)      (void)(a),UNUSED7(b,c,d,e,f,g,h,i)
#define UNUSED9(a,b,c,d,e,f,g,h,i,j)    (void)(a),UNUSED8(b,c,d,e,f,g,h,i,j)
#define UNUSED10(a,b,c,d,e,f,g,h,i,j,k) (void)(a),UNUSED9(b,c,d,e,f,g,h,i,j,k)

#define ALL_UNUSED_IMPL_(nargs) UNUSED ## nargs
#define ALL_UNUSED_IMPL(nargs) ALL_UNUSED_IMPL_(nargs)

#define DEBUG_LOG_NOT_USED_WITH_LEVEL(level, ...) ALL_UNUSED_IMPL(VA_NARGS(__VA_ARGS__)) (__VA_ARGS__)
#define DEBUG_LOG_NOT_USED(...) DEBUG_LOG_NOT_USED_WITH_LEVEL(DEFAULT_LOG_LEVEL, __VA_ARGS__)


/*! \brief Preserve message enums for debug
    These macros allow preservation of message enumerations for use with the 
    MESSAGE: form of DEBUG_LOG output.

    Named enumerations are preserved using LOGGING_PRESERVE_MESSAGE_ENUM()
    Anonymous enumerations defined as a typedef are preserved using 
    LOGGING_PRESERVE_MESSAGE_TYPE()
*/
#if defined(PRESERVE_TYPE_IN_SECTION)
#define LOGGING_PRESERVE_MESSAGE_TYPE(_type) PRESERVE_TYPE_IN_SECTION(_type, MSG_ENUMS)
#define LOGGING_PRESERVE_MESSAGE_ENUM(_type) PRESERVE_ENUM_IN_SECTION(_type, MSG_ENUMS)

#elif defined(PRESERVE_TYPE_FOR_DEBUGGING)
    /* Macros to preserve types in a specific section are a late addition.
       Use if available otherwise save the type using an earlier macro. These
       will be available to the debug tool, but won't be decoded */
#define LOGGING_PRESERVE_MESSAGE_TYPE(_type) PRESERVE_TYPE_FOR_DEBUGGING(_type)
#define LOGGING_PRESERVE_MESSAGE_ENUM(_type) PRESERVE_ENUM_FOR_DEBUGGING(_type)
#else
#define LOGGING_PRESERVE_MESSAGE_TYPE(_type)
#define LOGGING_PRESERVE_MESSAGE_ENUM(_type)
#endif

#ifndef DISABLE_LOG

#if !defined(DESKTOP_BUILD) && defined(INSTALL_HYDRA_LOG)

/* ! Function definition for a specific log level.
 * For this function the condition checking the log level
 * is moved into the function itself, to conserve the program space.
 */

#define DEBUG_LOG_FOR_LEVEL_(_symbol,_level) _DEBUG_LOG_##_symbol##_##_level
#define DEBUG_LOG_FOR_LEVEL(_symbol,_level) DEBUG_LOG_FOR_LEVEL_(_symbol,_level)

#define DEBUG_LOG_FUNC_FOR_LEVEL(_SYM,level)\
    void DEBUG_LOG_FOR_LEVEL(_SYM,level)(const char *fmt,int nargs,...)\
    {\
        va_list vargs;\
        va_start(vargs,nargs);\
        bool do_log = DebugLog_Callback(&LOG_LEVEL_CURRENT_SYMBOL, level, fmt, nargs, vargs);\
        if(level<=LOG_LEVEL_CURRENT_SYMBOL && do_log)\
        { \
            hydra_log_firm_va_arg(fmt, nargs, vargs);\
        }\
        va_end(vargs);\
    }


#if defined(DEBUG_LOG_MODULE_NAME) && !defined(DISABLE_PER_MODULE_LOG_LEVELS) \
                                    && !defined(DISABLE_DEBUG_LOG_LEVELS)
/*! Macros to declare a variable to hold the log level for this module
 * The define of \c DEBUG_LOG_MODULE_NAME must occur in the header or
 * the source file before including this file for these macros to work.
 * Please note: Recommendation that this define must occur before
 * including any header file(s).
 */
#define __LOG_LEVEL_SYMBOL_FOR_NAME(name)   debug_log_level_##name
#define _LOG_LEVEL_SYMBOL_FOR_NAME(name)    __LOG_LEVEL_SYMBOL_FOR_NAME(name)
#define LOG_LEVEL_CURRENT_SYMBOL            _LOG_LEVEL_SYMBOL_FOR_NAME(DEBUG_LOG_MODULE_NAME)

/*! Macro used in a module to define the variable that holds its log level */
#define DEBUG_LOG_DEFINE_LEVEL_VAR          debug_log_level_t LOG_LEVEL_CURRENT_SYMBOL = DEFAULT_LOG_LEVEL;\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_ERROR)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_WARN)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_INFO)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_DEBUG)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_VERBOSE)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_V_VERBOSE)

#else
#define LOG_LEVEL_CURRENT_SYMBOL        debug_log_level__global

/* If per module log levels are disabled then define the statement to nothing
 * in case the module normally does support its own level */
#define DEBUG_LOG_DEFINE_LEVEL_VAR

#endif

/* ! Define global log functions for all levels regardless of DISABLE_PER_MODULE_LOG_LEVELS and
 * DISABLE_DEBUG_LOG_LEVELS as DEBUG_LOG_DEFINE_GLOBAL_VAR is used in all combinations above.
 */
#define DEBUG_LOG_DEFINE_GLOBAL_VAR \
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_ERROR)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_WARN)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_INFO)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_DEBUG)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_VERBOSE)\
                    DEBUG_LOG_FUNC_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_V_VERBOSE)

/*! Declare a variable for the local log level. This will be either
 * a module defined log level or the global one. */
extern debug_log_level_t LOG_LEVEL_CURRENT_SYMBOL;

/*! Declare a variable for the local log level. This is always global level. */
extern debug_log_level_t debug_log_level__global;


extern void DEBUG_LOG_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_ERROR)(const char *,int nargs,...);
extern void DEBUG_LOG_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_WARN)(const char *,int nargs,...);
extern void DEBUG_LOG_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_INFO)(const char *,int nargs,...);
extern void DEBUG_LOG_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_DEBUG)(const char *,int nargs,...);
extern void DEBUG_LOG_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_VERBOSE)(const char *,int nargs,...);
extern void DEBUG_LOG_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,DEBUG_LOG_LEVEL_V_VERBOSE)(const char *,int nargs,...);

/*! \brief  Unconditionally display the supplied string in the condensed log.
  Not intended to be used directly. Instead use the DEBUG_LOG_ERROR,
  DEBUG_LOG_WARN etc. macros.
    \param fmt  String to display in the log
 */
#define _DEBUG_LOG_UNCONDITIONAL(...) \
            do { \
                HYDRA_LOG_STRING(log_fmt, EXTRA_LOGGING_STRING FMT(__VA_ARGS__,bonus_arg)); \
                hydra_log_firm_variadic(log_fmt, VA_NARGS(__VA_ARGS__) + EXTRA_LOGGING_NUM_PARAMS EXTRA_LOGGING_PARAMS MAKE_REST_OF_ARGS(VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__)); \
            } while (0)

/*! \brief  Display the supplied string in the condensed log
  Not intended to be used directly. Instead use the DEBUG_LOG_ERROR,
  DEBUG_LOG_WARN etc. macros.
    \param level Level from the enum DEBUG_LOG_LEVEL_ENUM to specify the
        severity of the message
    \param fmt  String to display in the log
 */
#ifndef DISABLE_DEBUG_LOG_LEVELS

/* ! To conserve program space the condition checking the log level has been moved
 * into a log function itself. That applies only to _DEBUG_LOG_L.
 */
#define _DEBUG_LOG_L(level, ...) \
            do { \
                HYDRA_LOG_STRING(log_fmt, EXTRA_LOGGING_STRING FMT(__VA_ARGS__,bonus_arg)); \
                DEBUG_LOG_FOR_LEVEL(LOG_LEVEL_CURRENT_SYMBOL,level)(log_fmt, VA_NARGS(__VA_ARGS__) + EXTRA_LOGGING_NUM_PARAMS EXTRA_LOGGING_PARAMS MAKE_REST_OF_ARGS(VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__)); \
            } while (0)


#define _DEBUG_LOG_DATA_L(level, data, data_size) \
            do { \
                bool do_log = DebugLog_DataCallback(&LOG_LEVEL_CURRENT_SYMBOL, level, data, data_size); \
                if(level<=LOG_LEVEL_CURRENT_SYMBOL && do_log) \
                { \
                    debugLogData(data, data_size); \
                } \
            } while (0)

#else  /* DISABLE_DEBUG_LOG_LEVELS */
/*! If log levels are not enabled then assume all messages of DEFAULT_LOG_LEVEL
 * or more important should be shown */
#define _DEBUG_LOG_L(level, ...) \
            do { \
                if(level<=DEFAULT_LOG_FOR_DISABLED_LEVELS) \
                { \
                    _DEBUG_LOG_UNCONDITIONAL(__VA_ARGS__); \
                } \
            } while (0)

#define _DEBUG_LOG_DATA_L(level, data, data_size) \
            do { \
                if(level<=DEFAULT_LOG_FOR_DISABLED_LEVELS) \
                { \
                    debugLogData(data, data_size); \
                } \
            } while (0)

#endif /* DISABLE_DEBUG_LOG_LEVELS */

#else   /* DESKTOP_BUILD */

#define LOG_LEVEL_CURRENT_SYMBOL DEBUG_LOG_LEVEL_VERBOSE

/* Hydra log on desktop builds just runs using printf */
#define _DEBUG_LOG_L(level, ...) \
            do { \
                if(level<=LOG_LEVEL_CURRENT_SYMBOL) \
                { \
                    printf(FMT(__VA_ARGS__,bonus_arg) "\n" MAKE_REST_OF_ARGS(VA_ANY_ARGS(__VA_ARGS__))(__VA_ARGS__)); \
                } \
            } while (0)

#define _DEBUG_LOG_DATA_L(level, data, data_size) \
            debugLogData(data, data_size)

/* No per-module log levels */
#define DEBUG_LOG_DEFINE_LEVEL_VAR
/* No global logging functions */
#define DEBUG_LOG_DEFINE_GLOBAL_VAR
#endif  /* DESKTOP_BUILD */

/*! \brief  Include a string, without parameters in the
        character debug buffer

    See the description in the file as to why the use of this
    function is not recommended.

    \param fmt  String to display in the log
 */
#define DEBUG_PRINT(...) \
        printf(__VA_ARGS__)

#else   /* DISABLE_LOG */

#define _DEBUG_LOG_L(level, ...) ALL_UNUSED_IMPL(VA_NARGS(__VA_ARGS__)) (__VA_ARGS__ )
#define DEBUG_PRINT(...) ALL_UNUSED_IMPL(VA_NARGS(__VA_ARGS__)) (__VA_ARGS__ )
#define _DEBUG_LOG_DATA_L(level, ...) ALL_UNUSED_IMPL(VA_NARGS(__VA_ARGS__)) (__VA_ARGS__ )

/* No per-module log levels */
#define DEBUG_LOG_DEFINE_LEVEL_VAR
/* No global logging functions */
#define DEBUG_LOG_DEFINE_GLOBAL_VAR

#endif  /* DISABLE_LOG */

/*! Macro to print an unsigned long long in a DEBUG_LOG statement.

    Usage: DEBUG_LOG("Print unsigned long long 0x%08lx%08lx", PRINT_ULL(unsigned long long))
*/
#define PRINT_ULL(x)   ((uint32)(((x) >> 32) & 0xFFFFFFFFUL)),((uint32)((x) & 0xFFFFFFFFUL))

#endif /* LOGGING_H */
