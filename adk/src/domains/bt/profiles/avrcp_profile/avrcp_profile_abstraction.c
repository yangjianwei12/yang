/*!
\copyright  Copyright (c) 2022 - 2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      This file abstracts the API calls for Synergy and ADK libraries and 
*           exposes common interface to application. Presently only a subset of the APIs
*           (browsing and metadata) are abstracted however other AVRCP APIs can also be 
*           abstracted in the future.
*/
#ifdef INCLUDE_AV
#include <logging.h>
#include <avrcp.h>
#include <stream.h>
#include "avrcp_profile_abstraction.h"

/***************************** function defs ****************************************/
#ifdef USE_SYNERGY
avrcp_response_type AvrcpProfileAbstract_GetResponseCode(CsrBtAvrcpStatus status)
{
    avrcp_response_type response = avrcp_response_guard_reserved;
    switch (status)
    {
        case CSR_BT_AVRCP_STATUS_ADDR_PLAYER_CHANGED:
            response = avctp_response_changed;
            break;

        case CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE:
            response = avctp_response_interim;
            break;

        default:
            response = avrcp_response_guard_reserved;
    }

    return response;
}

#if (defined INCLUDE_AVRCP_METADATA) || (defined INCLUDE_AVRCP_BROWSING)
static CsrBtAvrcpStatus avrcpProfile_GetStatusCode(avrcp_response_type        response)
{
    CsrBtAvrcpStatus status = CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE;

    switch(response)
    {
        case avctp_response_stable:
        case avctp_response_accepted:
        case avctp_response_interim:
        case avrcp_interim_success:
        case avrcp_response_browsing_success:
            status = CSR_BT_AVRCP_STATUS_OPERATION_COMPLETE;
            break;

        case avrcp_response_rejected_invalid_player_id:
            status = CSR_BT_AVRCP_STATUS_INVALID_PLAYER_ID;
            break;

        case avrcp_response_rejected_invalid_scope:
            status = CSR_BT_AVRCP_STATUS_INVALID_SCOPE;
            break;

        case avrcp_response_rejected_player_not_browsable:
            status = CSR_BT_AVRCP_STATUS_PLAYER_NOT_BROWSABLE;
            break;

        case avrcp_response_rejected_player_not_addressed:
            status = CSR_BT_AVRCP_STATUS_PLAYER_NOT_ADDRESSED;
            break;

        case avrcp_response_rejected_no_valid_search_results:
            status = CSR_BT_AVRCP_STATUS_NO_VALID_SEARCH_RES;
            break;

        case avrcp_response_rejected_no_available_players:
            status = CSR_BT_AVRCP_STATUS_NO_AVAILABLE_PLAYERS;
            break;

        /* Synergy handles completed responses as below */
        case avctp_response_changed:
        case avrcp_response_rejected_addressed_player_changed:
            status = CSR_BT_AVRCP_STATUS_ADDR_PLAYER_CHANGED;
            break;

        case avrcp_response_rejected_invalid_direction:
            status = CSR_BT_AVRCP_STATUS_INVALID_DIRECTION;
            break;

        case avrcp_response_rejected_invalid_content:
            status = CSR_BT_AVRCP_STATUS_INVALID_COMMAND;
            break;

        case avrcp_response_rejected_invalid_param:
            status = CSR_BT_AVRCP_STATUS_INVALID_PARAMETER;
            break;

        case avrcp_response_rejected_invalid_pdu:
            status = CSR_BT_AVRCP_STATUS_PARAMETER_NOT_FOUND;
            break;

        case avrcp_response_rejected_internal_error:
            status = CSR_BT_AVRCP_STATUS_INTERNAL_ERROR;
            break;

        case avrcp_response_rejected_uid_changed:
            status = CSR_BT_AVRCP_STATUS_UID_CHANGED;
            break;

        case avrcp_response_rejected_not_directory:
            status = CSR_BT_AVRCP_STATUS_NOT_A_DIRECTORY;
            break;

        case avrcp_response_rejected_uid_not_exist:
            status = CSR_BT_AVRCP_STATUS_DOES_NOT_EXIST;
            break;

        case avrcp_response_rejected_out_of_bound:
            status = CSR_BT_AVRCP_STATUS_RANGE_OOB;
            break;

        case avrcp_response_rejected_uid_directory:
            status = CSR_BT_AVRCP_STATUS_UID_A_DIRECTORY;
            break;

        case avrcp_response_rejected_media_in_use:
            status = CSR_BT_AVRCP_STATUS_MEDIA_IN_USE;
            break;

        case avrcp_response_rejected_play_list_full:
            status = CSR_BT_AVRCP_STATUS_NPL_FULL;
            break;

        case avrcp_response_rejected_search_not_supported:
            status = CSR_BT_AVRCP_STATUS_SEARCH_NOT_SUPPORTED;
            break;

        case avrcp_response_rejected_search_in_progress:
            status = CSR_BT_AVRCP_STATUS_SEARCH_IN_PROGRESS;
            break;

        case avrcp_response_guard_reserved:
        default:
            status = CSR_BT_AVRCP_STATUS_RESERVED;
            break;
    }

    return status;
}
#endif /* INCLUDE_AVRCP_METADATA || INCLUDE_AVRCP_BROWSING */
#endif /* USE_SYENRGY */


#ifdef INCLUDE_AVRCP_METADATA
void AvrcpProfileAbstract_EventTrackChangedResponse(avInstanceTaskData *the_inst,
                                                avrcp_response_type    response,
                                                uint32                 msgid,
                                                uint32              track_index_high,
                                                uint32              track_index_low)
{
#ifdef USE_SYNERGY
    CsrBtAvrcpStatus status;
    CsrBtAvrcpUid uid;

#ifdef USE_SYNERGY
    if(the_inst == NULL)
    {
        DEBUG_LOG_VERBOSE("AvrcpMetadata_HandleEventTrackChanged av instance cannot be NULL");
        return;
    }
#endif

    status = avrcpProfile_GetStatusCode(response);
    CsrMemCpy(&uid[0], &(track_index_low), CSR_BT_AVRCP_UID_SIZE/2);
    CsrMemCpy(&uid[4], &(track_index_high), CSR_BT_AVRCP_UID_SIZE/2);

    AvrcpTgNotiTrackRes(the_inst->avrcp.connectionId, status, msgid, uid);
#else
    AvrcpEventTrackChangedResponse(the_inst->avrcp.avrcp, response, track_index_high, track_index_low);
    UNUSED(msgid);
#endif
}

void AvrcpProfileAbstract_GetPlayStatusResponse(avInstanceTaskData *the_inst,
                                            avrcp_response_type response,
                                            const AVRCP_GET_PLAY_STATUS_IND_T *ind)
{
#ifdef USE_SYNERGY
    if(the_inst == NULL)
    {
        DEBUG_LOG_VERBOSE("AvrcpProfileAbstract_GetPlayStatusResponse av instance cannot be NULL");
        return;
    }
    AvrcpTgGetPlayStatusResSend(the_inst->avrcp.connectionId,
                        UINT32_MAX, 0, the_inst->avrcp.play_status, ind->msgId, avrcpProfile_GetStatusCode(response));
#else
    AvrcpGetPlayStatusResponse(the_inst->avrcp.avrcp, response,
                        UINT32_MAX, 0, the_inst->avrcp.play_status);
    UNUSED(ind);
#endif
}

void AvrcpProfileAbstract_GetElementAttributesResponse(avInstanceTaskData *the_inst,
                                            avrcp_response_type response,
                                            uint16 num_of_attributes,
                                            uint16 attr_length,
                                            uint8 *attr_data,
                                            TaskData *cleanup_task,
                                            const AVRCP_GET_ELEMENT_ATTRIBUTES_IND_T *ind)
{

#ifdef USE_SYNERGY
    UNUSED(cleanup_task);
    if(the_inst == NULL)
    {
        if(attr_data != NULL)
        {
            free(attr_data);
        }
        DEBUG_LOG_VERBOSE("AvrcpProfileAbstract_GetElementAttributesResponse av instance cannot be NULL");
        return;
    }

    AvrcpTgGetAttributesResSend(the_inst->avrcp.connectionId, num_of_attributes,
                                attr_length, attr_data, ind->msgId, avrcpProfile_GetStatusCode(response));
#else
    Source src_pdu = 0;

    UNUSED(ind);
    /* Create a source from the data */
    src_pdu = StreamRegionSource(attr_data, attr_length);
    MessageStreamTaskFromSink(StreamSinkFromSource(src_pdu), cleanup_task);

    AvrcpGetElementAttributesResponse(the_inst->avrcp.avrcp, response,
        num_of_attributes, attr_length, src_pdu);
#endif
}
#endif /* INCLUDE_AVRCP_METADATA */

