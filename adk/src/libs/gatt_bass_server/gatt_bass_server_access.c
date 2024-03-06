/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdlib.h>

#include <gatt_manager.h>

#include "gatt_bass_server_access.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_private.h"
#include "gatt_bass_server_common.h"
#include "gatt_bass_server_db.h"


/* Opcode values for the Broadcast Audio Scan Control Point Characteristic */
#define GATT_BASS_REMOTE_SCAN_STOP_OPCODE    (0x00)
#define GATT_BASS_REMOTE_SCAN_START_OPCODE   (0x01)
#define GATT_BASS_ADD_SOURCE_OPCODE          (0x02)
#define GATT_BASS_MODIFY_SOURCE_OPCODE       (0x03)
#define GATT_BASS_SET_BROADCAST_CODE_OPCODE  (0x04)
#define GATT_BASS_REMOVE_SOURCE_OPCODE       (0x05)

/* Size of the Broadcoast Audio Control Point characteristic in case of Remote Scan Stop Opcode */
#define GATT_BASS_REMOTE_SCAN_STOP_OPCODE_SIZE       (1)

/* Size of the Broadcoast Audio Control Point characteristic in case of Remote Scan Start Opcode */
#define GATT_BASS_REMOTE_SCAN_START_OPCODE_SIZE      (GATT_BASS_REMOTE_SCAN_STOP_OPCODE_SIZE)

/* Size of the Broadcoast Audio Control Point characteristic in case of Set Broadcost Code Opcode */
#define GATT_BASS_SET_BROADCAST_CODE_OPCODE_SIZE       (18)

/* Size of the Broadcoast Audio Control Point characteristic in case of Remove Source Code Opcode */
#define GATT_BASS_REMOVE_SOURCE_CODE_OPCODE_SIZE       (2)

/* Possible values of Sync_State in case of the Add Source or Modify Source operation */
#define GATT_BASS_NO_SYNC_PA                           (0x00)
#define GATT_BASS_SYNC_INFO_REQ                        (0x01)
#define GATT_BASS_SYNC_PA                              (0x02)
#define GATT_BASS_FAILED_SYNC_PA                       (0x03)
#define GATT_NO_PAST                                   (0x04)

/***************************************************************************
NAME
    bassServerSendScanningStateInd

DESCRIPTION
    Send an indication to the server application, due to a request
    from the client to start or to stop scanning on behalf of the server
*/
static void bassServerSendScanningStateInd(GBASSSS *bass_server,
                                           connection_id_t cid,
                                           bool client_scanning_state)
{
    MAKE_BASS_MESSAGE(GattBassServerScanningStateInd);

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = cid;
    message->clientScanningState = client_scanning_state;

    MessageSend(bass_server->app_task, GATT_BASS_SERVER_SCANNING_STATE_IND, message);
}

/***************************************************************************
NAME
    bassServerSendBroadcastCodeInd

DESCRIPTION
    Send an indication to the server application, to send a broadcast code
    received from the client for a specific broadcast source.
*/
static void bassServerSendBroadcastCodeInd(GBASSSS *bass_server,
                                           connection_id_t cid,
                                           uint8 source_id,
                                           uint8 index)
{
    MAKE_BASS_MESSAGE(GattBassServerBroadcastCodeInd);

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = cid;
    message->sourceId = source_id;

    memcpy(message->broadcastCode, bass_server->data.broadcast_source[index]->broadcast_code, GATT_BASS_SERVER_BROADCAST_CODE_SIZE);

    MessageSend(bass_server->app_task, GATT_BASS_SERVER_BROADCAST_CODE_IND, message);
}

