#ifndef CSR_PMEM_H__
#define CSR_PMEM_H__
/*****************************************************************************
Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

REVISION:      $Revision: #2 $
*****************************************************************************/

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_util.h"

#include "platform/csr_hydra_pmalloc.h"
#include "csr_panic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************

    NAME
        CsrPmemAlloc

    DESCRIPTION
        This function will allocate a contiguous block of memory of at least
        the specified size in bytes and return a pointer to the allocated
        memory. This function is not allowed to return NULL. A size of 0 is a
        valid request, and a unique and valid (not NULL) pointer must be
        returned in this case.

    PARAMETERS
        size - Size of memory requested. Note that a size of 0 is valid.

    RETURNS
        Pointer to allocated memory.

*****************************************************************************/
#define CsrPmemAlloc(size) PanicUnlessMalloc(size)


/*****************************************************************************

    NAME
        CsrPmemZalloc

    DESCRIPTION
        This function is equivalent to CsrPmemAlloc, but the allocated memory
        is initialised to zero.

    PARAMETERS
        size - Size of memory requested. Note that a size of 0 is valid.

    RETURNS
        Pointer to allocated memory.

*****************************************************************************/
#define CsrPmemZalloc(s) (CsrMemSet(CsrPmemAlloc(s), 0x00, (s)))


/*****************************************************************************

    NAME
        CsrPmemFree

    DESCRIPTION
        This function will deallocate a previously allocated block of memory.

    PARAMETERS
        ptr - Pointer to allocated memory.

*****************************************************************************/
#define CsrPmemFree(ptr) pfree(ptr)

#if !defined(HYDRA) && !defined(CAA)
/*****************************************************************************

    NAME
        pnew and zpnew

    DESCRIPTIOM
        Type-safe wrappers for CsrPmemAlloc and CsrPmemZalloc, for allocating
        single instances of a specified and named type.

    PARAMETERS
        t - type to allocate.

*****************************************************************************/
#define pnew(t) ((t *) (CsrPmemAlloc(sizeof(t))))
#define zpnew(t) ((t *) (CsrPmemZalloc(sizeof(t))))
#endif

#define SynergyMessageFree(_id, _msg)                                       \
    do                                                                      \
    {                                                                       \
        if (_msg)                                                           \
        {                                                                   \
            MessageFree((_id), (_msg));                                     \
        }                                                                   \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif

