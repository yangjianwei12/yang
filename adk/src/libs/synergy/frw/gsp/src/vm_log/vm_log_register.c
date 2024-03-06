/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/
#include "csr_synergy.h"
#include "csr_pmem.h"
#include "csr_log_text_2.h"
#ifdef SYNERGY_LOG_FUNC_ENABLED
#include <hydra_log.h>
extern void hydra_log_firm_va_arg(const char *event_key, size_t n_args, va_list va_argp);
#endif

#ifdef CSR_LOG_ENABLE
#ifdef CSR_LOG_ENABLE_REGISTRATION
#define LOG_UNREGISTERED (0)

extern CsrLogTextHandle g_log_registrations[];
extern const CsrUint16 g_log_registrations_size;
CsrUint8 g_log_register_index = 0;
void CsrLogTextRegister2(CsrLogTextHandle **handle,
                         const CsrCharString *originName,
                         CsrUint16 subOriginsCount,
                         const CsrCharString *subOrigins[]);

/*****************************************************************************
    NAME
        CsrLogTextRegister2
        
    DESCRIPTION
        It is called by a Synergy Module during initialization if CSR_LOG_ENABLE
        and CSR_LOG_ENABLE_REGISTRATION are TRUE. This function will allocate 
        a CsrLogTextHandle structure and set the default log level for the 
        module as LOG_INFO.
        
        A log for this module will get printed if the log level specified
        in the log is less than or equal to both the global log level and 
        the module's log level stored in its CsrLogTextHandle structure.
        
        Module's log level can be dynamically changed by developers to suit
        there needs.
        
    PARAMETERS
        handle          - pointer to Log Handle for the task.
        originName      - Module name
        subOriginsCount - Number of sub modules (Not Supported)
        subOrigins      - List of names of sub modules (Not Supported)        
        
    RETURNS
        None
*****************************************************************************/
void CsrLogTextRegister2(CsrLogTextHandle **handle,
                         const CsrCharString *originName,
                         CsrUint16 subOriginsCount,
                         const CsrCharString *subOrigins[])
{
    int index;
    CSR_UNUSED(subOriginsCount);
    CSR_UNUSED(subOrigins);
    if (*handle !=NULL)
        return;

    if (g_log_register_index < g_log_registrations_size)
    {
        index = g_log_register_index;
        g_log_register_index++;
        *handle = &g_log_registrations[index];
        (*handle)->logLevel = (CsrUint8)LOG_INFO;
        if (originName)
        {
            CsrStrLCpy((*handle)->originName, originName, TASK_NAME_LEN_MAX);
        }
    }
}
#endif /*CSR_LOG_ENABLE_REGISTRATION*/

#ifdef SYNERGY_LOG_FUNC_ENABLED
/*****************************************************************************
    NAME
        SynergyLogPrint

    DESCRIPTION
        This is a function to log debug prints.
        It creates a va_list out of its variadic arguments and passes it on to
        Hydra's logging utility.
        Log is printed only of the log level passed as parameter is less than
        both the global and module's log level.

    PARAMETERS
        level    - Log Level
        handle   - Pointer to calling module's CsrLogTextHandle
        fmt      - Print string
        num_args - Number of print arguments. This is followed by variadic
                   arguments.

    RETURNS
        None
*****************************************************************************/
void SynergyLogPrint(int level, void* handle, const char *fmt, int num_args, ...)
{
#ifdef CSR_LOG_ENABLE_REGISTRATION
    synergy_debug_log_level_t modlevel = debug_log_level_synergy;
    if(handle != NULL)
        modlevel = ((CsrLogTextHandle*)handle)->logLevel;
    if (debug_log_level_synergy >= level && modlevel >= level)
#else
    CSR_UNUSED(handle);
    if (debug_log_level_synergy >= level)
#endif

    {
        va_list vargs;
        va_start(vargs, num_args);
        hydra_log_firm_va_arg(fmt, num_args, vargs);
        va_end(vargs);
    }
}
#endif /*SYNERGY_LOG_FUNC_ENABLED*/
#endif /*CSR_LOG_ENABLE*/