/***************************************************************************
NAME
    bassServerRemoveSourceInd

DESCRIPTION
    Send an indication to the server application, due to a request from the
    client to remove a broadcast source.
*/
static void bassServerRemoveSourceInd(GBASSSS *bass_server,
                                      connection_id_t cid,
                                      uint8 index_source)
{
    MAKE_BASS_MESSAGE(GattBassServerRemoveSourceInd);

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = cid;
    message->sourceId = bass_server->data.broadcast_source[index_source]->source_id;

    MessageSend(bass_server->app_task, GATT_BASS_SERVER_REMOVE_SOURCE_IND, message);
}

/***************************************************************************/
static void bassClientSetSubgroupsData(uint8 numSubGroups,
                                       uint8 *ptr,
                                       GattBassServerSubGroupsData *subGroupsData)
{
    uint8 i;

    for(i=0; i<numSubGroups; i++)
    {
        subGroupsData[i].bisSync = ((uint32) (*ptr++));
        subGroupsData[i].bisSync |= ((uint32) (*ptr++)) << 8;
        subGroupsData[i].bisSync |= ((uint32) (*ptr++)) << 16;
        subGroupsData[i].bisSync |= ((uint32) (*ptr++)) << 24;

        subGroupsData[i].metadataLen = *(ptr++);

        if(subGroupsData[i].metadataLen)
        {
            subGroupsData[i].metadata = (uint8 *) PanicUnlessMalloc(sizeof(uint8) * subGroupsData[i].metadataLen);
            bassServerSwapByteTransmissionOrder(ptr,
                                                subGroupsData[i].metadataLen,
                                                subGroupsData[i].metadata);
            ptr += subGroupsData[i].metadataLen;
        }
        else
        {
            subGroupsData[i].metadata = NULL;
        }
    }
}

/***************************************************************************
NAME
    bassServerAddSourceHandle

DESCRIPTION
    Handle the Add Source operation
*/
static void bassServerAddSourceHandle(GBASSSS *bass_server,
                                const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    /* The format for the Add Source operation is:
     * value[0] -> 0x02 = Add Source operation
     * value[1] -> Advertising_Address_Type
     * value[2] -> value[7] -> Advertiser_Address
     * value[8] -> Advertising_SID
     * value[9] - value[11] -> Broadcast_ID
     * value[12] -> PA_Sync
     * value[13] - value[14] -> PA_Interval
     * value[15] -> Num_Subgroups
     * value[16] -> BIS_Sync[0]
     * ...
     * value[16+Num_Subgroups-1] -> BIS_Sync[Num_Subgroups-1]
     * value[16+Num_Subgroups] -> Metadata_Length[0]
     * ...
     * value[16+Num_Subgroups+Num_Subgroups-1] -> Metadata_Length[Num_Subgroups-1]
     * value[16+Num_Subgroups+Num_Subgroups] -
                  value[16+Num_Subgroups+Num_Subgroups+Metadata_Length[0]-1]-> Metada[0]
     * ...
     * value[...] - value[...]-> Metada[Num_Subgroups-1]
     * NOTE: Metadata exists only if the Metadata_Length parameter value is ≠ 0x00.
     */
    MAKE_BASS_MESSAGE(GattBassServerAddSourceInd);

    GATT_BASS_SERVER_DEBUG_INFO(("Add Source operation\n"));

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = access_ind->cid;

    message->advertiserAddress.type = access_ind->value[1];
    message->advertiserAddress.addr.nap = (((uint16) access_ind->value[7]) << 8) | ((uint16) access_ind->value[6]);
    message->advertiserAddress.addr.uap = access_ind->value[5];
    message->advertiserAddress.addr.lap = (((uint32) access_ind->value[4]) << 16) |
                                           (((uint32) access_ind->value[3]) << 8) |
                                           ((uint32) access_ind->value[2]);

    message->sourceAdvSid = access_ind->value[8];

    message->broadcastId = access_ind->value[9] |
                           (((uint32) access_ind->value[10]) << 8) |
                           (((uint32) access_ind->value[11]) << 16);

    message->paSync = access_ind->value[12];

    message->paInterval = ((uint16) access_ind->value[13]) |
                          ((uint16) access_ind->value[14] << 8);

    message->numSubGroups = access_ind->value[15];

    if(message->numSubGroups)
    {
        uint8 * ptr = &access_ind->value[16];

        message->subGroupsData = (GattBassServerSubGroupsData *) PanicUnlessMalloc(sizeof(GattBassServerSubGroupsData) * message->numSubGroups);
        bassClientSetSubgroupsData(message->numSubGroups, ptr, message->subGroupsData);
    }

    MessageSend(bass_server->app_task, GATT_BASS_SERVER_ADD_SOURCE_IND, message);
}

