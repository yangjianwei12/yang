/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "mcp_debug.h"
#include "mcp_private.h"
#include "mcp_common.h"
#include "mcp_read.h"
#include "mcp_write.h"
#include "mcp_init.h"

/***************************************************************************/
static void mcpSendReadCharacCfm(MCP *mcpInst,
                                 ServiceHandle srvc_hndl,
                                 MediaPlayerAttribute charac,
                                 McpStatus status,
                                 uint16 size_value,
                                 uint8 *value)
{
    void* msg = NULL;
    switch (charac)
    {
        case MCS_MEDIA_PLAYER_NAME:
        {
            McpGetMediaPlayerNameCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_MEDIA_PLAYER_NAME_CFM;
            message->status   = status;
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

        case MCS_MEDIA_PLAYER_ICON_URL:
        {
            McpGetMediaPlayerIconUrlCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_MEDIA_PLAYER_ICON_URL_CFM;
            message->status = status;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;
            message->len = size_value;

            if (size_value)
            {
                message->iconUrl = (uint8*)CsrPmemAlloc(message->len);
                if(message->iconUrl)
                {
                    memcpy(message->iconUrl, value, size_value);
                }
            }
            else
            {
                message->iconUrl = NULL;
            }

            msg = (void*)message;
            break;
        }
        case MCS_TRACK_TITLE:
        {
            McpGetTrackTitleCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_TRACK_TITLE_CFM;
            message->status = status;
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
            McpGetTrackDurationCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_TRACK_DURATION_CFM;
            message->status = status;
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
            McpGetTrackPositionCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_TRACK_POSITION_CFM;
            message->status = status;
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
            McpGetPlaybackSpeedCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_PLAYBACK_SPEEED_CFM;
            message->status = status;
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
            McpGetSeekingSpeedCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_SEEKING_SPEED_CFM;
            message->status = status;
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
            McpGetPlayinOrderCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_PLAYING_ORDER_CFM;
            message->status = status;
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
        case MCS_PLAYING_ORDER_SUPP:
        {
            McpGetPlayinOrderSupportedCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_PLAYING_ORDER_SUPPORTED_CFM;
            message->status = status;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->playingOrderSupported = *((int16*)value);
            }
            else
            {
                message->playingOrderSupported = 0;
            }

            msg = (void*)message;
            break;
        }
        case MCS_MEDIA_STATE:
        {
            McpGetMediaStateCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_MEDIA_STATE_CFM;
            message->status = status;
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
            McpGetOpcodesSupportedCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_SUPPORTED_OPCODES_CFM;
            message->status = status;
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
        case MCS_CONTENT_CONTROL_ID:
        {
            McpGetContentControlIdCfm* message = CsrPmemAlloc(sizeof(*message));
            message->id = MCP_GET_CONTENT_CONTROL_ID_CFM;
            message->status = status;
            message->prflHndl = mcpInst->mcpSrvcHndl;
            message->srvcHndl = srvc_hndl;

            if (size_value)
            {
                message->ccid = (uint8)((value[0] & 0xFF));
            }
            else
            {
                message->ccid = 0;
            }
            msg = (void*)message;
            break;
        }
        default:
        {
            MCP_ERROR("Invalid Get Operation\n");
            return;
        }
    }

    McpMessageSend(mcpInst->appTask, msg);
}

/*******************************************************************************/
void mcpHandleReadCharacCfm(MCP *mcpInst,
                            const GattMcsClientGetMediaPlayerAttributeCfm *msg)
{
    mcpSendReadCharacCfm(mcpInst,
                         msg->srvcHndl,
                         msg->charac,
                         msg->status,
                         msg->sizeValue,
                         msg->value);
}

/*******************************************************************************/
void McpGetMediaPlayerAttribute(McpProfileHandle profileHandle,
                               ServiceHandle mcsHandle,
                               MediaPlayerAttribute charac)
{
    MCP *mcpInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (mcpInst)
    {
        if (mcsHandle)
        {
            /* Check if the client handle is a valid one */
            if (mcpIsValidMcsInst(mcpInst, mcsHandle))
            {
                GattMcsClientGetMediaPlayerAttribute(mcsHandle, charac);
            }
            else
            {
                mcpSendReadCharacCfm(mcpInst,
                                     mcsHandle,
                                     charac,
                                     CSR_BT_GATT_RESULT_INTERNAL_ERROR,
                                     0,
                                     NULL);
            }
        }
        else
        {
            McpMcsSrvcHndl *ptr = mcpInst->firstMcsSrvcHndl;

            while(ptr)
            {
                GattMcsClientGetMediaPlayerAttribute(ptr->srvcHndl, charac);
                ptr = ptr->next;
            }
        }
    }
    else
    {
        MCP_DEBUG("Invalid profile_handle\n");
    }
}
