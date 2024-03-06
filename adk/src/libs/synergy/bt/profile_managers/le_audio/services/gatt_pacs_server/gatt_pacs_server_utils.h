/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

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


/***************************************************************************
NAME
    isPacRecordWithLtvPresent

DESCRIPTION
    Checks if any of the pac record holds the passed LTV data
*/

bool isPacRecordWithLtvPresent(void *list,
                        PacsCodecIdType codecId,
                        PacsVendorCodecIdType vendorCodecId,
                        PacRecordLtvType ltvType,
                        void *value);



/***************************************************************************
NAME
    getPacRecordList

DESCRIPTION
    Get pac_record list
*/

void* getPacRecordList(GPACSS_T *pacs, bool isAptx, bool isSink);

/***************************************************************************
NAME
    pacsServerGetDeviceIndexFromCid

DESCRIPTION
    Get device index from PACS connected client list.
*/

bool pacsServerGetDeviceIndexFromCid(GPACSS_T *pacs_server, ConnectionIdType cid, uint8 *index);

#endif

