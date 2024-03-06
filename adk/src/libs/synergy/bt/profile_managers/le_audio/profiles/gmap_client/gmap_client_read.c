/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #2 $
******************************************************************************/

#include "gmap_client_debug.h"
#include "gmap_client_read.h"
#include "gmap_client_init.h"

#include "gatt_gmas_client.h"
void gmapClientHandleReadRoleCfm(GMAP *gmapClientInst,
                                 const GattGmasClientReadRoleCfm *cfm)
{
    void* msg = NULL;
    GmapClientReadRoleCfm* message = CsrPmemZalloc(sizeof(*message));

    message->id = GMAP_CLIENT_READ_ROLE_CFM;
    message->status   = cfm->status;
    message->prflHndl = gmapClientInst->gmapSrvcHndl;
    message->role = cfm->value;
    msg = (void*)message;

    GmapClientMessageSend(gmapClientInst->appTask, msg);
}

void gmapClientHandleReadUnicastFeaturesCfm(GMAP *gmapClientInst,
                                            const GattGmasClientReadUnicastFeaturesCfm *cfm)
{
    void* msg = NULL;
    GmapClientReadUnicastFeaturesCfm* message = CsrPmemZalloc(sizeof(*message));

    message->id = GMAP_CLIENT_READ_UNICAST_FEATURES_CFM;
    message->status   = cfm->status;
    message->prflHndl = gmapClientInst->gmapSrvcHndl;
    message->unicastFeatures = cfm->value;
    msg = (void*)message;

    GmapClientMessageSend(gmapClientInst->appTask, msg);
}

void gmapClientHandleReadBroadcastFeaturesCfm(GMAP *gmapClientInst,
                                              const GattGmasClientReadBroadcastFeaturesCfm *cfm)
{
    void* msg = NULL;
    GmapClientReadBroadcastFeaturesCfm* message = CsrPmemZalloc(sizeof(*message));

    message->id = GMAP_CLIENT_READ_BROADCAST_FEATURES_CFM;
    message->status   = cfm->status;
    message->prflHndl = gmapClientInst->gmapSrvcHndl;
    message->broadcastFeatures = cfm->value;
    msg = (void*)message;

    GmapClientMessageSend(gmapClientInst->appTask, msg);
}

void GmapClientReadRoleReq(GmapClientProfileHandle profileHandle)
{
    GMAP *gmapClientInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (gmapClientInst)
    {
        if (gmapClientInst->gmasSrvcHndl == 0)
        {
            void* msg = NULL;
            GmapClientReadRoleCfm* message = CsrPmemZalloc(sizeof(*message));
        
            message->id = GMAP_CLIENT_READ_ROLE_CFM;
            message->status   = GMAP_CLIENT_STATUS_SUCCESS_GMAS_SRVC_NOT_FOUND;
            message->prflHndl = gmapClientInst->gmapSrvcHndl;
            message->role = 0;
            msg = (void*)message;

            GmapClientMessageSend(gmapClientInst->appTask, msg);
        }
        else
            GattGmasClientReadRoleReq(gmapClientInst->gmasSrvcHndl);
    }
    else
    {
        GMAP_CLIENT_ERROR("GmapClientReadRoleReq: Invalid profile handle\n");
    }
}

void GmapClientReadUnicastFeaturesReq(GmapClientProfileHandle profileHandle, uint8 role)
{
    GMAP *gmapClientInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (gmapClientInst)
    {
        if (gmapClientInst->gmasSrvcHndl == 0)
        {
            void* msg = NULL;
            GmapClientReadUnicastFeaturesCfm* message = CsrPmemZalloc(sizeof(*message));
        
            message->id = GMAP_CLIENT_READ_UNICAST_FEATURES_CFM;
            message->status   = GMAP_CLIENT_STATUS_SUCCESS_GMAS_SRVC_NOT_FOUND;
            message->prflHndl = gmapClientInst->gmapSrvcHndl;
            message->unicastFeatures = 0;
            msg = (void*)message;

            GmapClientMessageSend(gmapClientInst->appTask, msg);
        }
        else
            GattGmasClientReadUnicastFeaturesReq(gmapClientInst->gmasSrvcHndl, role);
    }
    else
    {
        GMAP_CLIENT_ERROR("GmapClientReadUnicastFeaturesReq: Invalid profile handle\n");
    }
}

void GmapClientReadBroadcastFeaturesReq(GmapClientProfileHandle profileHandle, uint8 role)
{
    GMAP *gmapClientInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (gmapClientInst)
    {
        if (gmapClientInst->gmasSrvcHndl == 0)
        {
            void* msg = NULL;
            GmapClientReadBroadcastFeaturesCfm* message = CsrPmemZalloc(sizeof(*message));
        
            message->id = GMAP_CLIENT_READ_BROADCAST_FEATURES_CFM;
            message->status   = GMAP_CLIENT_STATUS_SUCCESS_GMAS_SRVC_NOT_FOUND;
            message->prflHndl = gmapClientInst->gmapSrvcHndl;
            message->broadcastFeatures = 0;
            msg = (void*)message;

            GmapClientMessageSend(gmapClientInst->appTask, msg);
        }
        else
            GattGmasClientReadBroadcastFeaturesReq(gmapClientInst->gmasSrvcHndl, role);
    }
    else
    {
        GMAP_CLIENT_ERROR("GmapClientReadBroadcastFeaturesReq: Invalid profile handle\n");
    }
}