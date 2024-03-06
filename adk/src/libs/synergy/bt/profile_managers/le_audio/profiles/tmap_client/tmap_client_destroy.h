/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef TMAP_CLIENT_DESTROY_H_
#define TMAP_CLIENT_DESTROY_H_

#include "tmap_client_private.h"

/***************************************************************************
NAME
    tmapClientSendDestroyCfm
    
DESCRIPTION
    Send the TMAP_CLIENT_DESTROY_CFM message.
*/
void tmapClientSendDestroyCfm(TMAP * tmapClientInst, TmapClientStatus status);

/***************************************************************************
NAME
    tmapClientSendTmasTerminateCfm

DESCRIPTION
    Send the TMAP_CLIENT_TMAS_TERMINATE_CFM message.
*/
void tmapClientSendTmasTerminateCfm(TMAP *tmapClientInst,
                                    TmapClientStatus status,
                                    GattTmasClientDeviceData handles);


/***************************************************************************
NAME
    tmapClientHandleTmasClientTerminateResp

DESCRIPTION
    Handle the GATT_TMAS_CLIENT_TERMINATE_CFM message.
*/
void tmapClientHandleTmasClientTerminateResp(TMAP *tmapClientInst,
                                             const GattTmasClientTerminateCfm * message);

/***************************************************************************
NAME
    tmapClientDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void tmapClientDestroyProfileInst(TMAP *tmapClientInst);

#endif
