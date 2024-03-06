/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "tmap_client_debug.h"
#include "tmap_client_read.h"
#include "tmap_client_init.h"

#include "gatt_tmas_client.h"
/***************************************************************************/
static void tmapClientSendReadRoleCharacCfm(TMAP *tmapClientInst,
                                            ServiceHandle srvc_hndl,
                                            RoleType role,
                                            TmapClientStatus status)
{
    void* msg = NULL;
    TmapClientRoleCfm* message = CsrPmemZalloc(sizeof(*message));

    message->id = TMAP_CLIENT_ROLE_CFM;
    message->status   = status;
    message->prflHndl = tmapClientInst->tmapSrvcHndl;
    message->srvcHndl = srvc_hndl;
    message->role = role;
    msg = (void*)message;

    TmapClientMessageSend(tmapClientInst->appTask, msg);
}

/*******************************************************************************/
void tmapClientHandleReadRoleCharacCfm(TMAP *tmapClientInst,
                                       const GattTmasClientRoleCfm *msg)
{
    tmapClientSendReadRoleCharacCfm(tmapClientInst,
                                    msg->srvcHndl,
                                    msg->role,
                                    msg->status);
}

void TmapClientReadRoleReq(TmapClientProfileHandle profileHandle)
{
    TMAP *tmapClientInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (tmapClientInst)
    {
        if (tmapClientInst->tmasSrvcHndl == 0)
            tmapClientSendReadRoleCharacCfm(tmapClientInst, 0, 0, TMAP_CLIENT_STATUS_SUCCESS_TMAS_SRVC_NOT_FOUND);
        else
            GattTmasClientReadRoleReq(tmapClientInst->tmasSrvcHndl);
    }
    else
    {
        TMAP_CLIENT_ERROR("Invalid profile handle\n");
    }
}
