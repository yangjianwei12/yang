/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_PACS_SERVER_PAC_RECORD_H_
#define GATT_PACS_SERVER_PAC_RECORD_H_

#define PAC_RECORD_SINK_HANDLE_OFFSET    (uint16)0x1000
#define PAC_RECORD_SOURCE_HANDLE_OFFSET  (uint16)0x2000
#define PAC_RECORD_VS_SINK_HANDLE_OFFSET    (uint16)0x4000
#define PAC_RECORD_VS_SOURCE_HANDLE_OFFSET  (uint16)0x8000


#define IS_PAC_RECORD_SINK(handle) ((handle & PAC_RECORD_SINK_HANDLE_OFFSET) == PAC_RECORD_SINK_HANDLE_OFFSET)
#define IS_PAC_RECORD_SOURCE(handle) ((handle & PAC_RECORD_SOURCE_HANDLE_OFFSET) == PAC_RECORD_SOURCE_HANDLE_OFFSET)
#define IS_PAC_RECORD_VS_SINK(handle) ((handle & PAC_RECORD_VS_SINK_HANDLE_OFFSET) == PAC_RECORD_VS_SINK_HANDLE_OFFSET)
#define IS_PAC_RECORD_VS_SOURCE(handle) ((handle & PAC_RECORD_VS_SOURCE_HANDLE_OFFSET) == PAC_RECORD_VS_SOURCE_HANDLE_OFFSET)

#define IS_PAC_RECORD_HANDLE_INVALID(handle) \
        ((handle & PACS_RECORD_ERRORCODE_BASE) == PACS_RECORD_ERRORCODE_BASE)

/***************************************************************************
NAME
    getGeneratedPacsRecord

DESCRIPTION
    Returns generated PACS Record for Sink or Source.
*/
uint8* getGeneratedPacsRecord(uint8 *len, uint16 handle);
#endif

