/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_mcs_client_discovery.h"
#include "gatt_mcs_client_debug.h"
#include "gatt_mcs_client.h"
#include "gatt_mcs_client_uuid.h"
#include "gatt_mcs_client_init.h"


/****************************************************************************/
void handleDiscoverAllMcsCharacteristicsResp(GMCSC *gattMcsClient,
                                             const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTICS_CFM_T *cfm)
{
    GATT_MCS_CLIENT_INFO("GMCSC: DiscoverAllChar Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            switch (cfm->uuid[0])
            {
                case GATT_CHARACTERISTIC_UUID_MEDIA_PLAYER_NAME:
                {
                    gattMcsClient->handles.mediaPlayerNameHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: media_player_name cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_PLAYER_ICON_OBJ_ID:
                {
                    gattMcsClient->handles.mediaPlayerIconObjIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: media_player_icon_obj_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_PLAYER_ICON_URL:
                {
                    gattMcsClient->handles.mediaPlayerIconUrlHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: media_player_icon cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_TRACK_CHANGED:
                {
                    gattMcsClient->handles.trackChangedHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: track_changed cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_TRACK_TITLE:
                {
                    gattMcsClient->handles.trackTitleHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: track_title cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_TRACK_DURATION:
                {
                    gattMcsClient->handles.trackDurationHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: track_duration cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_TRACK_POSITION:
                {
                    gattMcsClient->handles.trackPositionHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: track_duration cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_PLAYBACK_SPEED:
                {
                    gattMcsClient->handles.playbackSpeedHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: playback_speed cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_SEEKING_SPEED:
                {
                    gattMcsClient->handles.seekingSpeedHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: seeking_speed cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_CURRENT_TRACK_SEGMENTS_OBJ_ID:
                {
                    gattMcsClient->handles.currentTrackSegmentsObjIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: current_track_segments_obj_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_CURRENT_TRACK_OBJ_ID:
                {
                    gattMcsClient->handles.currentTrackObjIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: current_track_obj_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_NEXT_TRACK_OBJ_ID:
                {
                    gattMcsClient->handles.nextTrackObjIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: next_track_obj_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_CURRENT_GROUP_OBJ_ID:
                {
                    gattMcsClient->handles.currentGroupObjIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: current_group_obj_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_PARENT_GROUP_OBJ_ID:
                {
                    gattMcsClient->handles.parentGroupObjIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: parent_group_obj_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_PLAYING_ORDER:
                {
                    gattMcsClient->handles.playingOrderHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: playing_order cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_PLAYING_ORDER_SUPPORTED:
                {
                    gattMcsClient->handles.playingOrderSuppHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: playing_order_supp cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_STATE:
                {
                    gattMcsClient->handles.mediaStateHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: media_state cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_CONTROL_POINT:
                {
                    gattMcsClient->handles.mediaControlPointHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: media_control_point cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_CONTROL_POINT_OP_SUPP:
                {
                    gattMcsClient->handles.mediaControlPointOpSuppHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: media_control_point_op_supp cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_SEARCH_RESULTS_OBJ_ID:
                {
                    gattMcsClient->handles.searchResultsObjIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: search_results_obj_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_SEARCH_CONTROL_POINT:
                {
                    gattMcsClient->handles.searchControlPointHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: search_control_point cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                case GATT_CHARACTERISTIC_UUID_CONTENT_CONTROL_ID:
                {
                    gattMcsClient->handles.contentControlIdHandle = cfm->handle;
                    GATT_MCS_CLIENT_DEBUG("GMCSC: content_control_id cid=[0x%x] handle=[0x%x]\n",
                                                    cfm->cid, cfm->handle);
                }
                break;

                default:
                    break;
            }
        }

        if (!cfm->more_to_come)
        {
            if (!gattMcsClient->handles.contentControlIdHandle ||
                !gattMcsClient->handles.mediaPlayerNameHandle ||
                !gattMcsClient->handles.mediaStateHandle||
                !gattMcsClient->handles.trackChangedHandle ||
                !gattMcsClient->handles.trackDurationHandle||
                !gattMcsClient->handles.trackPositionHandle||
                !gattMcsClient->handles.trackTitleHandle)
            {
                /* One of the Mandatory MCS characteristic is not found, initialisation complete */
                gattMcsClientSendInitCfm(gattMcsClient, GATT_MCS_CLIENT_STATUS_DISCOVERY_ERR);
            }
            else
            {
                /* All MCS characteristics found, find the descriptors */
                discoverAllMcsCharacteristicDescriptors(gattMcsClient);
            }
        }
    }
    else
    {
        gattMcsClientSendInitCfm(gattMcsClient, GATT_MCS_CLIENT_STATUS_DISCOVERY_ERR);
    }

}

/****************************************************************************/
void discoverAllMcsCharacteristicDescriptors(GMCSC *gattMcsClient)
{
    CsrBtGattDiscoverAllCharacDescriptorsReqSend(gattMcsClient->srvcElem->gattId,
                                                 gattMcsClient->srvcElem->cid,
                                                 gattMcsClient->handles.startHandle + 1,
                                                 gattMcsClient->handles.endHandle);
}

/****************************************************************************/
void handleDiscoverAllMcsCharacteristicDescriptorsResp(GMCSC *gattMcsClient,
                                                       const GATT_MANAGER_DISCOVER_ALL_CHARACTERISTIC_DESCRIPTORS_CFM_T *cfm)
{
    GATT_MCS_CLIENT_INFO("GMCSC: DiscoverAllDesc Rsp status=[%u] handle=[0x%x] uuid=[0x%lx] more=[%u]\n",
                          cfm->status,
                          cfm->handle,
                          cfm->uuid[0],
                          cfm->more_to_come);

    if (cfm->status == ATT_RESULT_SUCCESS)
    {
        if (cfm->uuid_type == ATT_UUID16)
        {
            switch (cfm->uuid[0])
            {
                case GATT_CHARACTERISTIC_UUID_MEDIA_PLAYER_NAME:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_TRACK_CHANGED:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_TRACK_TITLE:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_TRACK_DURATION:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_TRACK_POSITION:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_PLAYBACK_SPEED:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_SEEKING_SPEED:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_CURRENT_TRACK_OBJ_ID:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_NEXT_TRACK_OBJ_ID:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_CURRENT_GROUP_OBJ_ID:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_PARENT_GROUP_OBJ_ID:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_PLAYING_ORDER:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_STATE:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_CONTROL_POINT:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case GATT_CHARACTERISTIC_UUID_MEDIA_CONTROL_POINT_OP_SUPP:
                {
                    gattMcsClient->pendingHandle = cfm->handle;
                }
                    break;

                case CSR_BT_GATT_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC:
                {

                    if (gattMcsClient->pendingHandle == gattMcsClient->handles.mediaPlayerNameHandle)
                    {
                        gattMcsClient->handles.mediaPlayerNameCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.trackChangedHandle)
                    {
                        gattMcsClient->handles.trackChangedCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.trackTitleHandle)
                    {
                        gattMcsClient->handles.trackTitleCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle  == gattMcsClient->handles.trackDurationHandle)
                    {
                        gattMcsClient->handles.trackDurationCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.trackPositionHandle)
                    {
                        gattMcsClient->handles.trackPositionCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.playbackSpeedHandle)
                    {
                        gattMcsClient->handles.playbackSpeedCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.seekingSpeedHandle)
                    {
                        gattMcsClient->handles.seekingSpeedCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.currentTrackObjIdHandle)
                    {
                        gattMcsClient->handles.currentTrackObjIdCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.nextTrackObjIdHandle)
                    {
                        gattMcsClient->handles.nextTrackObjIdCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.currentGroupObjIdHandle)
                    {
                        gattMcsClient->handles.currentGroupObjIdCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.parentGroupObjIdHandle)
                    {
                        gattMcsClient->handles.parentGroupObjIdCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.playingOrderHandle)
                    {
                        gattMcsClient->handles.playingOrderCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.mediaControlPointOpSuppHandle)
                    {
                        gattMcsClient->handles.mediaControlPointOpSuppCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.mediaControlPointHandle)
                    {
                        gattMcsClient->handles.mediaControlPointCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                    else if (gattMcsClient->pendingHandle == gattMcsClient->handles.mediaStateHandle)
                    {
                        gattMcsClient->handles.mediaStateCccHandle = cfm->handle;
                        gattMcsClient->pendingHandle = 0;
                    }
                }
                break;

                default:
                    break;
            }
        }

        if (!cfm->more_to_come)
        {
            /* One of the Mandatory MCS characteristic's CCCD is not found, initialisation complete */
            if (!gattMcsClient->handles.mediaPlayerNameCccHandle ||
                !gattMcsClient->handles.mediaStateCccHandle ||
                !gattMcsClient->handles.trackChangedCccHandle)
            {
                gattMcsClientSendInitCfm(gattMcsClient, GATT_MCS_CLIENT_STATUS_DISCOVERY_ERR);
            }
            else
            {
                gattMcsClientSendInitCfm(gattMcsClient, GATT_MCS_CLIENT_STATUS_SUCCESS);
            }
         }
    }
    else
    {
        gattMcsClientSendInitCfm(gattMcsClient, GATT_MCS_CLIENT_STATUS_DISCOVERY_ERR);
    }
}

