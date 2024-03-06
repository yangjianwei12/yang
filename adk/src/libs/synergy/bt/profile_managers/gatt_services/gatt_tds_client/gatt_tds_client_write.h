/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef GATT_TDS_CLIENT_WRITE_H_
#define GATT_TDS_CLIENT_WRITE_H_


#include "gatt_tds_client.h"

/* Size of the TDS Control point Characteristic in number of octets
   for the opcodes without any parameters */
#define TDS_CLIENT_OPCODE_SIZE (1)

/* Size of the Media Control Point Characteristic in number of octects
   for the opcodes with values */
#define TDS_CLIENT_OPCODE_SIZE_PARAM (TDS_CLIENT_OPCODE_SIZE + sizeof(int32))

/***************************************************************************
NAME
    tdsClientHandleInternalWrite

DESCRIPTION
    Handle the TDS_CLIENT_INTERNAL_MSG_WRITE_REQ message.
*/
void tdsClientHandleInternalWrite(GTDSC *const tdsClient,
                                  uint16 sizeValue,
                                  uint8 * value);

void handleTdsWriteValueResp(GTDSC *tdsClient, uint16 handle, status_t resultCode);

void tdsClientHandleInternalSetTdsControlPoint(GTDSC *const tdsClient,
                                                 GattTdsOpcode op,
                                                 uint16 len,
                                                 uint8* val);
#endif