/***************************************************************************
NAME
    bassServerModifySourceHandle

DESCRIPTION
    Handle the Modify Source operation
*/
static void bassServerModifySourceHandle(GBASSSS *bass_server,
                                         connection_id_t cid,
                                         uint8 *value)
{
    /* The format for the Modify Source operation is:
     * value[0] -> 0x03 = Modify Source operation
     * value[1] -> Source_ID
     * value[2] -> PA_Sync
     * value[3] - value[5] -> PA_Interval
     * value[5] -> Num_Subgroups
     * value[6] -> BIS_Sync[0]
     * ...
     * value[6+Num_Subgroups-1] -> BIS_Sync[Num_Subgroups-1]
     * value[6+Num_Subgroups] -> Metadata_Length[0]
     * ...
     * value[6+Num_Subgroups+Num_Subgroups-1] -> Metadata_Length[Num_Subgroups-1]
     * value[6+Num_Subgroups+Num_Subgroups] -
                  value[12+Num_Subgroups+Num_Subgroups+Metadata_Length[0]-1]-> Metada[0]
     * ...
     * value[...] - value[...]-> Metada[Num_Subgroups-1]
     * NOTE: Metadata exists only if the Metadata_Length parameter value is ≠ 0x00.
     */
    uint8 index_source;

    GATT_BASS_SERVER_DEBUG_INFO(("Modify Source operation\n"));

    /* Check if the Broadcast Source to modify exists */
    if(bassFindBroadcastSource(bass_server,
                               value[1],
                               &index_source))
    {
        MAKE_BASS_MESSAGE(GattBassServerModifySourceInd);

        message->srvcHndl = bass_server->srvc_hndl;
        message->cid = cid;
        message->sourceId = value[1];
        message->paSyncState = value[2];

        message->paInterval = ((uint16) value[3]) |
                              ((uint16) value[4] << 8);

        message->numSubGroups = value[5];

        if(message->numSubGroups)
        {
            uint8 * ptr = &value[6];

            message->subGroupsData = (GattBassServerSubGroupsData *) PanicUnlessMalloc(sizeof(GattBassServerSubGroupsData) * message->numSubGroups);
            bassClientSetSubgroupsData(message->numSubGroups, ptr, message->subGroupsData);
        }

        MessageSend(bass_server->app_task, GATT_BASS_SERVER_MODIFY_SOURCE_IND, message);
    }
    else
    {
        GATT_BASS_SERVER_DEBUG_INFO(("Error - Modify Source operation: invalid Source ID\n"));
    }
}

/***************************************************************************
NAME
    bassServerSetBroadcastCodeHandle

DESCRIPTION
    Handle the Set Broadcast Code operation
*/
static void bassServerSetBroadcastCodeHandle(GBASSSS *bass_server,
                                             connection_id_t cid,
                                             uint8 *value)
{
    /* The format for the Set Broadcast Code operation is:
     * value[0] -> 0x04 = Set Broadcast_Code operation
     * value[1] -> Source_ID
     * value[2] - value[17] -> Broadcast_Code
     */
    uint8 index_source;
    GATT_BASS_SERVER_DEBUG_INFO(("Set Broadcast Code operation\n"));

    /* Check if the Broadcast Source is present */
    if(bassFindBroadcastSource(bass_server,
                               value[1],
                               &index_source))
    {
        bassServerSwapByteTransmissionOrder(&(value[2]),
                                            GATT_BASS_SERVER_BROADCAST_CODE_SIZE,
                                            bass_server->data.broadcast_source[index_source]->broadcast_code);

        bassServerSendBroadcastCodeInd(bass_server,
                                       cid,
                                       bass_server->data.broadcast_source[index_source]->source_id,
                                       index_source);
    }
    else
    {
       GATT_BASS_SERVER_DEBUG_INFO(("Error - Set Broadcast Code operation: invalid Source ID\n"));
    }
}

