/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include <stdlib.h>

#include "csr_bt_gatt_lib.h"

#include "gatt_bass_server_access.h"
#include "gatt_bass_server_debug.h"
#include "gatt_bass_server_private.h"
#include "gatt_bass_server_common.h"
#include "gatt_bass_server_db.h"

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

/*
 * Offsets of the different fields in the Control Point Operations.
 *
 * These offsets are used to get the values of the correspondent fields
 * in the ATT access indication the library receives in case the client
 * has executed a GATT (long or not) writing to perform
 * one of the possible control point operations.
*/
#define ADD_SOURCE_ADDRESS_TYPE_OFFSET    (2)
#define ADD_SOURCE_SID_OFFSET             (9)
#define ADD_SOURCE_PA_SYNC_STATE_OFFSET   (13)
#define ADD_SOURCE_BIS_SYNC_STATE_OFFSET  (17)

#define MODIFY_SOURCE_ID_OFFSET             (2)
#define MODIFY_SOURCE_PA_SYNC_STATE_OFFSET  (3)
#define MODIFY_SOURCE_BIS_SYNC_STATE_OFFSET (7)

#define SET_BROADCAST_CODE_ID_OFFSET (MODIFY_SOURCE_ID_OFFSET)
#define SET_BROADCAST_CODE_OFFSET    (3)

#define REMOVE_SOURCE_ID_OFFSET (MODIFY_SOURCE_ID_OFFSET)

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
    memset(message, 0, sizeof(GattBassServerScanningStateInd));

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = cid;
    message->clientScanningState = client_scanning_state;

    BassMessageSend(bass_server->app_task, GATT_BASS_SERVER_SCANNING_STATE_IND, message);
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
    memset(message, 0, sizeof(GattBassServerBroadcastCodeInd));

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = cid;
    message->sourceId = source_id;

    memcpy(message->broadcastCode, bass_server->data.broadcast_source[index]->broadcast_code, GATT_BASS_SERVER_BROADCAST_CODE_SIZE);

    BassMessageSend(bass_server->app_task, GATT_BASS_SERVER_BROADCAST_CODE_IND, message);
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
    memset(message, 0, sizeof(GattBassServerRemoveSourceInd));

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = cid;
    message->sourceId = bass_server->data.broadcast_source[index_source]->source_id;

    BassMessageSend(bass_server->app_task, GATT_BASS_SERVER_REMOVE_SOURCE_IND, message);
}

/***************************************************************************/
static void bassClientSetSubgroupsData(uint8 numSubGroups,
                                       uint16 numWriteUnits,
                                       CsrBtGattAttrWritePairs *writeUnit,
                                       uint16 offset,
                                       GattBassServerSubGroupsData *subGroupsData)
{
    uint8 i;

    for(i=0; i<numSubGroups; i++)
    {
        subGroupsData[i].bisSync = ((uint32) bassServerGetElementBuffer(numWriteUnits, writeUnit, offset));
        subGroupsData[i].bisSync |= ((uint32) bassServerGetElementBuffer(numWriteUnits, writeUnit, ++offset)) << 8;
        subGroupsData[i].bisSync |= ((uint32) bassServerGetElementBuffer(numWriteUnits, writeUnit, ++offset)) << 16;
        subGroupsData[i].bisSync |= ((uint32) bassServerGetElementBuffer(numWriteUnits, writeUnit, ++offset)) << 24;

        subGroupsData[i].metadataLen = (uint32) bassServerGetElementBuffer(numWriteUnits, writeUnit, ++offset);

        if(subGroupsData[i].metadataLen)
        {
            uint8 metadataIndex = 0;

            subGroupsData[i].metadata = (uint8 *) CsrPmemAlloc(sizeof(uint8) * subGroupsData[i].metadataLen);

            for(metadataIndex=0; metadataIndex < subGroupsData[i].metadataLen; metadataIndex++)
            {
                subGroupsData[i].metadata[metadataIndex] = bassServerGetElementBuffer(numWriteUnits, writeUnit, ++offset);
            }
        }
        else
        {
            subGroupsData[i].metadata = NULL;
        }

        offset++;
    }
}

