/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup boot Subsystem boot
 * \file boot.h
 * \ingroup boot
 *
 * Boot functionality executed just after the environment has been setup
 * to support functions written in C.
 */

#ifndef BOOT_H
#define BOOT_H

#include "proc/proc.h"

/** Specify the size of the stacks used in the subsystem.
 */
typedef struct
{
    unsigned array[PROC_PROCESSOR_BUILD]; /*!< Values must be a multiple of sizeof(unsigned). */
} BOOT_STACK_SIZES;

/**
 * \brief Finish boot process
 *
 * The function is called once the system has been configured.
 *
 * \param sizes Pointer to a structure representing the desired size of the stacks.
 */
extern void boot_configured(const BOOT_STACK_SIZES *sizes);

#endif /* BOOT_H */
