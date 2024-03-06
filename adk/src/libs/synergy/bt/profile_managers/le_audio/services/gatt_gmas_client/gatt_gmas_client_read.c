/******************************************************************************
 Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #3 $
******************************************************************************/


#include "gatt_gmas_client.h"
#include "gatt_gmas_client_debug.h"
#include "gatt_gmas_client_private.h"
#include "gatt_gmas_client_read.h"
#include "gatt_gmas_client_common_util.h"

/***************************************************************************/
void gattGmasClientHandleInternalReadRole(const GGMASC * gmasClient, uint16 handle)
{
    if (handle != GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattReadReqSend(gmasClient->srvcElem->gattId,
                             gmasClient->srvcElem->cid,
                             handle,
                             0);
    }
    else
    {
        GattGmasClientReadRoleCfm *message = CsrPmemZalloc(sizeof(*message));

        message->status = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        message->srvcHndl = gmasClient->srvcElem->service_handle;
        message->value = 0x00;

        GattGmasClientMessageSend(gmasClient->appTask, GATT_GMAS_CLIENT_READ_ROLE_CFM, message);
    }
}

void gattGmasClientHandleInternalReadUnicastFeatures(const GGMASC * gmasClient, uint16 handle)
{
    if (handle != GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattReadReqSend(gmasClient->srvcElem->gattId,
                             gmasClient->srvcElem->cid,
                             handle,
                             0);
    }
    else
    {
        GattGmasClientReadUnicastFeaturesCfm *message = CsrPmemZalloc(sizeof(*message));

        message->status = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        message->srvcHndl = gmasClient->srvcElem->service_handle;
        message->value = 0x00;

        GattGmasClientMessageSend(gmasClient->appTask, GATT_GMAS_CLIENT_READ_UNICAST_FEATURES_CFM, message);
    }
}

void gattGmasClientHandleInternalReadBroadcastFeatures(const GGMASC * gmasClient, uint16 handle)
{
    if (handle != GATT_ATTR_HANDLE_INVALID)
    {
        CsrBtGattReadReqSend(gmasClient->srvcElem->gattId,
                             gmasClient->srvcElem->cid,
                             handle,
                             0);
    }
    else
    {
        GattGmasClientReadBroadcastFeaturesCfm *message = CsrPmemZalloc(sizeof(*message));

        message->status = CSR_BT_GATT_RESULT_UNACCEPTABLE_PARAMETER;
        message->srvcHndl = gmasClient->srvcElem->service_handle;
        message->value = 0x00;

        GattGmasClientMessageSend(gmasClient->appTask, GATT_GMAS_CLIENT_READ_BROADCAST_FEATURES_CFM, message);
    }
}
/****************************************************************************/
void gattGmasClientHandleReadValueRespCfm(const GGMASC *gmasClient,
                                          const CsrBtGattReadCfm *readCfm)
{
    if (readCfm->handle == gmasClient->handles.roleHandle)
        gattGmasClientHandleReadRoleValueRespCfm(gmasClient, readCfm);
    else if ((readCfm->handle == gmasClient->handles.uggFeaturesHandle) ||
            (readCfm->handle == gmasClient->handles.ugtFeaturesHandle))
        gattGmasClientHandleReadUnicastFeaturesValueRespCfm(gmasClient, readCfm);
    else if ((readCfm->handle == gmasClient->handles.bgsFeaturesHandle) ||
             (readCfm->handle == gmasClient->handles.bgrFeaturesHandle))
        gattGmasClientHandleReadBroadcastFeaturesValueRespCfm(gmasClient, readCfm);
}

void gattGmasClientHandleReadRoleValueRespCfm(const GGMASC *gmasClient,
                                              const CsrBtGattReadCfm *readCfm)
{
    GattGmasClientReadRoleCfm *message = CsrPmemZalloc(sizeof(*message));

    message->status = getGmasClientStatusFromGattStatus(readCfm->resultCode);
    message->srvcHndl = gmasClient->srvcElem->service_handle;

    if(readCfm->valueLength && readCfm->value)
    {
        message->value = readCfm->value[0];
    }
    else
        message->value = 0;

    GattGmasClientMessageSend(gmasClient->appTask, GATT_GMAS_CLIENT_READ_ROLE_CFM, message);
}