/***************************************************************************
NAME
    bassServerAddSourceHandle

DESCRIPTION
    Handle the Add Source operation
*/
static void bassServerAddSourceHandle(GBASSSS *bass_server,
                                      const CsrBtGattAccessInd *access_ind)
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
    uint16 offset = ADD_SOURCE_ADDRESS_TYPE_OFFSET;

    message->srvcHndl = bass_server->srvc_hndl;
    message->cid = access_ind->cid;

    message->advertiserAddress.type = bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                                 access_ind->writeUnit,
                                                                 offset);

    message->advertiserAddress.addr.lap =
            (uint32) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset);
    message->advertiserAddress.addr.lap |=
            ((uint32) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset)) << 8;
    message->advertiserAddress.addr.lap |=
            ((uint32) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset)) << 16;
    message->advertiserAddress.addr.uap =
            bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset);
    message->advertiserAddress.addr.nap =
            (uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset);
    message->advertiserAddress.addr.nap |=
            ((uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset)) << 8;

    message->sourceAdvSid = bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                       access_ind->writeUnit,
                                                       ++offset);

    message->broadcastId =
            (uint32) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset);
    message->broadcastId |=
            ((uint32) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset)) << 8;
    message->broadcastId |=
            ((uint32) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset)) << 16;

    message->paSync = bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                 access_ind->writeUnit,
                                                 ++offset);

    message->paInterval =
            (uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset);
    message->paInterval |=
            ((uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset)) << 8;

    message->numSubGroups = bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                       access_ind->writeUnit,
                                                       ++offset);

    if(message->numSubGroups)
    {
        message->subGroupsData = (GattBassServerSubGroupsData *) CsrPmemAlloc(sizeof(GattBassServerSubGroupsData) * message->numSubGroups);
        bassClientSetSubgroupsData(message->numSubGroups,
                                   access_ind->numWriteUnits,
                                   access_ind->writeUnit,
                                   ++offset,
                                   message->subGroupsData);
    }

    BassMessageSend(bass_server->app_task, GATT_BASS_SERVER_ADD_SOURCE_IND, message);
}

/***************************************************************************
NAME
    bassServerModifySourceHandle

DESCRIPTION
    Handle the Modify Source operation
*/
static void bassServerModifySourceHandle(GBASSSS *bass_server,
                                         const CsrBtGattAccessInd *access_ind)
{
    /* The format for the Modify Source operation is:
     * value[0] -> 0x03 = Modify Source operation
     * value[1] -> Source_ID
     * value[2] -> PA_Sync
     * value[3] - value[4] -> PA_Interval
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
    uint8 index_source = 0;

    /* Check if the Broadcast Source to modify exists */
    if(bassFindBroadcastSource(bass_server,
                               bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                          access_ind->writeUnit,
                                                          MODIFY_SOURCE_ID_OFFSET),
                               &index_source))
    {
        MAKE_BASS_MESSAGE(GattBassServerModifySourceInd);
        uint16 offset = MODIFY_SOURCE_ID_OFFSET;

        message->srvcHndl = bass_server->srvc_hndl;
        message->cid = access_ind->cid;

        message->sourceId = bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                       access_ind->writeUnit,
                                                       offset);
        message->paSyncState = bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                          access_ind->writeUnit,
                                                          ++offset);

        message->paInterval =
                (uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset);
        message->paInterval |=
                (((uint16) bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, ++offset)) << 8);

        message->numSubGroups = bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                           access_ind->writeUnit,
                                                           ++offset);

        if(message->numSubGroups)
        {
            message->subGroupsData = (GattBassServerSubGroupsData *) CsrPmemAlloc(sizeof(GattBassServerSubGroupsData) * message->numSubGroups);
            bassClientSetSubgroupsData(message->numSubGroups,
                                       access_ind->numWriteUnits,
                                       access_ind->writeUnit,
                                       ++offset,
                                       message->subGroupsData);
        }

        BassMessageSend(bass_server->app_task, GATT_BASS_SERVER_MODIFY_SOURCE_IND, message);
    }
    else
    {
        GATT_BASS_SERVER_ERROR("Error - Modify Source operation: invalid Source ID\n");
    }
}