/***************************************************************************
NAME
    bassServerRemoveSourceHandle

DESCRIPTION
    Handle the Remove Source operation
*/
static void bassServerRemoveSourceHandle(GBASSSS *bass_server,
                                         connection_id_t cid,
                                         uint8 *value)
{
    /* The format for the Set Broadcast Code operation is:
     * value[0] -> 0x05 = Remove Source operation
     * value[1] -> Source_ID
     */
    uint8 index_source;

    GATT_BASS_SERVER_DEBUG_INFO(("Remove Source operation\n"));

    /* Check if the Broadcast Source is present */
    if(bassFindBroadcastSource(bass_server,
                               value[1],
                               &index_source))
    {
        /* Check if we are in sync to the PA and/or to the BIS/BIG of the
         * Broadcast Source to remove*/
        if(!bassServerIsSynchronized(bass_server, index_source))
        {
            /* Ask to the app to remove a broadcast source */
            bassServerRemoveSourceInd(bass_server,
                                      cid,
                                      index_source);
        }
        else
        {
            GATT_BASS_SERVER_DEBUG_INFO(("Error - Remove Source operation: In sync with the broadcast source\n"));
        }
    }
    else
    {
       GATT_BASS_SERVER_DEBUG_INFO(("Error - Remove Source operation: invalid Source ID\n"));
    }
}

/***************************************************************************/
void static bassClientPrepareBisSyncValuesToValidate(uint8 numSubGroups,
                                                     uint8 *valPtr,
                                                     uint32 *ptr)
{
    uint8 i;

    for(i=0; i<numSubGroups; i++)
    {
        /* Copy first 4 bytes of the first bisSync value */
        memmove(ptr, valPtr, sizeof(uint32));

        /* Move to the next value to copy:
         * 1. we need to skip the 4 bytes we have just copied ..*/
        valPtr += sizeof(uint32);

        /* .. 2. Before the next value of bisSync, there is one byte
         *       for the metadataLen (it's what the pointer is pointing now).
         *       We need to skip the metadata too, so we need to skip
         *       a number of bytes equal to metadataLen
         * */
        valPtr += (*valPtr);


        if (i != (numSubGroups - 1))
        {
            /* There is more values to copy */
            ptr++;
            valPtr += sizeof(uint8);
        }
    }
}

/***************************************************************************/
static bool bassServerIsAddSourceValueValid(uint8 * value)
{
    bool res = FALSE;

    if ((VALID_ADVERTISE_ADDRESS_TYPE(value[1]))        &&
        value[8] <= BASS_SERVER_ADVERTISING_SID_MAX     &&
        (VALID_PA_SYNC_STATE_CNTRL_POINT_OP(value[12])))
    {
        uint8 numSubGroups = value[15];

        if(numSubGroups)
        {
            uint32 * bisSync = (uint32 *) PanicUnlessMalloc(sizeof(uint32) * numSubGroups);
            uint8 *valPtr = &(value[16]);
            uint32 *ptr = bisSync;

            bassClientPrepareBisSyncValuesToValidate(numSubGroups, valPtr, ptr);

            res = bassServerIsValidBisSync(bisSync, numSubGroups);

            free(bisSync);
         }
        else
        {
            res = TRUE;
        }
    }

    return res;
}