#ifdef INCLUDE_AVRCP_BROWSING
void AvrcpProfileAbstract_EventAddressedPlayerChangedResponse(avInstanceTaskData *the_inst,
                            avrcp_response_type response,
                            uint32                 msgid)
{
#ifdef USE_SYNERGY
    AvrcpTgSetAddressedPlayerResSend(the_inst->avrcp.connectionId, 1, 0, msgid, avrcpProfile_GetStatusCode(response));
#else
    AvrcpEventAddressedPlayerChangedResponse(the_inst->avrcp.avrcp, response, 1, 0);
    UNUSED(msgid);
#endif
}

void AvrcpProfileAbstract_BrowseGetFolderItemsResponse(avInstanceTaskData *the_inst,
                                        avrcp_response_type response,
                                        uint16              uid_counter, 
                                        uint16              num_items,  
                                        uint16              item_list_size,
                                        uint8 *item, 
                                        TaskData *cleanup_task,
                                        const AVRCP_BROWSE_GET_FOLDER_ITEMS_IND_T *ind)
{
#ifdef USE_SYNERGY
    UNUSED(cleanup_task);
    AvrcpTgGetFolderItemsResSend(the_inst->avrcp.connectionId, num_items, uid_counter, item_list_size, item, ind->msgId, avrcpProfile_GetStatusCode(response));
#else
    Source src_pdu=0;
    /* Create a source from the data */
    src_pdu = StreamRegionSource(item, item_list_size);
    MessageStreamTaskFromSink(StreamSinkFromSource(src_pdu), cleanup_task);
    AvrcpBrowseGetFolderItemsResponse(the_inst->avrcp.avrcp, response, uid_counter, num_items, item_list_size, src_pdu);
    UNUSED(ind);
#endif
}

void AvrcpProfileAbstract_SetAddressedPlayerResponse(avInstanceTaskData *the_inst,
                                                const AVRCP_SET_ADDRESSED_PLAYER_IND_T *ind)
{
    avrcp_response_type res_type = avrcp_response_rejected_invalid_player_id;
    uint32  player_id;
#ifdef  USE_SYNERGY
    player_id = ind->playerId;
#else
    player_id = ind->player_id;
#endif
   if(player_id != 0xffffu)
   {
       DEBUG_LOG("AvrcpCalSetAddressedPlayerResponse: Request for AVRCP Addressed Played id %d\n", player_id);
       res_type = avctp_response_accepted;
   }

#ifdef USE_SYNERGY
    AvrcpTgSetAddressedPlayerResSend(the_inst->avrcp.connectionId, player_id, 0, ind->msgId, avrcpProfile_GetStatusCode(res_type));
#else
    AvrcpSetAddressedPlayerResponse(the_inst->avrcp.avrcp, res_type);
#endif
}

void AvrcpProfileAbstract_BrowseGetNumberOfItemsResponse( avInstanceTaskData         * the_inst  ,
                                        avrcp_response_type response,
                                        uint16              uid_counter,
                                        uint32              num_items,
                                        const AVRCP_BROWSE_GET_NUMBER_OF_ITEMS_IND_T *ind)

{
#ifdef USE_SYNERGY
    AvrcpTgGetTotalNumberOfItemsResSend(the_inst->avrcp.connectionId, num_items, uid_counter, ind->msgId, avrcpProfile_GetStatusCode(response));
#else
    AvrcpBrowseGetNumberOfItemsResponse(ind->avrcp, response, uid_counter, num_items);
    UNUSED(the_inst);
#endif
}

void AvrcpProfileAbstract_BrowseConnectResponse( avInstanceTaskData *the_inst,
                                 uint16 connection_id,
                                 uint16 signal_id,
                                 bool accept)
{

#ifndef USE_SYNERGY
    AvrcpBrowseConnectResponse(the_inst->avrcp.avrcp, connection_id, signal_id, accept);
#else
    UNUSED(connection_id);
    UNUSED(signal_id);
    UNUSED(accept);
    UNUSED(the_inst);
#endif
}
#endif /* INCLUDE_AVRCP_BROWSING */
#else
static const int compiler_happy;
#endif  /* INCLUDE_AV */

