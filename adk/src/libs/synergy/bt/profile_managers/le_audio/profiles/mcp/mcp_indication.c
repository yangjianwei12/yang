/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */


#include "mcp.h"
#include "mcp_indication.h"
#include "mcp_debug.h"

/***************************************************************************/
static void mcpSendCharacIndCfm(MCP *mcpInst,
                                ServiceHandle srvc_hndl,
                                MediaPlayerAttribute charac,
                                uint16 size_value,
                                uint8 *value)
{
    void* msg = NULL;
    switch (charac)
    {
        case MCS_MEDIA_PLAYER_NAME:
        {
            McpMediaPlayerNameInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_MEDIA_PLAYER_NAME_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;
            message->len      = size_value;

            if (size_value)
            {
                message->name = (uint8*)CsrPmemAlloc(message->len);
                if(message->name)
                {
                    memcpy(message->name, value, size_value);
                }
            }
            else
            {
                message->name = NULL;
            }

            msg = (void*)message;
            break;
        }

        case MCS_TRACK_CHANGED:
        {
            McpTrackChangedInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_TRACK_CHANGED_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            msg = (void*)message;
            break;
        }
        case MCS_TRACK_TITLE:
        {
            McpTrackTitleInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_TRACK_TITLE_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;
            message->len = size_value;

            if (size_value)
            {
                message->trackTitle = (uint8*)CsrPmemAlloc(message->len);
                if(message->trackTitle)
                {
                    memcpy(message->trackTitle, value, size_value);
                }
            }
            else
            {
                message->trackTitle = NULL;
            }

            msg = (void*)message;
            break;
        }
        case MCS_TRACK_DURATION:
        {
            McpTrackDurationInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_TRACK_DURATION_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->trackDuration = *((int32*)value);
            }
            else
            {
                message->trackDuration = 0;
            }

            msg = (void*)message;
            break;
        }
        case MCS_TRACK_POSITION:
        {
            McpTrackPositionInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_TRACK_POSITION_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->trackPosition = *((int32*)value);
            }
            else
            {
                message->trackPosition = 0;
            }

            msg = (void*)message;
            break;
        }
        case MCS_PLAYBACK_SPEED:
        {
            McpPlaybackSpeedInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_PLAYBACK_SPEEED_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->playbackSpeed = (int8)((value[0] & 0xFF));
            }
            else
            {
                message->playbackSpeed = 0;
            }

            msg = (void*)message;
            break;
        }
        case MCS_SEEKING_SPEED:
        {
            McpSeekingSpeedInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_SEEKING_SPEED_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->seekingSpeed = (int8)((value[0] & 0xFF));
            }
            else
            {
                message->seekingSpeed = 0;
            }

            msg = (void*)message;
            break;
        }
        case MCS_PLAYING_ORDER:
        {
            McpPlayingOrderInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_PLAYING_ORDER_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->playingOrder = (uint8)((value[0] & 0xFF));
            }
            else
            {
                message->playingOrder = 0;
            }

            msg = (void*)message;
            break;
        }
        case MCS_MEDIA_STATE:
        {
            McpMediaStateInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_MEDIA_STATE_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->mediaState = (uint8)((value[0] & 0xFF));
            }
            else
            {
                message->mediaState = 0;
            }

            msg = (void*)message;
            break;
        }

        case MCS_MEDIA_CONTROL_POINT_OP_SUPP:
        {
            McpOpcodesSupportedInd* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_SUPPORTED_OPCODES_IND;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->opcodesSupported = *((uint32*)value);
            }
            else
            {
                message->opcodesSupported = 0;
            }

            msg = (void*)message;
            break;
        }
        default:
        {
            MCP_ERROR("Invalid Indication\n");
            return;
        }
    }

    McpMessageSend(mcpInst->appTask, msg);

}

/*******************************************************************************/
void mcpHandleCharacIndCfm(MCP *mcpInst,
                           const GattMcsClientMediaPlayerAttributeInd *msg)
{
    mcpSendCharacIndCfm(mcpInst,
                        msg->srvcHndl,
                        msg->charac,
                        msg->sizeValue,
                        msg->value);
}