/***************************************************************************/
static bool bassServerIsModifySourceValueValid(uint8 * value)
{
    bool res = FALSE;

    if (VALID_PA_SYNC_STATE_CNTRL_POINT_OP(value[2]))
    {
        uint8 numSubGroups = value[5];

        if(numSubGroups)
        {
            uint32 * bisSync = (uint32 *) PanicUnlessMalloc(sizeof(uint32) * numSubGroups);
            uint8 *valPtr = &(value[6]);
            uint32 *ptr = bisSync;

            bassClientPrepareBisSyncValuesToValidate(numSubGroups, valPtr, ptr);

            res = bassServerIsValidBisSync(bisSync, numSubGroups);

            free(bisSync);
         }
        else
        {
            res = TRUE;
        }
    }

    return res;
}

/***************************************************************************
NAME
    broadcastAudioScanControlPointAccess

DESCRIPTION
    Deals with access of the HANDLE_BROADCAST_AUDIO_SCAN_CONTROL_POINT handle.
*/
static void broadcastAudioScanControlPointAccess(GBASSSS *bass_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    uint16 result = gatt_status_success;

    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        uint8 opcode = access_ind->value[0];
        uint16 length = access_ind->size_value;

        switch(opcode)
        {
            case GATT_BASS_REMOTE_SCAN_STOP_OPCODE:
            {
                /* The format for the Scan Stop operation is:
                 * value[0] -> 0x00 = Remote Scan Stop operation
                 */
                if (length == GATT_BASS_REMOTE_SCAN_STOP_OPCODE_SIZE)
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Remote Scan Stop operation\n"));
                    bassServerSendScanningStateInd(bass_server, access_ind->cid, FALSE);
                }
                else
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Remote Scan Stop operation: invalid length\n"));
                    result = gatt_status_write_request_rejected;
                }
            }
            break;

            case GATT_BASS_REMOTE_SCAN_START_OPCODE:
            {
                /* The format for the Scan Start operation is:
                 * value[0] -> 0x01 = Remote Scan Start operation
                 */
                if (length == GATT_BASS_REMOTE_SCAN_START_OPCODE_SIZE)
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Remote Scan Start operation\n"));
                    bassServerSendScanningStateInd(bass_server, access_ind->cid, TRUE);
                }
                else
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Remote Scan Start operation: invalid length\n"));
                    result = gatt_status_write_request_rejected;
                }
            }
            break;

            case GATT_BASS_ADD_SOURCE_OPCODE:
            {
                if (length  >= GATT_BASS_ADD_SOURCE_OPCODE_SIZE &&
                    length == bassServerCalculateAddSourceOpLength(access_ind->value))
                {
                    /* Validate the values of the info of the source to add */
                    if (bassServerIsAddSourceValueValid(access_ind->value))
                    {
                        bassServerAddSourceHandle(bass_server, access_ind);
                    }
                    else
                    {
                        GATT_BASS_SERVER_DEBUG_INFO(("Error - Add Source operation: invalid parameters\n"));
                    }
                }
                else
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Add Source operation: invalid length\n"));
                    result = gatt_status_write_request_rejected;
                }
            }
            break;

            case GATT_BASS_MODIFY_SOURCE_OPCODE:
            {
                if(length < GATT_BASS_MODIFY_SOURCE_OPCODE_SIZE ||
                   length != bassServerCalculateModifySourceOpLength(access_ind->value))
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Modify Source operation: invalid length\n"));
                    result = gatt_status_write_request_rejected;
                    break;
                }

                if(!bassServerIsValidSourceId(bass_server, access_ind->value[1]))
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Modify Source operation: invalid Source_id\n"));
                    result = GATT_BASS_SERVER_ERR_INVALID_SOURCE_ID;
                    break;
                }

                if(bassServerIsModifySourceValueValid(access_ind->value))
                {
                    bassServerModifySourceHandle(bass_server,
                                                 access_ind->cid,
                                                 access_ind->value);
                }
                else
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Modify Source operation: invalid parameters\n"));
                }
            }
            break;

            case GATT_BASS_SET_BROADCAST_CODE_OPCODE:
            {
                if (length != GATT_BASS_SET_BROADCAST_CODE_OPCODE_SIZE)
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Set broadcast Code operation: invalid length\n"));
                    result = gatt_status_write_request_rejected;
                    break;
                }

                if (!bassServerIsValidSourceId(bass_server, access_ind->value[1]))
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Set Broadcast Code operation: invalid Source_id\n"));
                    result = GATT_BASS_SERVER_ERR_INVALID_SOURCE_ID;
                    break;
                }

                bassServerSetBroadcastCodeHandle(bass_server, access_ind->cid, access_ind->value);
            }
            break;

            case GATT_BASS_REMOVE_SOURCE_OPCODE:
            {
                if (length != GATT_BASS_REMOVE_SOURCE_CODE_OPCODE_SIZE)
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Remove Source operation: invalid length\n"));
                    result = gatt_status_write_request_rejected;
                    break;
                }

                if (!bassServerIsValidSourceId(bass_server, access_ind->value[1]))
                {
                    GATT_BASS_SERVER_DEBUG_INFO(("Error - Remove Source operation: invalid Source_id\n"));
                    result = GATT_BASS_SERVER_ERR_INVALID_SOURCE_ID;
                    break;
                }

                bassServerRemoveSourceHandle(bass_server, access_ind->cid, access_ind->value);
            }
            break;

            default:
            {
                GATT_BASS_SERVER_DEBUG_INFO(("Error - Broadcast Audio Scan Control Point: invalid opcode\n"));
                result = GATT_BASS_SERVER_ERR_UNSUPPORTED_OPCODE;
            }
            break;
        }

        sendBassServerAccessRsp((Task) &(bass_server->lib_task),
                                access_ind->cid,
                                access_ind->handle,
                                result,
                                0,
                                NULL);
    }
    else if (access_ind->flags == ATT_ACCESS_PERMISSION)
    {
        sendBassServerAccessRsp((Task) &(bass_server->lib_task),
                                access_ind->cid,
                                access_ind->handle,
                                result,
                                0,
                                NULL);
    }
    else
    {
        sendBassServerAccessErrorRsp(
                        (Task) &bass_server->lib_task,
                        access_ind->cid,
                        access_ind->handle,
                        gatt_status_request_not_supported
                        );
    }
}