void gattGmasClientHandleReadUnicastFeaturesValueRespCfm(const GGMASC *gmasClient,
                                                         const CsrBtGattReadCfm *readCfm)
{
    GattGmasClientReadUnicastFeaturesCfm *message = CsrPmemZalloc(sizeof(*message));

    message->status = getGmasClientStatusFromGattStatus(readCfm->resultCode);
    message->srvcHndl = gmasClient->srvcElem->service_handle;

    if(readCfm->valueLength && readCfm->value)
    {
        message->value = readCfm->value[0];
    }
    else
        message->value = 0;

    GattGmasClientMessageSend(gmasClient->appTask, GATT_GMAS_CLIENT_READ_UNICAST_FEATURES_CFM, message);
}

void gattGmasClientHandleReadBroadcastFeaturesValueRespCfm(const GGMASC *gmasClient,
                                                           const CsrBtGattReadCfm *readCfm)
{
    GattGmasClientReadBroadcastFeaturesCfm *message = CsrPmemZalloc(sizeof(*message));

    message->status = getGmasClientStatusFromGattStatus(readCfm->resultCode);
    message->srvcHndl = gmasClient->srvcElem->service_handle;

    if(readCfm->valueLength && readCfm->value)
    {
        message->value = readCfm->value[0];
    }
    else
        message->value = 0;

    GattGmasClientMessageSend(gmasClient->appTask, GATT_GMAS_CLIENT_READ_BROADCAST_FEATURES_CFM, message);
}

void GattGmasClientReadRoleReq(ServiceHandle clntHndl)
{
    GGMASC *gmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (gmasClient)
    {
        GattGmasClientInternalMsgReadRole *message = CsrPmemZalloc(sizeof(*message));
        message->srvcHndl = gmasClient->srvcElem->service_handle;
        message->handle = gmasClient->handles.roleHandle;

        GattGmasClientMessageSend(gmasClient->libTask, GATT_GMAS_CLIENT_INTERNAL_MSG_READ_ROLE, message);
    }
    else
    {
        GATT_GMAS_CLIENT_PANIC("Invalid Gmas instance!\n");
    }
}

void GattGmasClientReadUnicastFeaturesReq(ServiceHandle clntHndl, uint8 role)
{
    GGMASC *gmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (gmasClient)
    {
        GattGmasClientInternalMsgReadUnicastFeatures *message = CsrPmemZalloc(sizeof(*message));
        message->srvcHndl = gmasClient->srvcElem->service_handle;

        if ((role & 0x01) == 0x01)
            message->handle = gmasClient->handles.uggFeaturesHandle;

        else if ((role & 0x02) == 0x02)
            message->handle = gmasClient->handles.ugtFeaturesHandle;

        GattGmasClientMessageSend(gmasClient->libTask, GATT_GMAS_CLIENT_INTERNAL_MSG_READ_UNICAST_FEATURES, message);
    }
    else
    {
        GATT_GMAS_CLIENT_PANIC("Invalid Gmas instance!\n");
    }
}

void GattGmasClientReadBroadcastFeaturesReq(ServiceHandle clntHndl, uint8 role)
{
    GGMASC *gmasClient = ServiceHandleGetInstanceData(clntHndl);

    if (gmasClient)
    {
        GattGmasClientInternalMsgReadBroadcastFeatures *message = CsrPmemZalloc(sizeof(*message));
        message->srvcHndl = gmasClient->srvcElem->service_handle;

        if ((role & 0x04) == 0x04)
            message->handle = gmasClient->handles.bgsFeaturesHandle;

        else if ((role & 0x08) == 0x08)
            message->handle = gmasClient->handles.bgrFeaturesHandle;

        GattGmasClientMessageSend(gmasClient->libTask, GATT_GMAS_CLIENT_INTERNAL_MSG_READ_BROADCAST_FEATURES, message);
    }
    else
    {
        GATT_GMAS_CLIENT_PANIC("Invalid Gmas instance!\n");
    }
}