/***************************************************************************
NAME
    bassServerSetBroadcastCodeHandle

DESCRIPTION
    Handle the Set Broadcast Code operation
*/
static void bassServerSetBroadcastCodeHandle(GBASSSS *bass_server,
                                             const CsrBtGattAccessInd *access_ind)
{
    /* The format for the Set Broadcast Code operation is:
     * value[0] -> 0x04 = Set Broadcast_Code operation
     * value[1] -> Source_ID
     * value[2] - value[17] -> Broadcast_Code
     */
    uint8 index_source = 0;

    /* Check if the Broadcast Source is present */
    if (bassFindBroadcastSource(bass_server,
                                bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                           access_ind->writeUnit,
                                                           SET_BROADCAST_CODE_ID_OFFSET),
                                &index_source))
    {
        uint8 i;
        for (i = 0; i < GATT_BASS_SERVER_BROADCAST_CODE_SIZE; i++)
        {
            bass_server->data.broadcast_source[index_source]->broadcast_code[i] =
                bassServerGetElementBuffer(access_ind->numWriteUnits, access_ind->writeUnit, (SET_BROADCAST_CODE_OFFSET + i));
        }

        bassServerSendBroadcastCodeInd(bass_server,
                                       access_ind->cid,
                                       bass_server->data.broadcast_source[index_source]->source_id,
                                       index_source);
    }
    else
    {
        GATT_BASS_SERVER_ERROR("Error - Set Broadcast Code operation: invalid Source ID\n");
    }
}

/***************************************************************************
NAME
    bassServerRemoveSourceHandle

DESCRIPTION
    Handle the Remove Source operation
*/
static void bassServerRemoveSourceHandle(GBASSSS *bass_server,
                                         const CsrBtGattAccessInd *access_ind)
{
    /* The format for the Set Broadcast Code operation is:
     * value[0] -> 0x05 = Remove Source operation
     * value[1] -> Source_ID
     */
    uint8 index_source = 0;

    /* Check if the Broadcast Source is present */
    if(bassFindBroadcastSource(bass_server,
                               bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                          access_ind->writeUnit,
                                                          REMOVE_SOURCE_ID_OFFSET),
                               &index_source))
    {
        /* Check if we are in sync to the PA and/or to the BIS/BIG of the
         * Broadcast Source to remove*/
        if(!bassServerIsSynchronized(bass_server, index_source))
        {
            /* Ask to the app to remove a broadcast source */
            bassServerRemoveSourceInd(bass_server,
                                      access_ind->cid,
                                      index_source);
        }
        else
        {
            GATT_BASS_SERVER_ERROR("Error - Remove Source operation: In sync with the broadcast source\n");
        }
    }
    else
    {
       GATT_BASS_SERVER_ERROR("Error - Remove Source operation: invalid Source ID\n");
    }
}

/***************************************************************************/
void static bassServerPrepareBisSyncValuesToValidate(uint8 numSubGroups,
                                                     uint16 numWriteUnits,
                                                     CsrBtGattAttrWritePairs *writeUnit,
                                                     uint16 offset,
                                                     uint32 *ptr)
{
    uint8 i;

    for(i=0; i<numSubGroups; i++)
    {
        uint8 sizeBysSync = sizeof(uint32);

        /* Copy first sizeof(uint32) bytes of the first bisSync value */
        while(sizeBysSync)
        {
            (*ptr) |= ((uint32) bassServerGetElementBuffer(numWriteUnits, writeUnit, offset)) << ((sizeBysSync-1) * 8);
            sizeBysSync--;
            offset++;
        }

        /* Move to the next value to copy: before the next value of bisSync, there is one byte
         * for the metadataLen and the metadata too
         */
        offset += bassServerGetElementBuffer(numWriteUnits, writeUnit, offset);

        if (i != (numSubGroups - 1))
        {
            /* There is more values to copy */
            ptr++;
            offset++;
        }
    }
}

