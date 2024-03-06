/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_PACS_SERVER_PAC_RECORD_H_
#define GATT_PACS_SERVER_PAC_RECORD_H_

/***************************************************************************
NAME
    getGeneratedPacsRecord

DESCRIPTION
    Returns generated PACS Record for Sink or Source.
*/
uint8* getGeneratedPacsRecord(bool audioSink, uint8 *len, uint16 handle);
#endif

