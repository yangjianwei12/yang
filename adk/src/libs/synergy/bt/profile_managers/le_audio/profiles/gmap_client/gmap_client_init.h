/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GMAP_CLIENT_INIT_H_
#define GMAP_CLIENT_INIT_H_

#include "gmap_client_private.h"

/***************************************************************************
NAME
    gmapClientSendInitCfm
    
DESCRIPTION
    Send a GMAP_CLIENT_INIT_CFM message to the application.
*/
void gmapClientSendInitCfm(GMAP * gmapClientInst, GmapClientStatus status);


/***************************************************************************
NAME
    gmapClientHandleGmasClientInitResp

DESCRIPTION
    Handle the GATT_GMAS_CLIENT_INIT_CFM message.
*/
void gmapClientHandleGmasClientInitResp(GMAP *gmapClientInst,
                                        const GattGmasClientInitCfm * message);

#endif
