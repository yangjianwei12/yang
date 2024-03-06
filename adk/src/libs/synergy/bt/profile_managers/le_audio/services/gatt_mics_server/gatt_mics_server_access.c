/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "gatt_mics_server_access.h"
#include "gatt_mics_server_debug.h"
#include "gatt_mics_server_private.h"
#include "gatt_mics_server_db.h"



/***************************************************************************
NAME
    micsHandleMuteCharacteristicAccess

DESCRIPTION
    Deals with access of the HANDLE_PLAYBACK_SPEED handle.
*/

static void micsHandleMuteCharacteristicAccess(
                             GMICS_T *const mics,
                             GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
                             )
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        sendMicsServerAccessRsp(
                mics->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                sizeof(int8),
                (uint8*)(&(mics->data.micsServerMute)));
    }
    else if(accessInd->flags & ATT_ACCESS_WRITE)
    {
        uint8 mute_value = accessInd->value[0];

        /* If Mute is disabled send MICS_ERROR_MUTE_DISABLED error response */

        if(mics->data.micsServerMute == MICS_SERVER_MUTE_DISABLED)
        {
            sendMicsServerAccessErrorRsp(
                        mics->gattId,
                        accessInd->cid,
                        accessInd->handle,
                        MICS_ERROR_MUTE_DISABLED
                        );
            return;
        }

        /* If client writes values  MICS_SERVER_MUTE_DISABLED or RFU send error response*/
        if(mute_value >= MICS_SERVER_MUTE_DISABLED)
        {
            sendMicsServerAccessErrorRsp(
                        mics->gattId,
                        accessInd->cid,
                        accessInd->handle,
                        MICS_ERROR_VALUE_OUT_OF_RANGE
                        );
            return;
        }
        else if(mics->data.micsServerMute == mute_value)
        {
            gattMicsServerWriteGenericResponse(
                        mics->gattId,
                        accessInd->cid,
                        CSR_BT_GATT_ACCESS_RES_SUCCESS,
                        accessInd->handle
                        );
            return;
        }
        else
        {
            GattMicsServerMicStateSetInd *message = (GattMicsServerMicStateSetInd*)
                                        CsrPmemZalloc(sizeof(GattMicsServerMicStateSetInd));

            message->srvcHndl = mics->srvcHandle;
            message->gattId = mics->gattId;
            message->cid = accessInd->cid;
            message->muteValue = mute_value;
            mics->ind_pending = TRUE;

            MicsMessageSend(mics->appTask, GATT_MICS_SERVER_MIC_STATE_SET_IND, message);
        }
    }
    else
    {
        sendMicsServerAccessErrorRsp(
                mics->gattId,
                accessInd->cid,
                accessInd->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}



/***************************************************************************/
static bool micsFindCid(const GMICS_T *mics, connection_id_t cid, uint8 *index)
{
    uint8 i;
    bool res = FALSE;

    for(i=0; i< MICS_MAX_CONNECTIONS; i++)
    {
        if(mics->data.connectedClients[i].cid == cid)
        {
            (*index) = i;
            res = TRUE;
        }
    }
    return res;
}

/***************************************************************************/
void micsHandleAccessIndication(
        GMICS_T *mics,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd
        )
{
    uint8 i;

    if(micsFindCid(mics, accessInd->cid, &i))
    {
        switch (accessInd->handle)
        {

            case HANDLE_MICS_SERVER_MUTE:
            {
                micsHandleMuteCharacteristicAccess(
                                           mics,
                                           accessInd
                                           );
                break;
            }            
            
            case HANDLE_MICS_SERVER_MUTE_CLIENT_CONFIG:
            {
                uint16 clientConfig =
                    GET_MICS_CLIENT_CONFIG(mics->data.connectedClients[i].clientCfg.micsMuteClientCfg);

                if (accessInd->flags & ATT_ACCESS_READ)
                {
                    micsHandleReadClientConfigAccess(
                                mics->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                clientConfig
                                );
                }
                else if (accessInd->flags & ATT_ACCESS_WRITE)
                {
                    clientConfig = 0;
                    micsHandleWriteClientConfigAccess(
                                                mics,
                                                accessInd,
                                                &clientConfig
                                                );
                    mics->data.connectedClients[i].clientCfg.micsMuteClientCfg = MICS_CCC_MASK(clientConfig);
                }
                else
                {
                    sendMicsServerAccessErrorRsp(
                                mics->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                                );
                }
                break;
            }
            default:
            {
                sendMicsServerAccessErrorRsp(
                            mics->gattId,
                            accessInd->cid,
                            accessInd->handle,
                            CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE
                            );
                break;
            }
        } /* switch */
    }
    else
    {
        GATT_MICS_SERVER_ERROR(
                    "GMICS: No valid Cid!\n"
                    );
    }
}

