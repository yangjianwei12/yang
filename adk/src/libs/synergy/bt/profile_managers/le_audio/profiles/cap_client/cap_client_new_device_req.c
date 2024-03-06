/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_new_device_req.h"
#include "cap_client_util.h"
#include "cap_client_debug.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
static void capClientInitInitializeBapProfile(CapClientGroupInstance *const cap,
                                       BapHandles *handles,
                                       uint32 cid)
{
    /*initialize service and profiles data*/
    BapInstElement *bap = NULL;
    bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_GROUPID(cap->bapList, cap->groupId);

    if (bap == NULL)
    {
        bap = (BapInstElement*)CAP_CLIENT_ADD_BAP_INST(cap->bapList);
    }

    if (bap == NULL)
    {
        /*TODO: Add debug log here*/
        CAP_CLIENT_PANIC("\n capClientInitInitializeBapProfile: NULL instance of BAP\n");
        return;
    }

    bap->cid = cid;
    bap->groupId = cap->groupId;
    bap->recentStatus = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    /* Bap state is getting overriden here to idle as in case of dummy bap creation state will be invalid and when actual addition of
     * The device will happen, function(CAP_CLIENT_GET_BAP_ELEM_FROM_GROUPID) will find the bap instance based on group id and bap state */
    setBapStatePerCigId(bap, CAP_CLIENT_BAP_STATE_IDLE, CAP_CLIENT_BAP_STATE_INVALID);
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    if (cap->role & CAP_CLIENT_COMMANDER)
    {
        bap->bass = (BroadcastAssistantInst*)CsrPmemZalloc(sizeof(BroadcastAssistantInst));
    }
#endif

    if (handles == NULL)
    {
        /*Device already present, Hence return*/
        return;
    }

    bap->bapData = (BapHandles*)CsrPmemZalloc(sizeof(BapHandles));

    bap->bapData->ascsHandles = (BapAscsClientDeviceData*)
                              CsrPmemZalloc(sizeof(BapAscsClientDeviceData));
    CsrMemCpy(bap->bapData->ascsHandles,
                       handles->ascsHandles, sizeof(BapAscsClientDeviceData));
    bap->bapData->pacsHandles = (BapPacsClientDeviceData*)
                              CsrPmemZalloc(sizeof(BapPacsClientDeviceData));
    CsrMemCpy(bap->bapData->pacsHandles,
                       handles->pacsHandles, sizeof(BapPacsClientDeviceData));

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
    if (cap->role & CAP_CLIENT_COMMANDER)
    {

        bap->bapData->bassHandles = (BapBassClientDeviceData*)
                                  CsrPmemZalloc(sizeof(BapBassClientDeviceData));
        CsrMemCpy(bap->bapData->bassHandles,
                           handles->bassHandles, sizeof(BapBassClientDeviceData));
    }
#endif

}

static void capClientInitInitializeVcpProfile(CapClientGroupInstance *const cap,
                                       GattVcsClientDeviceData *handles,
                                       uint32 cid)
{
    /*initialize service and profiles data*/
    VcpInstElement *vcp = NULL;
    vcp = (VcpInstElement*)CAP_CLIENT_GET_VCP_ELEM_FROM_CID(cap->vcpList, cid);

    if (vcp == NULL)
    {
        vcp = (VcpInstElement*)CAP_CLIENT_ADD_VCP_INST(cap->vcpList);
    }


    vcp->cid = cid;
    vcp->groupId = cap->groupId;
    vcp->vcpHandle = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    vcp->recentStatus = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;


    if (handles == NULL)
    {
        vcp->vcsData = NULL;
        /*Device already present, Hence return*/
        return;
    }

    vcp->vcsData = (GattVcsClientDeviceData*)
                              CsrPmemZalloc(sizeof(GattVcsClientDeviceData));
    CsrMemCpy(vcp->vcsData, handles, sizeof(GattVcsClientDeviceData));

}

