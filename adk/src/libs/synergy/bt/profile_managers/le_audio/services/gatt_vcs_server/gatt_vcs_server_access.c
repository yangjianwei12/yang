/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "gatt_vcs_server_access.h"
#include "gatt_vcs_server_private.h"

/* Required octets for values sent to Client Configuration Descriptor */
#define CLIENT_CONFIG_VALUE_SIZE                              (2)

/* Required octets for all the possiblie values of Volume Control Point Characteristic */
#define GATT_VCS_SERVER_VOL_CTRL_POINT_SIZE_ABSOLUTE_VOL      (3)
#define GATT_VCS_SERVER_VOL_CTRL_POINT_SIZE_NOT_ABSOLUTE_VOL  (2)

/* Required octets for Volume Flags characteristic */
#define GATT_VCS_SERVER_VOLUME_FLAG_SIZE                      (1)

/* Maximun value for the Volume Setting */
#define GATT_VCS_SERVER_VOLUME_SETTING_VALUE_MAX             (255)

/* Opcode values for the Volume Control Point characteristic */
#define GATT_VCS_SERVER_RELATIVE_VOLUME_DOWN_OPCODE          (0x00)
#define GATT_VCS_SERVER_RELATIVE_VOLUME_UP_OPCODE            (0x01)
#define GATT_VCS_SERVER_UNMUTE_RELATIVE_VOLUME_DOWN_OPCODE   (0x02)
#define GATT_VCS_SERVER_UNMUTE_RELATIVE_VOLUME_UP_OPCODE     (0x03)
#define GATT_VCS_SERVER_SET_ABSOLUTE_VOLUME_OPCODE           (0x04)
#define GATT_VCS_SERVER_UNMUTE_OPCODE                        (0x05)
#define GATT_VCS_SERVER_MUTE_OPCODE                          (0x06)


