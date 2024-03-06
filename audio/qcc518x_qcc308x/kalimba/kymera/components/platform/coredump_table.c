/****************************************************************************
 * Copyright (c) 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file coredump_table.c
 * \ingroup platform
 *
 * On-chip coredump region descriptor table
 */

#include "coredump_table.h"
#include "hal/hal_dm_sections.h"

/* Pointer to on-chip coredump region descriptor table
 * For now this is NULL, unless / until we work out
 * what it should contain to be useful
 */
DM_SHARED uint32 *coredump_region_ptr = NULL;

