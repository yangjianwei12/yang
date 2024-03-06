/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GMAP_CLIENT_DESTROY_H_
#define GMAP_CLIENT_DESTROY_H_

#include "gmap_client_private.h"

/***************************************************************************
NAME
    gmapClientSendDestroyCfm
    
DESCRIPTION
    Send the GMAP_CLIENT_DESTROY_CFM message.
*/
void gmapClientSendDestroyCfm(GMAP * gmapClientInst, GmapClientStatus status);

/***************************************************************************
NAME
    gmapClientHandleGmasClientTerminateResp

DESCRIPTION
    Handle the GATT_GMAS_CLIENT_TERMINATE_CFM message.
*/
void gmapClientHandleGmasClientTerminateResp(GMAP *gmapClientInst,
                                             const GattGmasClientTerminateCfm * message);

/***************************************************************************
NAME
    gmapClientDestroyProfileInst

DESCRIPTION
    Destroy the profile memory instance.
*/
void gmapClientDestroyProfileInst(GMAP *gmapClientInst);

#endif
