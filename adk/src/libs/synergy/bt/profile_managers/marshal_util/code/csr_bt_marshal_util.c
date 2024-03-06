/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include "csr_synergy.h"

#include "csr_bt_marshal_util.h"
#include "csr_types.h"
#include "csr_pmem.h"
#include "csr_util.h"
#include "csr_bt_panic.h"


struct CsrBtMarshalUtilInst
{
    CsrUint8 *buffer;

    CsrUint16 remainingLength;
    CsrUint16 index;
    CsrUint16 resumeIndex;

    CsrBtMarshalUtilType type:2;
    CsrBool overflow:1;
};

CsrBtMarshalUtilInst *CsrBtMarshalUtilCreate(CsrBtMarshalUtilType type)
{
    CsrBtMarshalUtilInst *conv = CsrPmemZalloc(sizeof(CsrBtMarshalUtilInst));
    conv->type = type;
    return (conv);
}

void CsrBtMarshalUtilDestroy(CsrBtMarshalUtilInst *conv)
{
    CsrPmemFree(conv);
}

void CsrBtMarshalUtilResetBuffer(CsrBtMarshalUtilInst *conv,
                                 CsrUint16 size,
                                 CsrUint8 *buf,
                                 CsrBool resetIndex)
{
    conv->buffer = buf;
    conv->remainingLength = size;
    conv->overflow = FALSE;

    if (resetIndex)
    {
        conv->index = 0;
    }
}

CsrBool CsrBtMarshalUtilStatus(CsrBtMarshalUtilInst *conv)
{
    return (!conv->overflow);
}

CsrUint16 CsrBtMarshalUtilRemainingLengthGet(CsrBtMarshalUtilInst *conv)
{
    return conv->remainingLength;
}

CsrUint16 CsrBtMarshalUtilTypeGet(CsrBtMarshalUtilInst *conv)
{
    return conv->type;
}

CsrBool CsrBtMarshalUtilConvert(CsrBtMarshalUtilInst *conv, void *obj, CsrUint16 len)
{
    CsrBool result = TRUE;

    if (conv->overflow)
    {
        result = FALSE;
    }
    else
    {
        if (conv->index < conv->resumeIndex)
        {
            /* skip serializing */
        }
        else
        {
            if (len <= conv->remainingLength)
            {
                if (conv->type == CSR_BT_MARSHAL_UTIL_SERIALIZER)
                {
                    CsrMemCpy(conv->buffer, obj, len); /* We don't care about endianess */
                }
                else
                {
                    CsrMemCpy(obj, conv->buffer, len); /* We don't care about endianess */
                }

                conv->buffer += len;
                conv->remainingLength -= len;
            }
            else
            {
                conv->resumeIndex = conv->index;
                conv->overflow = TRUE;
                result = FALSE;

                if (conv->type == CSR_BT_MARSHAL_UTIL_DESERIALIZER && conv->remainingLength)
                {
                    CsrPanic(CSR_TECH_BT,
                             (CsrUint16) (CSR_BT_PANIC_MYSTERY),
                             "Partial object received");
                }
            }
        }
        conv->index += len;
    }

    return result;
}

void CsrBtMarshalUtilConvertObjPtr(CsrBtMarshalUtilInst *conv,
                                   CsrBtMarshalUtilConvertFn fn,
                                   void **obj,
                                   CsrUint16 size)
{
    CsrUint8 present = *obj ? TRUE : FALSE;

    CsrBtMarshalUtilConvert(conv, &present, sizeof(present));

    if (present)
    {
        if (conv->type == CSR_BT_MARSHAL_UTIL_DESERIALIZER)
        {
            if (!(*obj))
            {
                *obj = CsrPmemZalloc(size);
            }
        }

        fn(conv, *obj, size);
    }
}