/***************************************************************************
NAME
    bassServerBroadcastReceiveStateAccess

DESCRIPTION
    Deals with access of all the HANDLE_BASS_BROADCAST_RECEIVE_STATE handles.
*/
static void bassServerBroadcastReceiveStateAccess(GBASSSS *bass_server,
                                        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                                        uint8 index)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint16 size_value = 0;
        uint8 *value = NULL;

        if(bass_server->data.broadcast_source[index])
        {
            size_value = bassServerCalculateBroadcastReceiveStateCharacteristicLen(bass_server->data.broadcast_source[index]);
            value = PanicUnlessMalloc(sizeof(uint8) * (size_value));
            bassConvertBroadcastReceiveStateValue(bass_server, value, index);
        }

        sendBassServerAccessRsp(bass_server->app_task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_success,
                                size_value,
                                value);

        if(value)
            free(value);
    }
    else
    {
        sendBassServerAccessErrorRsp(
                (Task) &bass_server->lib_task,
                access_ind->cid,
                access_ind->handle,
                gatt_status_request_not_supported);
    }
}

/***************************************************************************/
static bool bassServerFindCid(GBASSSS *bass_server, connection_id_t cid, uint8 *index)
{
    uint8 i;
    bool res = FALSE;

    for(i=0; i<BASS_SERVER_MAX_CONNECTIONS; i++)
    {
        if(bass_server->data.connected_clients[i].cid == cid)
        {
            (*index) = i;
            res = TRUE;
        }
    }

    return res;
}


