/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Assistant interface.
 */

/**
 * \defgroup BAP_BROADCAST_ASSISTANT BAP
 * @{
 */
#ifndef BAP_BROADCAST_ASSISTANT_H_
#define BAP_BROADCAST_ASSISTANT_H_

#include "../bap_client_list_util_private.h"
#include "../bap_connection.h"
#include "bap_broadcast_assistant_private.h"
#include "bap_client_lib.h"
#include "gatt_bass_client.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

void bapBroadcastAssistantInit(BapConnection* connection);

void bapBroadcastAssistantDeinit(BapConnection* connection);

void bapBassClientSrvcInit(BapConnection* connection,
                           CsrBtConnId cid,
                           uint16 startHndl,
                           uint16 endHndl,
                           BapBassClientDeviceData *data);

BroadcastAssistant *bapBroadcastAssistantGetInstFromConn(BapConnection* connection);


void bapBroadcastAssistantUpdateState(BapConnection* connection,
                                      bapAssistantState  assistantState);


uint8 bapBroadcastAssistantGetState(BapConnection* connection);

void bapBroadcastAssistantAddSourceParam(BapConnection* connection, GattBassClientAddSourceParam *params,
	                                     uint16 syncHandle, bool srcCollocated);


void bapBroadcastAssistantModifySourceParam(BapConnection* connection, GattBassClientModifySourceParam *params,
	                                        uint16 syncHandle, bool srcCollocated, uint8 advSid);


void bapBroadcastAssistantRemoveSourceId(BapConnection* connection,
                                         uint16 sourceId);

bool bapBroadcastAssistantValidateBrsParams(BapConnection* connection,
                                            BD_ADDR_T addr,
                                            uint8 advertiseAddType,
                                            uint8 advSid);

bool bapBroadcastAssistantIsSourceIdPending(BapConnection* connection);

void bapBroadcastAssistantSetSourceId(BapConnection* connection,
                                      uint8 sourceId);

void bapBroadcastAssistantSetSourceIdPending(BapConnection* connection);

void bapBroadcastAssistantStartSyncInfoReq(BapConnection* connection);

void bapBroadcastAssistantUpdateControlPointOp(BapConnection* connection,
                                               bool controlOpResponse,
                                               bool longWrite);

void bapBroadcastAssistantGetControlPointOp(BapConnection* connection,
                                            bool *controlOpResponse,
                                            bool *longWrite);

void bapBroadcastAssistantAddSyncParam(BapConnection* connection,
                                       TYPED_BD_ADDR_T sourceAddress,
                                       uint8 advSid);

void bapBroadcastAssistantGetSyncParams(BapConnection* connection,
                                        TYPED_BD_ADDR_T* sourceAddress,
                                        uint8* advSid);

bool bapBroadcastIsSourceCollocated(BapConnection* connection);

uint16 bapBroadcastGetSyncHandle(BapConnection* connection);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT*/
#ifdef __cplusplus
}
#endif

#endif /* BAP_BROADCAST_ASSISTANT_H_ */

/**@}*/