/***************************************************************************/
static bool bassServerIsAddSourceValueValid(uint16 numWriteUnits,
                                            CsrBtGattAttrWritePairs *writeUnit)
{
    bool res = FALSE;

    if (VALID_ADVERTISE_ADDRESS_TYPE(bassServerGetElementBuffer(numWriteUnits, writeUnit, ADD_SOURCE_ADDRESS_TYPE_OFFSET)) &&
        bassServerGetElementBuffer(numWriteUnits, writeUnit, ADD_SOURCE_SID_OFFSET) <= BASS_SERVER_ADVERTISING_SID_MAX     &&
        VALID_PA_SYNC_STATE_CNTRL_POINT_OP(bassServerGetElementBuffer(numWriteUnits, writeUnit, ADD_SOURCE_PA_SYNC_STATE_OFFSET)))
    {
        uint8 numSubGroups = bassServerGetElementBuffer(numWriteUnits, writeUnit, GATT_BASS_ADD_SOURCE_OPCODE_SIZE);

        if(numSubGroups)
        {
            uint32 * bisSync = (uint32 *) CsrPmemZalloc(sizeof(uint32) * numSubGroups);
            uint32 *ptr = bisSync;

            bassServerPrepareBisSyncValuesToValidate(numSubGroups,
                                                     numWriteUnits,
                                                     writeUnit,
                                                     ADD_SOURCE_BIS_SYNC_STATE_OFFSET,
                                                     ptr);

            res = bassServerIsValidBisSync(bisSync, numSubGroups);

            CsrPmemFree(bisSync);
         }
        else
        {
            res = TRUE;
        }
    }

    return res;
}