/***************************************************************************
NAME
    vcsServerHandleVolumeStateAccess

DESCRIPTION
    Deals with access of the HANDLE_VOLUME_STATE handle.
*/
static void vcsServerHandleVolumeStateAccess(
        GVCS *const volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind
        )
{
	uint8* value = NULL;

    if (access_ind->flags & ATT_ACCESS_READ)
    {
       value = (uint8*) CsrPmemAlloc(GATT_VCS_SERVER_VOLUME_STATE_SIZE);

        vcsServerComposeVolumeStateValue(value, volume_control_server);

        vcsServerSendAccessRsp(
                volume_control_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                GATT_VCS_SERVER_VOLUME_STATE_SIZE,
                value
                );
    }
    else
    {
        vcsServerSendAccessErrorRsp(
                volume_control_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************
NAME
    vcsServerSendChangedVolumeStateInd

DESCRIPTION
    Send an indication to the server application and a notification
    to each client has subscribed for it
*/

static void vcsServerSendChangedVolumeStateInd(GVCS *const volume_control_server,
                                               connection_id_t cid)
{
    uint8 i;

    /* Send Indication to the application */
    MAKE_VCS_MESSAGE(
            GattVcsServerVolumeStateInd
            );

    message->vcsServiceHandle = volume_control_server->srvc_hndl;
    message->volumeSetting = volume_control_server->data.volume_setting;
    message->mute = volume_control_server->data.mute;
    message->changeCounter = volume_control_server->data.change_counter;
    message->id = GATT_VCS_SERVER_VOLUME_STATE_IND;
    message->cid = cid;

    VcsServerMessageSend(
            volume_control_server->app_task,
            message);

    for (i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if (volume_control_server->data.connected_clients[i].cid != 0)
        {
            /* If the Client Config is 0x01 (Notify is TRUE), a notification will
             * be sent to the client */
            if (volume_control_server->data.connected_clients[i].client_cfg.volumeStateClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
            {
                uint8* value = (uint8*)CsrPmemAlloc(GATT_VCS_SERVER_VOLUME_STATE_SIZE);

                vcsServerComposeVolumeStateValue(value, volume_control_server);

                vcsServerSendCharacteristicChangedNotification(
                                                     volume_control_server->gattId,
                                                     volume_control_server->data.connected_clients[i].cid,
                                                     HANDLE_VOLUME_STATE,
                                                     GATT_VCS_SERVER_VOLUME_STATE_SIZE,
                                                     value
                                                     );
            }
        }
    }
}

/***************************************************************************
NAME
    vcsServerHandleChangedMuteValue

DESCRIPTION
    Handle the case in which the Mute value is changed
    by the client (writing of the Volume Control Point characteristic)
*/
static void vcsServerHandleChangedMuteValue(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind
        )
{
    /* Check if the change counter is valid */
    if ( access_ind->value[1] == volume_control_server->data.change_counter)
    {
        /* Send the response to the client */
        vcsServerWriteGenericResponse(
                    volume_control_server->gattId,
                    access_ind->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    HANDLE_VOLUME_CONTROL_POINT
                    );

        if ((access_ind->value[0] == GATT_VCS_SERVER_UNMUTE_OPCODE &&
             volume_control_server->data.mute == GATT_VCS_SERVER_MUTE_VALUE) ||
            (access_ind->value[0] == GATT_VCS_SERVER_MUTE_OPCODE &&
             volume_control_server->data.mute == GATT_VCS_SERVER_UNMUTE_VALUE) )
        {
            /* The actual mute value is different from the one the client wants to set */
            /*Increase the change counter of the Volume state characteristic */
            vcsServerHandleChangeCounter(volume_control_server);

            /* Save the new value of mute */
            volume_control_server->data.mute = (!volume_control_server->data.mute);

            /* Inform the application and all the registered clients that the mute value
             * of the Volume State characteristic is changed */
            vcsServerSendChangedVolumeStateInd(volume_control_server, access_ind->cid);
        }
    }
    else
    {
        /* Invalid change counter */
        vcsServerSendAccessErrorRsp(
                    volume_control_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    GATT_VCS_SERVER_ERR_INVALID_CHANGE_COUNTER
                    );
    }
}

/***************************************************************************
NAME
    vcsServerHandleVolCtrlPointOpcodeVolDown

DESCRIPTION
    Deals with the procedure to follow in case of RELATIVE_VOLUME_DOWN_OPCODE
    opcode written to the Volume Control Point characteristic
*/
static void vcsServerHandleVolCtrlPointOpcodeVolDown(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind
        )
{
    uint8 old_change_counter = volume_control_server->data.change_counter;
    uint8 new_volume_setting = 0;
    uint8 mute = volume_control_server->data.mute;

    if(access_ind->value[1] == old_change_counter)
    {
        /* Send the response to the client */
        vcsServerWriteGenericResponse(
                    volume_control_server->gattId,
                    access_ind->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    HANDLE_VOLUME_CONTROL_POINT
                    );

        /* Reduce the value of the Volume Setting by Step Size */
        if (volume_control_server->data.volume_setting >= volume_control_server->data.step_size)
        {
            new_volume_setting = volume_control_server->data.volume_setting - volume_control_server->data.step_size;
        }
        else
        {
            new_volume_setting = 0;
        }

        if (access_ind->value[0] == GATT_VCS_SERVER_UNMUTE_RELATIVE_VOLUME_DOWN_OPCODE &&
            volume_control_server->data.mute == GATT_VCS_SERVER_MUTE_VALUE)
        {
            /* If we are handling the procedure for the Unmute/Relative Volume Down opcode,
             * we have to check if a change of the mute value of the Volume State characteristic
             * is necessary.
             * If the mute value of the Mute is 1 (Mute value), we should change it
             * in 0 (Unmute value).
             */
            mute = GATT_VCS_SERVER_UNMUTE_VALUE;
        }

        if (new_volume_setting != volume_control_server->data.volume_setting ||
            mute != volume_control_server->data.mute)
        {
            /* If there is a change of the Volume State characteristic:
             * increase the change counter ... */
            vcsServerHandleChangeCounter(volume_control_server);

            /* ... save the new value ... */
            volume_control_server->data.volume_setting = new_volume_setting;
            volume_control_server->data.mute = mute;

            /* ... inform the application and the registered clients. */
            vcsServerSendChangedVolumeStateInd(volume_control_server, access_ind->cid);
        }
    }
    else
    {
        /* Invalid change counter */
        vcsServerSendAccessErrorRsp(
                    volume_control_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    GATT_VCS_SERVER_ERR_INVALID_CHANGE_COUNTER
                    );
    }
}

/***************************************************************************
NAME
    vcsServerHandleVolumeControlPointOpcodeVolumeUp

DESCRIPTION
    Deals with the procedure to follow in case of
    GATT_VCS_SERVER_RELATIVE_VOLUME_UP_OPCODE opcode written to the
    Volume Control Point characteristic
*/
static void vcsServerHandleVolumeControlPointOpcodeVolumeUp(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind)
{
    uint8 old_change_counter = volume_control_server->data.change_counter;
    uint8 volume_setting = volume_control_server->data.volume_setting;
    uint8 new_volume_setting = 0;
    uint8 mute = volume_control_server->data.mute;

    if(access_ind->value[1] == old_change_counter)
    {
        /* Check a possible increase of the volume setting of step size */
        if((GATT_VCS_SERVER_VOLUME_SETTING_VALUE_MAX - volume_setting) < volume_control_server->data.step_size)
        {
            /* If adding step_size to volume_setting, obtain a value of volume
             * greater then is maximum, the new value of volume_setting should be set
             * to its maximum value ...
             */
            new_volume_setting = GATT_VCS_SERVER_VOLUME_SETTING_VALUE_MAX;
        }
        else
        {
            /* ... otherwise increase the value of the Volume Setting by Step Size */
            new_volume_setting = volume_setting + volume_control_server->data.step_size;
        }

        if (access_ind->value[0] == GATT_VCS_SERVER_UNMUTE_RELATIVE_VOLUME_UP_OPCODE &&
            volume_control_server->data.mute == GATT_VCS_SERVER_MUTE_VALUE)
        {
            /* If we are hendling the procedure for the Unmute/Relative Volume Up opcode,
             * we have to check if a change of the mute value of the Volume State characteristic
             * is necessary.
             * If the mute value of the Mute is 1 (Mute value), we should change it
             * in 0 (Unmute value).
             */
             mute = GATT_VCS_SERVER_UNMUTE_VALUE;
        }

        /* Send the response to the client */
        vcsServerWriteGenericResponse(
                    volume_control_server->gattId,
                    access_ind->cid,
                    CSR_BT_GATT_ACCESS_RES_SUCCESS,
                    HANDLE_VOLUME_CONTROL_POINT
                    );

        if (new_volume_setting != volume_setting ||
            mute != volume_control_server->data.mute)
        {
            /* A new value of Volume State has to be saved */
            vcsServerHandleChangeCounter(volume_control_server);

            volume_control_server->data.volume_setting = new_volume_setting;
            volume_control_server->data.mute = mute;

            /* We have to inform the app and all the registered clients that
             * Volume State characteristic is changed.
             */
            vcsServerSendChangedVolumeStateInd(volume_control_server, access_ind->cid);
        }
    }
    else
    {
        /* Invalid change counter */
        vcsServerSendAccessErrorRsp(
                    volume_control_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    GATT_VCS_SERVER_ERR_INVALID_CHANGE_COUNTER
                    );
    }
}

/**************************************************************************
NAME
    vcsServerCheckSizeVolCtrlPointCharacteristic

DESCRIPTION
    Check the right size of the Volume Control Point characteristic
    in the indication
*/
static bool vcsServerCheckSizeVolCtrlPointCharacteristic(GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind)
{
    if (((access_ind->value[0] <= GATT_VCS_SERVER_UNMUTE_RELATIVE_VOLUME_UP_OPCODE)) ||
        (access_ind->value[0] == GATT_VCS_SERVER_UNMUTE_OPCODE)                      ||
        (access_ind->value[0] == GATT_VCS_SERVER_MUTE_OPCODE))
    {
        /* Volume Control Point size for all the opcodes except
         * GATT_VCS_SERVER_SET_ABSOLUTE_VOLUME_OPCODE is always 2-octets.
         */
        if ( access_ind->size_value == GATT_VCS_SERVER_VOL_CTRL_POINT_SIZE_NOT_ABSOLUTE_VOL )
        {
            return TRUE;
        }
    }
    else if ((access_ind->value[0] == GATT_VCS_SERVER_SET_ABSOLUTE_VOLUME_OPCODE))
    {
        /* Volume Control Point size for the opcode GATT_VCS_SERVER_SET_ABSOLUTE_VOLUME_OPCODE
         * is always 3-octets.
         */
        if (access_ind->size_value == GATT_VCS_SERVER_VOL_CTRL_POINT_SIZE_ABSOLUTE_VOL)
        {
            return TRUE;
        }
    }
    else
    {
        /* The wrong value of the OPCODE will be checked outside this function */
        return TRUE;
    }

    return FALSE;
}

/**************************************************************************/
void vcsServerSetVolumeFlag(GVCS *volume_control_server)
{
    uint8 i;

    /* Set the Volume Flag to GATT_VCS_SERVER_VOLUME_SETTING_PERSISTED
     * as a consequence of a first change of Volume setting
     */
    /* Indicate the application the Volume Flag is changed */
    MAKE_VCS_MESSAGE(GattVcsServerVolumeFlagInd);

    volume_control_server->data.volume_flag = GATT_VCS_SERVER_VOLUME_SETTING_PERSISTED;

    message->vcsServiceHandle = volume_control_server->srvc_hndl;
    message->volumeFlag = volume_control_server->data.volume_flag;
    message->id = GATT_VCS_SERVER_VOLUME_FLAG_IND;

    VcsServerMessageSend(volume_control_server->app_task, message);

    /* Notify the change of the Volume flag characteristic
     * to all the clients registered for.
     */
    for (i=0; i<GATT_VCS_MAX_CONNECTIONS; i++)
    {
        if (volume_control_server->data.connected_clients[i].cid != 0)
        {
            if (volume_control_server->data.connected_clients[i].client_cfg.volumeFlagClientCfg == GATT_VCS_SERVER_CCC_NOTIFY)
            {
                uint8* value = (uint8*) CsrPmemAlloc(GATT_VCS_SERVER_VOLUME_FLAG_SIZE);
                *value = volume_control_server->data.volume_flag;
                /* The Volume Flags characteristic has been configured for Notification.
                   Send notification to the client */
                vcsServerSendCharacteristicChangedNotification(
                                                     volume_control_server->gattId,
                                                     volume_control_server->data.connected_clients[i].cid,
                                                     HANDLE_VOLUME_FLAGS,
                                                     GATT_VCS_SERVER_VOLUME_FLAG_SIZE,
                                                     value
                                                     );
            }
        }
    }
}

/**************************************************************************
NAME
    vcsServerHandleVolumeControlPointAccess

DESCRIPTION
    Deals with access of the HANDLE_VOLUME_CONTROL_POINT handle.
*/
static void vcsServerHandleVolumeControlPointAccess(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        if (vcsServerCheckSizeVolCtrlPointCharacteristic(access_ind))
        {
            /* Validate the input parameters */
            uint8 opcode = access_ind->value[0];
            uint8 old_volume_setting = volume_control_server->data.volume_setting;

            switch (opcode)
            {
                case GATT_VCS_SERVER_RELATIVE_VOLUME_DOWN_OPCODE:
                case GATT_VCS_SERVER_UNMUTE_RELATIVE_VOLUME_DOWN_OPCODE:
                {
                    vcsServerHandleVolCtrlPointOpcodeVolDown(
                                volume_control_server,
                                access_ind
                                );
                    break;
                }

                case GATT_VCS_SERVER_RELATIVE_VOLUME_UP_OPCODE:
                case GATT_VCS_SERVER_UNMUTE_RELATIVE_VOLUME_UP_OPCODE:
                {
                    vcsServerHandleVolumeControlPointOpcodeVolumeUp(
                                volume_control_server,
                                access_ind
                                );
                    break;
                }

                case GATT_VCS_SERVER_SET_ABSOLUTE_VOLUME_OPCODE:
                {
                    /* Check if the Change counter value is valid */
                    if (access_ind->value[1] == volume_control_server->data.change_counter)
                    {
                        /* Send the response to the client */
                        vcsServerWriteGenericResponse(
                                volume_control_server->gattId,
                                access_ind->cid,
                                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                HANDLE_VOLUME_CONTROL_POINT
                                );

                        if (volume_control_server->data.volume_setting != access_ind->value[2])
                        {
                            /* The absolute volume to set is different from the actual volume */

                            /* Increase change counter */
                            vcsServerHandleChangeCounter(volume_control_server);

                            /* Save the new value of volume setting */
                            volume_control_server->data.volume_setting = access_ind->value[2];

                            /* Inform the application and the registered clients that the Volume State
                             * is changed.
                             */
                            vcsServerSendChangedVolumeStateInd(volume_control_server, access_ind->cid);
                        }
                    }
                    else
                    {
                        /* Invalid change counter */
                        vcsServerSendAccessErrorRsp(
                                    volume_control_server->gattId,
                                    access_ind->cid,
                                    access_ind->handle,
                                    GATT_VCS_SERVER_ERR_INVALID_CHANGE_COUNTER
                                    );
                    }
                    break;
                }

                case GATT_VCS_SERVER_UNMUTE_OPCODE:
                case GATT_VCS_SERVER_MUTE_OPCODE:
                {
                    vcsServerHandleChangedMuteValue(
                                volume_control_server,
                                access_ind
                                );
                    break;
                }

                default:
                {
                    vcsServerSendAccessErrorRsp(
                                volume_control_server->gattId,
                                access_ind->cid,
                                access_ind->handle,
                                GATT_VCS_SERVER_ERR_OPCODE_NOT_SUPPORTED
                                );
                    break;
                }
            }

            if (volume_control_server->data.volume_setting != old_volume_setting &&
                !volume_control_server->data.volume_flag)
            {
                /*It's the first time the volume setting is changed:
                  we need to set the Volume Flag to 1
                */
                vcsServerSetVolumeFlag(volume_control_server);
            }
        }
        else
        {
            /* Invalid length */
            vcsServerSendAccessErrorRsp(
                        volume_control_server->gattId,
                        access_ind->cid,
                        access_ind->handle,
                        CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH
                        );
        }
    }
    else
    {
        /* Volume Control Point characteristic supports only Write */
        vcsServerSendAccessErrorRsp(
            volume_control_server->gattId,
            access_ind->cid,
            access_ind->handle,
            CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
            );
    }
}

/***************************************************************************
NAME
    vcsServerHandleVolumeFlagsAccess

DESCRIPTION
    Deals with access of the HANDLE_VOLUME_FLAGS handle.
*/
static void vcsServerHandleVolumeFlagAccess(
        GVCS *const volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind
        )
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint8* value = (uint8*) CsrPmemAlloc(sizeof(uint8)*GATT_VCS_SERVER_VOLUME_FLAG_SIZE);

        *value = volume_control_server->data.volume_flag;

        GATT_VCS_SERVER_DEBUG("HANDLE_VOLUME_FLAGS: ATT_ACCESS_READ\n");
        vcsServerSendAccessRsp(
                volume_control_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                GATT_VCS_SERVER_VOLUME_FLAG_SIZE,
                value
                );
    }
    else
    {
        GATT_VCS_SERVER_ERROR("HANDLE_VOLUME_FLAGS: CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED\n");
        vcsServerSendAccessErrorRsp(
                volume_control_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
                );
    }
}

/***************************************************************************/
static void vcsServerHandleAccessIndCcc(
                GVCS *volume_control_server,
                GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind)
{
    uint8 index_client = vcsServerGetCidIndex(volume_control_server, access_ind->cid);

    if ((index_client != GATT_VCS_SERVER_INVALID_CID_INDEX) && 
        (access_ind->flags & ATT_ACCESS_READ))
    {
        if (access_ind->handle == HANDLE_VOLUME_STATE_CLIENT_CONFIG)
        {
            vcsServerHandleReadClientConfigAccess(
                    volume_control_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    volume_control_server->data.connected_clients[index_client].client_cfg.volumeStateClientCfg);
        }
        else if (access_ind->handle == HANDLE_VOLUME_FLAGS_CLIENT_CONFIG)
        {
            vcsServerHandleReadClientConfigAccess(
                    volume_control_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    volume_control_server->data.connected_clients[index_client].client_cfg.volumeFlagClientCfg);
        }
    }
    else if ((index_client != GATT_VCS_SERVER_INVALID_CID_INDEX) && 
        (access_ind->flags & ATT_ACCESS_WRITE))
    {
        vcsServerHandleWriteClientConfigAccess(
                    volume_control_server,
                    access_ind);
    }
    else
    {
        vcsServerSendAccessErrorRsp(
                    volume_control_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
}

/***************************************************************************/
void vcsServerHandleAccessIndication(
        GVCS *volume_control_server,
        GATT_MANAGER_SERVER_ACCESS_IND_T *const access_ind
        )
{
    GATT_VCS_SERVER_INFO("Vcs Server Handle Access Ind 0x%x\n",access_ind->handle);
    switch (access_ind->handle)
    {
        case HANDLE_VOLUME_STATE:
        {
            vcsServerHandleVolumeStateAccess(
                    volume_control_server,
                    access_ind
                    );
            break;
        }
        
        case HANDLE_VOLUME_STATE_CLIENT_CONFIG:
        {
            vcsServerHandleAccessIndCcc(volume_control_server, access_ind);
            break;
        }
        
        case HANDLE_VOLUME_CONTROL_POINT:
        {
            vcsServerHandleVolumeControlPointAccess(volume_control_server, access_ind);
            break;
        }
        
        case HANDLE_VOLUME_FLAGS:
        {
            vcsServerHandleVolumeFlagAccess(volume_control_server, access_ind);
            break;
        }
        
        case HANDLE_VOLUME_FLAGS_CLIENT_CONFIG:
        {
            vcsServerHandleAccessIndCcc(volume_control_server, access_ind);
            break;
        }
    }
}
