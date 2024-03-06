/* Copyright (c) 2022 Qualcomm Technologies International, Ltd. */
/*   %%version */
/*
 * \file
 *
 *
 * This file contains what is needed to make a few entries appear in the
 * static SLT
 */

#ifndef PMALLOC_SLT_ENTRY_H
#define PMALLOC_SLT_ENTRY_H

#include "pmalloc/pmalloc_private.h"

#define CORE_PMALLOC_SLT_ENTRY(m)                                   \
    SLT_ENTRY(m, (APPS_SLT_PMALLOC_LENGTH, &pmalloc_num_pools))     \
    SLT_ENTRY(m, (APPS_SLT_PMALLOC_ARRAY_PTR, &pmalloc_pools))         \
    SLT_ENTRY(m, (APPS_SLT_PMALLOC_STRUCT_SIZE, sizeof(pmalloc_pool)))

#endif /* PMALLOC_SLT_ENTRY_H */
