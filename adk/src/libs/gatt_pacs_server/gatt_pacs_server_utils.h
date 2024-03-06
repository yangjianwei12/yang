/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_PACS_SERVER_UTILS_H_
#define GATT_PACS_SERVER_UTILS_H_

/***************************************************************************
NAME
    generatePacRecordHandle

DESCRIPTION
    Returns PAC Record handle for Sink PAC Record or Source PAC Record.
*/
uint16 generatePacRecordHandle(GPACSS_T *pacs_server);

/***************************************************************************
NAME
    removePacRecordHandle

DESCRIPTION
    Removes the pac_record_handle from list of pac_record_handles
*/
void removePacRecordHandle(GPACSS_T *pacs_server, uint16 pac_record_handle);

#endif