/***************************************************************************
NAME
    bassServerBroadcastReceiveStateClientConfigAccess

DESCRIPTION
    Deals with access of all the
    HANDLE_BASS_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG handles.
*/
static void bassServerBroadcastReceiveStateClientConfigAccess(
                                        GBASSSS *bass_server,
                                        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                                        uint8 index)
{
    uint8 i;

    if(bassServerFindCid(bass_server, access_ind->cid, &i))
    {
        if (access_ind->flags & ATT_ACCESS_READ)
        {
            bassServerHandleReadClientConfigAccess(
                        (Task) &bass_server->lib_task,
                        access_ind->cid,
                        access_ind->handle,
                        bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[index]);
        }
        else if (access_ind->flags & ATT_ACCESS_WRITE)
        {
            bassServerHandleWriteClientConfigAccess(
                        (Task) &bass_server->lib_task,
                        access_ind,
                        &bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[index]);
        }
        else
        {
            sendBassServerAccessErrorRsp(
                        (Task) &bass_server->lib_task,
                        access_ind->cid,
                        access_ind->handle,
                        gatt_status_request_not_supported);
        }
    }
    else
    {
        sendBassServerAccessErrorRsp(
                    (Task) &bass_server->lib_task,
                    access_ind->cid,
                    access_ind->handle,
                    gatt_status_invalid_cid);
    }
}

/***************************************************************************
NAME
    bassServerGetCharacteristicHandleFromAccessInd

DESCRIPTION
    Get the characteristic handle to use in handleBassServerAccess from the
    real handle in the received access indication.
*/
static uint8 bassServerGetCharacteristicHandleFromAccessInd(
                                             GBASSSS *bass_server,
                                             const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                                             uint16 *handle)
{
    uint8 i;

    /* Offset is the offset between the handles of the Broadcast Receive State characteristics in the ATT database */
    uint8 offset = GATT_BASS_BROADCAST_RECEIVE_STATE_DB_SIZE + GATT_BASS_CLIENT_CONFIG_VALUE_SIZE;

    for(i=0; i<bass_server->data.broadcast_receive_state_num; i++)
    {
        if(access_ind->handle == (HANDLE_BASS_BROADCAST_RECEIVE_STATE_1 + (i * offset)))
        {
            (*handle) = HANDLE_BASS_BROADCAST_RECEIVE_STATE_1;
            break;
        }
        else if (access_ind->handle == (HANDLE_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG_1 + (i * offset)))
        {
            (*handle) =  HANDLE_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG_1;
            break;
        }
    }

    return i;
}

/***************************************************************************/
void handleBassServerAccess(GBASSSS *bass_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind)
{
    uint16 handle = access_ind->handle;
    uint8 index = bassServerGetCharacteristicHandleFromAccessInd(bass_server,
                                                                 access_ind,
                                                                 &handle);

    switch (handle)
    {       
        case HANDLE_BROADCAST_AUDIO_SCAN_CONTROL_POINT:
        {
            broadcastAudioScanControlPointAccess(bass_server, access_ind);
        }
        break;
        
        case HANDLE_BASS_BROADCAST_RECEIVE_STATE_1:
        {
            bassServerBroadcastReceiveStateAccess(bass_server, access_ind, index);
        }
        break;
        
        case HANDLE_BROADCAST_RECEIVE_STATE_CLIENT_CONFIG_1:
        {
            bassServerBroadcastReceiveStateClientConfigAccess(bass_server, access_ind, index);
        }
        break;

        default:
        {
            /* Respond to invalid handles */
           sendBassServerAccessErrorRsp(
                    (Task) &bass_server->lib_task,
                    access_ind->cid,
                    access_ind->handle,
                    gatt_status_invalid_handle);
        }
        break;
    }
}