/***************************************************************************/
static bool bassServerIsModifySourceValueValid(uint16 numWriteUnits,
                                               CsrBtGattAttrWritePairs *writeUnit)
{
    bool res = FALSE;

    if (VALID_PA_SYNC_STATE_CNTRL_POINT_OP(
                bassServerGetElementBuffer(numWriteUnits, writeUnit, MODIFY_SOURCE_PA_SYNC_STATE_OFFSET)))
    {
        uint8 numSubGroups = bassServerGetElementBuffer(numWriteUnits, writeUnit, GATT_BASS_MODIFY_SOURCE_OPCODE_SIZE);

        if(numSubGroups)
        {
            uint32 * bisSync = (uint32 *) CsrPmemZalloc(sizeof(uint32) * numSubGroups);
            uint32 *ptr = bisSync;

            bassServerPrepareBisSyncValuesToValidate(numSubGroups,
                                                     numWriteUnits,
                                                     writeUnit,
                                                     MODIFY_SOURCE_BIS_SYNC_STATE_OFFSET,
                                                     ptr);

            res = bassServerIsValidBisSync(bisSync, numSubGroups);

            CsrPmemFree(bisSync);
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
static void broadcastAudioScanControlPointAccess(GBASSSS *bass_server,
                                                 const CsrBtGattAccessInd *access_ind)
{
    uint16 result = CSR_BT_GATT_ACCESS_RES_SUCCESS;

    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        uint8 i = 0;
        uint16 length = 0;

        for(; i<access_ind->numWriteUnits; i++)
           length += access_ind->writeUnit[i].valueLength;

        if(length)
        {
            uint8 opcode = access_ind->writeUnit[0].value[0];
            GATT_BASS_SERVER_INFO("Broadcast Audio Scan Control Point Characteristic Opcode: 0x%x\n", opcode);

            switch(opcode)
            {
                case GATT_BASS_REMOTE_SCAN_STOP_OPCODE:
                {
                    /* The format for the Scan Stop operation is:
                     * value[0] -> 0x00 = Remote Scan Stop operation
                     */
                    if (length == GATT_BASS_REMOTE_SCAN_STOP_OPCODE_SIZE)
                    {
                        bassServerSendScanningStateInd(bass_server, access_ind->cid, FALSE);
                    }
                    else
                    {
                        GATT_BASS_SERVER_ERROR("Error - Remote Scan Stop operation: invalid length\n");
                        result = CSR_BT_GATT_ACCESS_RES_WRITE_REQUEST_REJECTED;
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
                        bassServerSendScanningStateInd(bass_server, access_ind->cid, TRUE);
                    }
                    else
                    {
                        GATT_BASS_SERVER_ERROR("Error - Remote Scan Start operation: invalid length\n");
                        result = CSR_BT_GATT_ACCESS_RES_WRITE_REQUEST_REJECTED;
                    }
                    break;
                }

                case GATT_BASS_ADD_SOURCE_OPCODE:
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
                     * NOTE: Metadata exists only if the Metadata_Length parameter value is 0x00.
                    */
                    if (bassServerIsControlPointLengthValid(length,
                                                            GATT_BASS_ADD_SOURCE_OPCODE,
                                                            access_ind->numWriteUnits,
                                                            access_ind->writeUnit))
                    {
                        /* Validate the values of the info of the source to add */
                        if (bassServerIsAddSourceValueValid(access_ind->numWriteUnits,
                                                            access_ind->writeUnit))
                        {
                            bassServerAddSourceHandle(bass_server, access_ind);
                        }
                        else
                        {
                            GATT_BASS_SERVER_ERROR("Error - Add Source operation: invalid parameters\n");
                        }
                    }
                    else
                    {
                        GATT_BASS_SERVER_ERROR("Error - Add Source operation: invalid length\n");
                        result = CSR_BT_GATT_ACCESS_RES_WRITE_REQUEST_REJECTED;
                    }
                }
                break;

                case GATT_BASS_MODIFY_SOURCE_OPCODE:
                {
                    /* The format for the Modify Source operation is:
                     * value[0] -> 0x03 = Modify Source operation
                     * value[1] -> Source_ID
                     * value[2] -> PA_Sync
                     * value[3] - value[4] -> PA_Interval
                     * value[5] -> Num_Subgroups
                     * value[6] -> BIS_Sync[0]
                     * ...
                     * value[6+Num_Subgroups-1] -> BIS_Sync[Num_Subgroups-1]
                     * value[6+Num_Subgroups] -> Metadata_Length[0]
                     * ...
                     * value[6+Num_Subgroups+Num_Subgroups-1] -> Metadata_Length[Num_Subgroups-1]
                     * value[6+Num_Subgroups+Num_Subgroups] -
                                  value[6+Num_Subgroups+Num_Subgroups+Metadata_Length[0]-1]-> Metada[0]
                     * ...
                     * value[...] - value[...]-> Metada[Num_Subgroups-1]
                     * NOTE: Metadata exists only if the Metadata_Length parameter value is ? 0x00.
                    */
                    if(!bassServerIsControlPointLengthValid(length,
                                                            GATT_BASS_MODIFY_SOURCE_OPCODE,
                                                            access_ind->numWriteUnits,
                                                            access_ind->writeUnit))
                    {
                        GATT_BASS_SERVER_ERROR("Error - Modify Source operation: invalid length\n");
                        result = CSR_BT_GATT_ACCESS_RES_WRITE_REQUEST_REJECTED;
                        break;
                    }

                    if(!bassServerIsValidSourceId(bass_server,
                                                  bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                                             access_ind->writeUnit,
                                                                             MODIFY_SOURCE_ID_OFFSET)))
                    {
                        GATT_BASS_SERVER_ERROR("Error - Modify Source operation: invalid Source_id\n");
                        result = GATT_BASS_SERVER_ERR_INVALID_SOURCE_ID;
                        break;
                    }

                    if(bassServerIsModifySourceValueValid(access_ind->numWriteUnits,
                                                          access_ind->writeUnit))
                    {
                        bassServerModifySourceHandle(bass_server,
                                                     access_ind);
                    }
                    else
                    {
                        GATT_BASS_SERVER_ERROR("Error - Modify Source operation: invalid parameters\n");
                    }
                }
                break;

                case GATT_BASS_SET_BROADCAST_CODE_OPCODE:
                {
                    if (length != GATT_BASS_SET_BROADCAST_CODE_OPCODE_SIZE)
                    {
                        GATT_BASS_SERVER_ERROR("Error - Set broadcast Code operation: invalid length\n");
                        result = CSR_BT_GATT_ACCESS_RES_WRITE_REQUEST_REJECTED;
                        break;
                    }

                    if (!bassServerIsValidSourceId(bass_server,
                                                   bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                                              access_ind->writeUnit,
                                                                              SET_BROADCAST_CODE_ID_OFFSET)))
                    {
                        GATT_BASS_SERVER_ERROR("Error - Set Broadcast Code operation: invalid Source_id\n");
                        result = GATT_BASS_SERVER_ERR_INVALID_SOURCE_ID;
                        break;
                    }

                    bassServerSetBroadcastCodeHandle(bass_server, access_ind);
                }
                break;

                case GATT_BASS_REMOVE_SOURCE_OPCODE:
                {
                    if (length != GATT_BASS_REMOVE_SOURCE_CODE_OPCODE_SIZE)
                    {
                        GATT_BASS_SERVER_ERROR("Error - Remove Source operation: invalid length\n");
                        result = CSR_BT_GATT_ACCESS_RES_WRITE_REQUEST_REJECTED;
                        break;
                    }

                    if (!bassServerIsValidSourceId(bass_server,
                                                   bassServerGetElementBuffer(access_ind->numWriteUnits,
                                                                              access_ind->writeUnit,
                                                                              REMOVE_SOURCE_ID_OFFSET)))
                    {
                        GATT_BASS_SERVER_ERROR("Error - Remove Source operation: invalid Source_id\n");
                        result = GATT_BASS_SERVER_ERR_INVALID_SOURCE_ID;
                        break;
                    }

                    bassServerRemoveSourceHandle(bass_server,access_ind);
                }
                break;

                default:
                {
                    GATT_BASS_SERVER_ERROR("Error - Broadcast Audio Scan Control Point: invalid opcode\n");
                    result = GATT_BASS_SERVER_ERR_UNSUPPORTED_OPCODE;
                }
                break;
            }
        }

        sendBassServerAccessRsp(bass_server->gattId,
                                access_ind->cid,
                                access_ind->handle,
                                result,
                                0,
                                NULL);
    }
    else
    {
        sendBassServerAccessErrorRsp(
                        bass_server->gattId,
                        access_ind->cid,
                        access_ind->handle,
                        CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED
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
                                                  const CsrBtGattAccessInd *access_ind,
                                                  uint8 index)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint16 size_value = 0;
        uint8 *value = NULL;

        if(bass_server->data.broadcast_source[index])
        {
            size_value = bassServerCalculateBroadcastReceiveStateCharacteristicLen(bass_server->data.broadcast_source[index]);
            value = CsrPmemAlloc(sizeof(uint8) * (size_value));
            bassConvertBroadcastReceiveStateValue(bass_server, value, index);
        }

        if(access_ind->offset > size_value)
        {
            sendBassServerAccessErrorRsp(
                    bass_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_INVALID_OFFSET);
        }
        else
        {
            if(access_ind->offset == size_value)
            {
                sendBassServerAccessRsp(bass_server->gattId,
                                        access_ind->cid,
                                        access_ind->handle,
                                        CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                        0,
                                        NULL);
            }
            else
            {
                uint16 mtu = CsrBtGattGetMtuFromConnInst((CsrBtConnId) access_ind->cid);
                uint16 sizeToSend = (size_value - access_ind->offset) > (mtu - 1) ?
                            (mtu - 1) : (size_value - access_ind->offset);

                sendBassServerAccessRsp(bass_server->gattId,
                                        access_ind->cid,
                                        access_ind->handle,
                                        CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                        sizeToSend,
                                        &value[access_ind->offset]);
            }
        }

        if(value)
        {
            CsrPmemFree(value);
        }
    }
    else
    {
        sendBassServerAccessErrorRsp(
                bass_server->gattId,
                access_ind->cid,
                access_ind->handle,
                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
    }
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
                                        const CsrBtGattAccessInd *access_ind,
                                        uint8 index)
{
    uint8 i;

    if(bassServerFindCid(bass_server, access_ind->cid, &i))
    {
        if (access_ind->flags & ATT_ACCESS_READ)
        {
            bassServerHandleReadClientConfigAccess(
                        bass_server->gattId,
                        access_ind->cid,
                        access_ind->handle,
                        bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[index]);
        }
        else if (access_ind->flags & ATT_ACCESS_WRITE)
        {
            bassServerHandleWriteClientConfigAccess(
                        bass_server,
                        access_ind,
                        &bass_server->data.connected_clients[i].client_cfg.receiveStateCcc[index]);
        }
        else
        {
            sendBassServerAccessErrorRsp(
                        bass_server->gattId,
                        access_ind->cid,
                        access_ind->handle,
                        CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED);
        }
    }
    else
    {
        sendBassServerAccessErrorRsp(
                    bass_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    BASS_SERVER_STATUS_INVALID_BT_CONN_ID);
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
                                             const CsrBtGattAccessInd *access_ind,
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
void handleBassServerAccess(GBASSSS *bass_server, const CsrBtGattAccessInd *access_ind)
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
                    bass_server->gattId,
                    access_ind->cid,
                    access_ind->handle,
                    CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE);
        }
        break;
    }
}
