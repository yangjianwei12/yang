/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #1 $
******************************************************************************/

#include "gmap_client_destroy.h"
#include "gmap_client_debug.h"

void gmapClientSendDestroyCfm(GMAP * gmapClientInst, GmapClientStatus status)
{
    GmapClientDestroyCfm *message = CsrPmemZalloc(sizeof(*message));

    message->id = GMAP_CLIENT_DESTROY_CFM;
    message->status = status;
    message->prflHndl = gmapClientInst->gmapSrvcHndl;

    GmapClientMessageSend(gmapClientInst->appTask, message);
}

bool GmapClientRemoveDevice(GmapClientProfileHandle profileHandle,
                            uint32 cid)
{
    GMAP * gmapClientInst = NULL;

    gmapClientInst = FIND_GMAP_CLIENT_INST_BY_PROFILE_HANDLE(profileHandle);

    if (gmapClientInst)
    {
        REMOVE_GMAP_CLIENT_CID_ELEM(gmapClientInst->gmapClientCidList.cidList, cid);

        return TRUE;
    }

    return FALSE;
}

/******************************************************************************/
void GmapClientDestroyReq(GmapClientProfileHandle profileHandle)
{
    GMAP * gmapClientInst = ServiceHandleGetInstanceData(profileHandle);

    if (gmapClientInst)
    {
        if (gmapClientInst->gmapClientCidList.cidList.count == 0)
        {
            /* Send confirmation message with status in progress */
            gmapClientSendDestroyCfm(gmapClientInst, GMAP_CLIENT_STATUS_IN_PROGRESS);

            if (gmapClientInst->gmasSrvcHndl != 0)
                GattGmasClientTerminateReq(gmapClientInst->gmasSrvcHndl);
            else
                gmapClientDestroyProfileInst(gmapClientInst);
        }
        else
        {
            /* Send confirmation message with status active connection present */
            gmapClientSendDestroyCfm(gmapClientInst, GMAP_CLIENT_STATUS_ACTIVE_CONN_PRESENT);

            GMAP_CLIENT_WARNING("Cannot destroy instance as still active connections present");
        }
    }
    else
    {
        GMAP_CLIENT_ERROR("Invalid profile handle\n");
    }
}

/****************************************************************************/
void gmapClientHandleGmasClientTerminateResp(GMAP *gmapClientInst,
                                             const GattGmasClientTerminateCfm * message)
{
    if (message->status == GATT_GMAS_CLIENT_STATUS_SUCCESS)
    {
        /* There are no instances of GMAS client to terminate */
        gmapClientDestroyProfileInst(gmapClientInst);
    }
    else
    {
        gmapClientSendDestroyCfm(gmapClientInst, GMAP_CLIENT_STATUS_FAILED);
    }
}


/****************************************************************************/
void gmapClientDestroyProfileInst(GMAP *gmapClientInst)
{
    bool res = FALSE;
    AppTask appTask = gmapClientInst->appTask;

    /* Send the confirmation message */
    GmapClientDestroyCfm *message = CsrPmemZalloc(sizeof(*message));
    GmapClientMainInst *mainInst = gmapClientGetMainInstance();

    message->id = GMAP_CLIENT_DESTROY_CFM;
    message->prflHndl = gmapClientInst->gmapSrvcHndl;

    /* Free the profile instance memory */
    res = FREE_GMAP_CLIENT_INST(gmapClientInst->gmapSrvcHndl);

    /* Remove the profile element from main list */
    if (mainInst)
        REMOVE_GMAP_CLIENT_SERVICE_HANDLE(mainInst->profileHandleList, gmapClientInst->gmapSrvcHndl);

    if (res)
    {
        message->status = GMAP_CLIENT_STATUS_SUCCESS;
    }
    else
    {
        message->status = GMAP_CLIENT_STATUS_FAILED;
    }

    GmapClientMessageSend(appTask, message);
}
