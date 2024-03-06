/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #4 $
******************************************************************************/

#include "gatt_qss_server_access.h"
#include "gatt_qss_server.h"
#include "gatt_qss_server_private.h"

extern ServiceHandle qssServerServiceHandle;

/****************************************************************************/
bool GattQssServerReadQssSupportResponse(ServiceHandle srvHndl,
                                         ConnectionId cid,
                                         uint8 isQssSupport)
{
    GQSSS* gattQssServerInst = NULL;
    uint16 status = CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
    
    if (srvHndl != qssServerServiceHandle)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID SERVICE HANDLE VALUE \n"));
        return FALSE;
    }
    if (cid == 0)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID CID \n"));
        return FALSE;
    }
    
    gattQssServerInst = (GQSSS*)ServiceHandleGetInstanceData(srvHndl);

    if (gattQssServerInst == NULL)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INSTANCE NULL POINTER \n"));
        return FALSE;
    }

    if (isQssSupport <= QSS_SUPPORTED)
    {
        status = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }

    gattQssServerSendQssSupportAccessRsp(gattQssServerInst,
                                         cid,
                                         isQssSupport,
                                         status);
    
    return TRUE;
}

/****************************************************************************/
bool GattQssServerReadUserDescriptionResponse(ServiceHandle srvHndl,
                                              ConnectionId cid,
                                              uint16 length,
                                              uint8* description)
{
    GQSSS* gattQssServerInst = NULL;
    uint16 status = CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
    
    if (srvHndl != qssServerServiceHandle)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID SERVICE HANDLE VALUE \n"));
        return FALSE;
    }
    
    if (cid == 0)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID CID \n"));
        return FALSE;
    }

    if (description != NULL && length != 0)
    {
        status = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }

    gattQssServerInst = (GQSSS*) ServiceHandleGetInstanceData(srvHndl);

    if (gattQssServerInst == NULL)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INSTANCE NULL POINTER \n"));
        return FALSE;
    }
    
    gattQssServerSendUserDescriptionAccessRsp(gattQssServerInst,
                                              cid,
                                              length,
                                              description,
                                              status);

    return TRUE;
}

/****************************************************************************/
bool GattQssServerReadLosslessAudioResponse (ServiceHandle srvHndl,
                                             ConnectionId cid,
                                             uint32 losslessAudio)
{
    GQSSS* gattQssServerInst = NULL;
    uint16 status = CSR_BT_GATT_ACCESS_RES_INSUFFICIENT_RESOURCES;
    uint8 losslessMode = (uint8)(losslessAudio >> 24);
    uint8 losslessFormat = (uint8)(losslessAudio >> 16);

    if (srvHndl != qssServerServiceHandle)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID SERVICE HANDLE VALUE \n"));
        return FALSE;
    }
    if (cid == 0)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID CID \n"));
        return FALSE;
    }
    
    /* If lossless mode is not suppouted then losslessFormat should be 0x00.
     * If lossless mode is supported then losslessFormat could be 0x00 or non-zero value.
     */
    if(!((losslessMode == LOSSLESS_MODE_NOT_SUPPORTED) && (losslessFormat != LOSSLESS_FORMAT_UNKNOWN)) 
       && (losslessMode <= LOSSLESS_MODE_SUPPORTED))
    {
        status = CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }
    
    gattQssServerInst = (GQSSS*) ServiceHandleGetInstanceData(srvHndl);

    if (gattQssServerInst == NULL)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INSTANCE NULL POINTER \n"));
        return FALSE;
    }
    
    gattQssServerSendLosslessAudioAccessRsp(gattQssServerInst,
                                            cid,
                                            losslessAudio,
                                            status);

    return TRUE;
}

/****************************************************************************/
bool GattQssServerReadClientConfigResponse(ServiceHandle srvHndl, 
                                           ConnectionId cid, 
                                           uint16 clientConfig)
{
    GQSSS* gattQssServerInst = NULL;

    if (cid == 0)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID CID \n"));
        return FALSE;
    }
    if (srvHndl != qssServerServiceHandle)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID SERVICE HANDLE VALUE \n"));
        return FALSE;
    }
    if (clientConfig > QSS_NOTIFICATION_ENABLED)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INVALID CLIENT CONFIG VALUE \n"));
        return FALSE;
    }

    gattQssServerInst = (GQSSS*)ServiceHandleGetInstanceData(srvHndl);

    if (gattQssServerInst == NULL)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INSTANCE NULL POINTER \n"));
        return FALSE;
    }

    gattQssServerSendLosslessAudioConfigAccessRsp(gattQssServerInst,
                                                  cid,
                                                  clientConfig);
    
    return TRUE;
}