static void capClientInitInitializeCsipProfile(CapClientGroupInstance *const cap,
                                       GattCsisClientDeviceData *handles,
                                       uint32 cid)
{
    /*initialize service and profiles data*/
    CsipInstElement *csip = NULL;
    csip = (CsipInstElement*)CAP_CLIENT_GET_CSIP_ELEM_FROM_CID(cap->csipList, cid);

    if(csip == NULL)
    {
        /* Device Not Present, Hence add the Device*/
        csip = (CsipInstElement*)CAP_CLIENT_ADD_CSIP_INST(cap->csipList);
    }

    csip->cid = cid;
    csip->groupId = cap->groupId;
    csip->csipHandle = CAP_CLIENT_INVALID_SERVICE_HANDLE;
    csip->recentStatus = CAP_CLIENT_RESULT_FAILURE_UNKNOWN_ERR;

    if (handles == NULL)
    {
        /*No Handles So No need to copy, Hence return*/
        return;
    }

    csip->csipData = (GattCsisClientDeviceData*)
                          CsrPmemZalloc(sizeof(GattCsisClientDeviceData));
    CsrMemCpy(csip->csipData, handles, sizeof(GattCsisClientDeviceData));
}

void capClientInitCoordinatedSet(CapClientGroupInstance* cap)
{
    /*Initialize cap instance data*/
    cap->setSize = 0;
    cap->sirkType = 0;
    cap->profileListSort = FALSE;
}

void capClientInitAddProfileAndServices(CapClientGroupInstance *const cap,
                                   uint32 cid, CapClientHandleList *handles)
{
    /*Dont add same CID to same group twice*/
    BapHandles* capBapHandles = NULL;
    GattVcsClientDeviceData* vcsHandles = NULL;
    GattCsisClientDeviceData* csipHandles = NULL;

    if (cap == NULL)
    {
        CAP_CLIENT_ERROR("\n(CAP):capInitAddProfileAndServices, NULL instance ");
        return;
    }

    if (handles)
    {
        if ((handles->ascsData != NULL) ||
#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
            (handles->bassData != NULL) ||
#endif
            (handles->pacsData != NULL))
        {
            capBapHandles = (BapHandles*)CsrPmemZalloc(sizeof(BapHandles));
            capBapHandles->ascsHandles = NULL;
            capBapHandles->pacsHandles = NULL;

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
            capBapHandles->bassHandles = NULL;
#endif
        }

        if (handles->ascsData)
        {
            capBapHandles->ascsHandles = handles->ascsData;
        }

        if (handles->pacsData)
        {
            capBapHandles->pacsHandles = handles->pacsData;
        }

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        if(handles->bassData)
        {
            capBapHandles->bassHandles = handles->bassData;
        }
#endif

        if (handles->csisData)
        {
            csipHandles = handles->csisData;
        }

        if (handles->vcsData)
        {
            vcsHandles = handles->vcsData;
        }
    }

    /* BAP CSIP roles are required both in case of Commander and Initiator */
    capClientInitInitializeCsipProfile(cap, csipHandles, cid);
    capClientInitInitializeBapProfile(cap, capBapHandles, cid);

    if (cap->role & CAP_CLIENT_COMMANDER)
    {
        capClientInitInitializeVcpProfile(cap, vcsHandles, cid);
    }

    /* Free Device Data*/
    if (handles)
    {
        CsrPmemFree(handles->ascsData);
        handles->ascsData = NULL;

        CsrPmemFree(handles->pacsData);
        handles->pacsData = NULL;

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
        CsrPmemFree(handles->bassData);
        handles->bassData = NULL;
#endif
        CsrPmemFree(handles->csisData);
        handles->csisData = NULL;

        CsrPmemFree(handles->vcsData);
        handles->vcsData = NULL;

        CsrPmemFree(capBapHandles);
        capBapHandles = NULL;
    }
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
