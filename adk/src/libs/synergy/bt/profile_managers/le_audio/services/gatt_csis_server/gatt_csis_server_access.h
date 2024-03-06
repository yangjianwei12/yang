/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#ifndef GATT_CSIS_SERVER_ACCESS_H_
#define GATT_CSIS_SERVER_ACCESS_H_


#include "gatt_csis_server_private.h"


/***************************************************************************
NAME
    handleCsisServerAccessInd

DESCRIPTION
    Handle the access indications that were sent
    to the CSIS Server library.
*/
void handleCsisServerAccessInd(GCSISS_T *csis_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

void csisServerGenerateEncryptedSirk(GCSISS_T *csis_server,
    uint8 operation,
    uint8 index,
    connection_id_t cid);

void csisServerhandleSirkOperation(GCSISS_T *csis_server,
    const CsrBtCmLeSirkOperationCfm * cfm);

#endif

