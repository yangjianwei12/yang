/******************************************************************************
 Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#ifndef GATT_PACS_SERVER_ACCESS_H_
#define GATT_PACS_SERVER_ACCESS_H_

#include "gatt_pacs_server_private.h"

/***************************************************************************
NAME
    handlePacsServerAccessInd

DESCRIPTION
    Handle the access indications that were sent
    to the PACS Server library.
*/
void handlePacsServerAccessInd(GPACSS_T *pacs,
    const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
    uint16 maxRspValueLength);


void sendPacsServerAccessRsp(CsrBtGattId task,
                                    connection_id_t cid,
                                    uint16 handle,
                                    uint16 result,
                                    uint16 size_value,
                                    uint8 *const value);

#endif

