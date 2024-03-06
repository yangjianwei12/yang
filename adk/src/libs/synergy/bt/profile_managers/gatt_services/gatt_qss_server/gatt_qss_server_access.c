/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #3 $
******************************************************************************/


#include "gatt_qss_server_access.h"
#include "gatt_qss_server_msg_handler.h"
#include "csr_bt_gatt_lib.h"

/***************************************************************************
NAME
    gattQssServerSendAccessRsp

DESCRIPTION
    Send an access response to the GATT library.
*/
static void gattQssServerSendAccessRsp(CsrBtGattId gattId,
                                       uint32 btConnId,
                                       uint16 handle,
                                       uint16 result,
                                       uint16 size_value,
                                       uint8* value)
{
    CsrBtGattDbReadAccessResSend(gattId,
                                 btConnId,
                                 handle,
                                 result,
                                 size_value,
                                 value);
}

/***************************************************************************
NAME
    gattQssServerSendAccessErrorRsp

DESCRIPTION
    Send a read access error response to the GATT library.
*/
static void gattQssServerSendAccessErrorRsp(const GQSSS* gattQssServerInst,
                         const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd,
                         uint16 error)
{
    CsrBtGattDbReadAccessResSend(gattQssServerInst->gattId,
                                 accessInd->cid,
                                 accessInd->handle,
                                 error,
                                 0,
                                 NULL);
}

/***************************************************************************
NAME
    gattQssServerSendWriteAccessErrorRsp

DESCRIPTION
    Send a write access error response to the GATT library.
*/
static void gattQssServerSendWriteAccessErrorRsp(const GQSSS* gattQssServerInst,
                              const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd,
                              uint16 error)
{
    CsrBtGattDbWriteAccessResSend(gattQssServerInst->gattId,
                                  accessInd->cid,
                                  accessInd->handle,
                                  error);
}

void gattQssServerHandleQssSupportReadAccess(const GQSSS* gattQssServerInst,
                           const GATT_MANAGER_SERVER_ACCESS_IND_T*accessInd)
{
    /* Send read level message to appTask so it can return the qssSupport */
    MAKE_QSS_MESSAGE(GATT_QSS_SERVER_READ_IND);
    message->cid = accessInd->cid;

    QssMessageSend(gattQssServerInst->appTask,
                   GATT_QSS_SERVER_READ_QSS_SUPPORT_IND,
                   message);
}

void gattQssServerHandleUserDescriptionReadAccess(const GQSSS* gattQssServerInst,
                               const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd)
{
    /* Send read level message to appTask so it can return the user description */
    MAKE_QSS_MESSAGE(GATT_QSS_SERVER_READ_IND);
    message->cid = accessInd->cid;
    message->offset = accessInd->offset;

    QssMessageSend(gattQssServerInst->appTask, 
                   GATT_QSS_SERVER_READ_USER_DESCRIPTION_IND,
                   message);
}

void gattQssServerHandleLosslessAudioReadAccess(const GQSSS* gattQssServerInst,
                             const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd)
{
    /* Send read level message to appTask so it can return the lossless audio */
    MAKE_QSS_MESSAGE(GATT_QSS_SERVER_READ_IND);
    message->cid = accessInd->cid;

    QssMessageSend(gattQssServerInst->appTask,
                   GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_IND,
                   message);
}

void gattQssServerHandleLosslessAudioClientConfigReadAccess(const GQSSS* gattQssServerInst,
                                         const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd)
{
    /* Send read level message to appTask so it can return the lossless audio */
    MAKE_QSS_MESSAGE(GATT_QSS_SERVER_READ_CLIENT_CONFIG_IND);
    message->cid = accessInd->cid;

    QssMessageSend(gattQssServerInst->appTask,
                   GATT_QSS_SERVER_READ_LOSSLESS_AUDIO_CLIENT_CONFIG_IND,
                   message);
}

/***************************************************************************
NAME
    gattQssServerHandleLosslessAudioClientConfigWriteAccess

DESCRIPTION
    Deals with write access of the HANDLE_LOSSLESS_AUDIO_CLIENT_CONFIG handle.
*/
static void gattQssServerHandleLosslessAudioClientConfigWriteAccess(const GQSSS* gattQssServerInst,
                                                 const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd)
{
    if (accessInd->size_value == GATT_QSS_CLIENT_CONFIG_OCTET_SIZE) 
    {
        MAKE_QSS_MESSAGE(GATT_QSS_SERVER_WRITE_CLIENT_CONFIG_IND);
        uint16 conf = accessInd->value[1];
        conf = (conf << 8) | (accessInd->value[0]);
        message->cid = accessInd->cid;
        message->clientConfig = conf;

        QssMessageSend(gattQssServerInst->appTask,
                       GATT_QSS_SERVER_WRITE_LOSSLESS_AUDIO_CLIENT_CONFIG_IND,
                       message);

        CsrBtGattDbWriteAccessResSend(gattQssServerInst->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_SUCCESS);
    }
    else
    {
        gattQssServerSendWriteAccessErrorRsp(gattQssServerInst,
                                             accessInd,
                                             CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH);
    }
}

