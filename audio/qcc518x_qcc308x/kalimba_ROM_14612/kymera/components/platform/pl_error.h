/****************************************************************************
 * Copyright (c) 2008 - 2019 Qualcomm Technologies International, Ltd.
 ************************************************************************//**
 * \file pl_error.h
 * \ingroup platform
 *
 * Header file for the kalimba error handler
 *
 ****************************************************************************/

#ifndef PL_ERROR_H
#define PL_ERROR_H

#include "types.h"

/****************************************************************************
Include Files
*/

/****************************************************************************
Public Macro Declarations
*/

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Global Variable Definitions
*/

/****************************************************************************
Public Function Prototypes
*/

/* General-purpose error handler for Kalimba libs */
extern void pl_lib_error_handler(unsigned data);

/* Handler for un-registered interrupt */
extern void pl_unknown_interrupt_handler(void);

/* Handler for memory-access exception */
void pl_sw_exception_handler(unsigned code_address);

/* Handler for stack-overflow exception */
void pl_stack_exception_handler(unsigned data);

/* Enable handler functions for memory exceptions.
 * We do this because we may handle an exception early in the boot sequence,
 * before we're ready to deal with it (e.g. the sssm has finished and panic is
 * ready) */
extern void error_enable_exception_handlers(bool enable);

#ifdef __KCC__
#include "audio_log/audio_log.h"  /* for AUDIO_LOG_STRING() */

/* Handler for ARITHMETIC_MODE error (invoked from calls below) */
extern void pl_arithmetic_mode_error(const char *reason, unsigned int context,
                                     unsigned int mode);

/* Assembly routine to check ARITHMETIC_MODE sanity.
 * (Usually you should use CHECK_ARITHMETIC_MODE_SANITY() rather
 * than calling this directly.) */
extern void check_arithmetic_mode_sanity(const char *reason,
                                         unsigned int context);

/* Check the Kalimba ARITHMETIC_MODE register is sane, and panic
 * if not. Arguments are used in a log message like
 * ("... %s 0x%X", reasonstr, context) */
#define CHECK_ARITHMETIC_MODE_SANITY(reasonstr, context) \
    do { \
        AUDIO_LOG_STRING(reason, reasonstr); \
        check_arithmetic_mode_sanity(reason, context); \
    } while(0)
#else  /* __KCC__ */
/* This isn't a meaningful check except on a Kalimba */
/* Some callers use the non-macro form, so make that go away too */
#define check_arithmetic_mode_sanity(reason, context) \
    do { \
        /* Mark arguments as UNUSED */ \
        (void)reason; \
        (void)context; \
    } while(0)
#define CHECK_ARITHMETIC_MODE_SANITY(reason, context) \
    check_arithmetic_mode_sanity(reason,context)
#endif /* __KCC__ */

#endif   /* PL_ERROR_H */
