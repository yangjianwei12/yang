/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef TMAP_CLIENT_INIT_H_
#define TMAP_CLIENT_INIT_H_

#include "tmap_client_private.h"

/***************************************************************************
NAME
    tmapClientSendInitCfm
    
DESCRIPTION
    Send a TMAP_CLIENT_INIT_CFM message to the application.
*/
void tmapClientSendInitCfm(TMAP * tmapClientInst, TmapClientStatus status);


/***************************************************************************
NAME
    tmapClientHandleTmasClientInitResp

DESCRIPTION
    Handle the GATT_TMAS_CLIENT_INIT_CFM message.
*/
void tmapClientHandleTmasClientInitResp(TMAP *tmapClientInst,
                                        const GattTmasClientInitCfm * message);

#endif