void gattQssServerSendQssSupportAccessRsp(const GQSSS* gattQssServerInst, 
                             uint32 cid, 
                             uint8 qssSupport,
                             uint16 result)
{
    if (result == CSR_BT_GATT_ACCESS_RES_SUCCESS) 
    {
        uint8* value = (uint8*)CsrPmemAlloc(GATT_QSS_SUPPORT_OCTATE_SIZE);
        CsrMemCpy(value, &qssSupport, GATT_QSS_SUPPORT_OCTATE_SIZE);

        gattQssServerSendAccessRsp(gattQssServerInst->gattId,
                                   cid,
                                   HANDLE_QUALCOMM_SNAPDRAGON_SOUND_SUPPORT,
                                   result,
                                   GATT_QSS_SUPPORT_OCTATE_SIZE,
                                   value);
    }
    else
    {
        gattQssServerSendAccessRsp(gattQssServerInst->gattId,
                                   cid,
                                   HANDLE_QUALCOMM_SNAPDRAGON_SOUND_SUPPORT,
                                   result,
                                   0x00,
                                   NULL);
    }
}

void gattQssServerSendUserDescriptionAccessRsp(const GQSSS* gattQssServerInst,
                                               uint32 cid,
                                               uint16 size,
                                               uint8* userDescription,
                                               uint16 result)
{
    if (result == CSR_BT_GATT_ACCESS_RES_SUCCESS)
    {
        uint8* value = (uint8*)CsrPmemAlloc(size);
        CsrMemCpy(value, userDescription , size);

        gattQssServerSendAccessRsp(gattQssServerInst->gattId,
                                    cid,
                                    HANDLE_USER_DESCRIPTION,
                                    result,
                                    size,
                                    value);
    }
    else
    {
        gattQssServerSendAccessRsp(gattQssServerInst->gattId,
                                    cid,
                                    HANDLE_USER_DESCRIPTION,
                                    result,
                                    0x00,
                                    NULL);
    }
}

void gattQssServerSendLosslessAudioAccessRsp(const GQSSS* gattQssServerInst,
                                             uint32 cid,
                                             uint32 losslessAudio,
                                             uint16 result)
{
    if (result == CSR_BT_GATT_ACCESS_RES_SUCCESS)
    {
        uint8* value = (uint8*)CsrPmemAlloc(GATT_QSS_LOSSLESS_AUDIO_OCTET_SIZE);
        CSR_COPY_UINT32_TO_LITTLE_ENDIAN(losslessAudio, value);

        gattQssServerSendAccessRsp(gattQssServerInst->gattId,
                                    cid,
                                    HANDLE_LOSSLESS_AUDIO,
                                    result,
                                    GATT_QSS_LOSSLESS_AUDIO_OCTET_SIZE,
                                    value);
    }
    else
    {
        gattQssServerSendAccessRsp(gattQssServerInst->gattId, 
                                    cid,
                                    HANDLE_LOSSLESS_AUDIO, result, 0x00, NULL);
    }
}

void gattQssServerSendLosslessAudioConfigAccessRsp(const GQSSS* gattQssServerInst,
                                                   uint32 cid,
                                                   uint16 config)
{
    uint8 configRsp[GATT_QSS_CLIENT_CONFIG_OCTET_SIZE];
    uint8* value = NULL;

    configRsp[0] = config & 0xFF;
    configRsp[1] = (config >> 8) & 0xFF;

    value = (uint8*)CsrPmemAlloc(GATT_QSS_CLIENT_CONFIG_OCTET_SIZE);
    CsrMemCpy(value, configRsp, GATT_QSS_CLIENT_CONFIG_OCTET_SIZE);

    gattQssServerSendAccessRsp(gattQssServerInst->gattId,
                                cid,
                                HANDLE_LOSSLESS_AUDIO_CLIENT_CONFIG,
                                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                GATT_QSS_CLIENT_CONFIG_OCTET_SIZE,
                                value);
}

/***************************************************************************/
void gattQssServerHandleReadAccessIndication(const GQSSS* gattQssServerInst,
                                const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd)
{
    uint16 handle = accessInd->handle;

    switch (handle)
    {
        case HANDLE_QUALCOMM_SNAPDRAGON_SOUND_SUPPORT:
        {
            gattQssServerHandleQssSupportReadAccess(gattQssServerInst,
                                                    accessInd);
            break;
        }

        case HANDLE_USER_DESCRIPTION:
        {
            gattQssServerHandleUserDescriptionReadAccess(gattQssServerInst,
                                                         accessInd);
            break;
        }

        case HANDLE_LOSSLESS_AUDIO:
        {
            gattQssServerHandleLosslessAudioReadAccess(gattQssServerInst,
                                                       accessInd);
            break;
        }

        case HANDLE_LOSSLESS_AUDIO_CLIENT_CONFIG:
        {
            gattQssServerHandleLosslessAudioClientConfigReadAccess(gattQssServerInst,
                                                                   accessInd);
            break;
        }
        default:
        {
            gattQssServerSendAccessErrorRsp(gattQssServerInst, 
                                            accessInd,
                                            CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        }
    }
   
}

/***************************************************************************/
void gattQssServerHandleWriteAccessIndication(const GQSSS* gattQssServerInst,
                            const GATT_MANAGER_SERVER_ACCESS_IND_T* accessInd)
{
    uint16 handle = accessInd->handle;

    switch (handle)
    {
        case HANDLE_LOSSLESS_AUDIO_CLIENT_CONFIG:
        {
            gattQssServerHandleLosslessAudioClientConfigWriteAccess(gattQssServerInst,
                                                                    accessInd);
        }

        default:
        {
            gattQssServerSendWriteAccessErrorRsp(gattQssServerInst,
                                                 accessInd,
                                                 CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        }
    }
}
