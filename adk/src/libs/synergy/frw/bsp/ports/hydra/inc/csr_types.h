#ifndef CSR_TYPES_H__
#define CSR_TYPES_H__
/******************************************************************************
 Copyright (c) 2019 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"

#include "platform/csr_hydra_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef  FALSE
#define FALSE (0)

#undef  TRUE
#define TRUE (1)

/* Basic types */
typedef size_t CsrSize;         /* Return type of sizeof (ISO/IEC 9899:1990 7.1.6) */
typedef ptrdiff_t CsrPtrdiff;   /* Type of the result of subtracting two pointers (ISO/IEC 9899:1990 7.1.6) */

/* Unsigned fixed width types */
typedef uint8 CsrUint8;
typedef uint16 CsrUint16;
typedef uint32 CsrUint32;

/* Signed fixed width types */
typedef int8 CsrInt8;
typedef int16 CsrInt16;
typedef int32 CsrInt32;

/* Boolean */
typedef CsrUint8 CsrBool;

/* String types */
typedef char CsrCharString;
typedef CsrUint8 CsrUtf8String;
typedef CsrUint16 CsrUtf16String;   /* 16-bit UTF16 strings */
typedef CsrUint32 CsrUint24;


#ifdef __cplusplus
}
#endif

#endif
