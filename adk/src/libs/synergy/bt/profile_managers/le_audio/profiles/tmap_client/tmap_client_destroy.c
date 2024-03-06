/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #58 $
******************************************************************************/

#include "tmap_client_destroy.h"
#include "tmap_client_debug.h"

void tmapClientSendDestroyCfm(TMAP * tmapClientInst, TmapClientStatus status)
{
    TmapClientDestroyCfm *message = CsrPmemZalloc(sizeof(*message));

    message->id = TMAP_CLIENT_DESTROY_CFM;
    message->status = status;
    message->prflHndl = tmapClientInst->tmapSrvcHndl;

    TmapClientMessageSend(tmapClientInst->appTask, message);
}

bool TmapClientRemoveDevice(TmapClientProfileHandle profileHandle,
                            uint32 cid)
{
    TMAP * tmapClientInst = NULL;

    tmapClientInst = FIND_TMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    if (tmapClientInst)
    {
        REMOVE_TMAP_CLIENT_CID_ELEM(tmapClientInst->tmapClientCidList.cidList, cid);

        return TRUE;
    }

    return FALSE;
}

/******************************************************************************/
void TmapClientDestroyReq(TmapClientProfileHandle profileHandle)
{
    TMAP * tmapClientInst = ServiceHandleGetInstanceData(profileHandle);

    if (tmapClientInst)
    {
        if (tmapClientInst->tmapClientCidList.cidList.count == 0)
        {
            /* Send confirmation message with status in progress */
            tmapClientSendDestroyCfm(tmapClientInst, TMAP_CLIENT_STATUS_IN_PROGRESS);

            if (tmapClientInst->tmasSrvcHndl != 0)
                GattTmasClientTerminateReq(tmapClientInst->tmasSrvcHndl);
            else
                tmapClientDestroyProfileInst(tmapClientInst);
        }
        else
        {
            /* Send confirmation message with status active connection present */
            tmapClientSendDestroyCfm(tmapClientInst, TMAP_CLIENT_STATUS_ACTIVE_CONN_PRESENT);

            TMAP_CLIENT_WARNING("Cannot destroy instance as still active connections present");
        }
    }
    else
    {
        TMAP_CLIENT_ERROR("Invalid profile handle\n");
    }
}

/******************************************************************************/
void tmapClientSendTmasTerminateCfm(TMAP *tmapClientInst,
                                    TmapClientStatus status,
                                    GattTmasClientDeviceData handles)
{
    TmapClientTmasTerminateCfm *message = CsrPmemZalloc(sizeof(*message));

    message->id = TMAP_CLIENT_TMAS_TERMINATE_CFM;
    message->status = status;
    message->prflHndl = tmapClientInst->tmapSrvcHndl;

    memcpy(&(message->tmasClientHandle), &handles, sizeof(GattTmasClientDeviceData));

    TmapClientMessageSend(tmapClientInst->appTask, message);
}


/****************************************************************************/
void tmapClientHandleTmasClientTerminateResp(TMAP *tmapClientInst,
                                             const GattTmasClientTerminateCfm * message)
{
    if (message->status == GATT_TMAS_CLIENT_STATUS_SUCCESS)
    {

        /* Send the TMAS characteristic handles to the application */
        tmapClientSendTmasTerminateCfm(tmapClientInst,
                                       TMAP_CLIENT_STATUS_SUCCESS,
                                       message->deviceData);

        /* There are no instances of TMAS to terminate */
        tmapClientDestroyProfileInst(tmapClientInst);
    }
    else
    {
        tmapClientSendDestroyCfm(tmapClientInst, TMAP_CLIENT_STATUS_FAILED);
    }
}


/****************************************************************************/
void tmapClientDestroyProfileInst(TMAP *tmapClientInst)
{
    bool res = FALSE;
    AppTask appTask = tmapClientInst->appTask;

    /* Send the confirmation message */
    TmapClientDestroyCfm *message = CsrPmemZalloc(sizeof(*message));
    TmapClientMainInst *mainInst = tmapClientGetMainInstance();

    message->id = TMAP_CLIENT_DESTROY_CFM;
    message->prflHndl = tmapClientInst->tmapSrvcHndl;

    /* Free the profile instance memory */
    res = FREE_TMAP_CLIENT_INST(tmapClientInst->tmapSrvcHndl);

    /* Remove the profile element from main list */
    if (mainInst)
        REMOVE_TMAP_CLIENT_SERVICE_HANDLE(mainInst->profileHandleList, tmapClientInst->tmapSrvcHndl);

    if (res)
    {
        message->status = TMAP_CLIENT_STATUS_SUCCESS;
    }
    else
    {
        message->status = TMAP_CLIENT_STATUS_FAILED;
    }

    TmapClientMessageSend(appTask, message);
}
