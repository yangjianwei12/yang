/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef CSR_BT_MARSHAL_UTIL_H_
#define CSR_BT_MARSHAL_UTIL_H_

#include "csr_synergy.h"
#include "csr_types.h"


typedef enum
{
    CSR_BT_MARSHAL_UTIL_SERIALIZER,
    CSR_BT_MARSHAL_UTIL_DESERIALIZER
} CsrBtMarshalUtilType;

typedef struct CsrBtMarshalUtilInst CsrBtMarshalUtilInst;

CsrBtMarshalUtilInst *CsrBtMarshalUtilCreate(CsrBtMarshalUtilType type);

/* Destroys serializer/deserializer */
void CsrBtMarshalUtilDestroy(CsrBtMarshalUtilInst *inst);

/* Sets buffer for serializer/deserializer to work on. Optionally, resets the internal index of the serializer/deserializer */
void CsrBtMarshalUtilResetBuffer(CsrBtMarshalUtilInst *inst,
                                 CsrUint16 size,
                                 CsrUint8 *buf,
                                 CsrBool resetIndex);

/* Returns TRUE if serialization/deserialization was successful, else returns FALSE */
CsrBool CsrBtMarshalUtilStatus(CsrBtMarshalUtilInst *inst);

/* Returns remaining buffer length */
CsrUint16 CsrBtMarshalUtilRemainingLengthGet(CsrBtMarshalUtilInst *inst);

/* Returns marshal util type */
CsrUint16 CsrBtMarshalUtilTypeGet(CsrBtMarshalUtilInst *inst);

/* Serializes/Deserializes the object */
CsrBool CsrBtMarshalUtilConvert(CsrBtMarshalUtilInst *inst,
                                void *obj,
                                CsrUint16 len);

#define CsrBtMarshalUtilConvertObj(_inst, _obj)     CsrBtMarshalUtilConvert(_inst, &(_obj), sizeof(_obj));

#define CsrBtMarshalUtilConvertUint8(_c, _obj)      MarshalingConvert((_c), &(_obj), 1);
#define CsrBtMarshalUtilConvertUint16(_c, _obj)     MarshalingConvert((_c), &(_obj), 2);
#define CsrBtMarshalUtilConvertUint32(_c, _obj)     MarshalingConvert((_c), &(_obj), 4);

typedef void (*CsrBtMarshalUtilConvertFn)(CsrBtMarshalUtilInst *inst, void *obj, CsrUint16 len);

void CsrBtMarshalUtilConvertObjPtr(CsrBtMarshalUtilInst *inst,
                                   CsrBtMarshalUtilConvertFn fn,
                                   void **obj,
                                   CsrUint16 size);

#endif /* CSR_BT_MARSHAL_UTIL_H_ */
