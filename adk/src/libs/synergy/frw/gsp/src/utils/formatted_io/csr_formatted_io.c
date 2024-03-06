/*****************************************************************************
 Copyright (c) 2009-2018, The Linux Foundation.
 All rights reserved.
*****************************************************************************/

#include "csr_synergy.h"

#include "csr_types.h"
#include "csr_formatted_io.h"
#include "csr_util.h"

CsrInt32 CsrSnprintf(CsrCharString *dest, CsrSize n, const CsrCharString *fmt, ...)
{
    CsrInt32 r;
    va_list args;
    va_start(args, fmt);
    r = CsrVsnprintf(dest, n, fmt, args);
    va_end(args);

    if (dest && (n > 0))
    {
        dest[n - 1] = '\0';
    }

    return r;
}
