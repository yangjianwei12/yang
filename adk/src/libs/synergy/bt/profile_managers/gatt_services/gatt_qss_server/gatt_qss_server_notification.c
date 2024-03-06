/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #5 $
******************************************************************************/

#include "gatt_qss_server.h"
#include "gatt_qss_server_access.h"
#include "csr_bt_gatt_lib.h"

extern ServiceHandle qssServerServiceHandle;

/****************************************************************************/
bool GattQssServerSendLosslessAudioNotification(ServiceHandle srvHndl,
                                                ConnectionId cid,
                                                uint32 losslessAudio )
{ 
    GQSSS* gattQssServerInst = NULL;
    uint8 losslessMode = (uint8)(losslessAudio >> 24);
    uint8 losslessFormat = (uint8)(losslessAudio >> 16);
    uint8 *value = NULL;

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

    /* If lossless mode is not suppouted then losslessFormat should be 0x00.
     * If lossless mode is supported then losslessFormat could be 0x00 or non-zero value.
    */
    if (losslessMode > LOSSLESS_MODE_SUPPORTED)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: RFU bits are non-zero \n"));
        return FALSE;
    }
    if ((losslessMode == LOSSLESS_MODE_NOT_SUPPORTED) &&
        (losslessFormat != LOSSLESS_FORMAT_UNKNOWN))
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: LOSSLESS_MODE_NOT_SUPPORTED and \
               LOSSLESS FORMAT is not set to LOSSLESS_FORMAT_UNKNOWN \n"));
        return FALSE;
    }
 
    
    gattQssServerInst = (GQSSS*) ServiceHandleGetInstanceData(srvHndl);

    if (gattQssServerInst == NULL)
    {
        GATT_QSS_SERVER_ERROR(("\n GQSSS: INSTANCE NULL POINTER \n"));
        return FALSE;
    }

    value = (uint8*) CsrPmemAlloc(GATT_QSS_LOSSLESS_AUDIO_OCTET_SIZE);

    CSR_COPY_UINT32_TO_LITTLE_ENDIAN(losslessAudio, value);
    CsrBtGattNotificationEventReqSend(gattQssServerInst->gattId,
                                      cid,
                                      HANDLE_LOSSLESS_AUDIO,
                                      GATT_QSS_LOSSLESS_AUDIO_OCTET_SIZE,
                                      value);

    return TRUE;
}
