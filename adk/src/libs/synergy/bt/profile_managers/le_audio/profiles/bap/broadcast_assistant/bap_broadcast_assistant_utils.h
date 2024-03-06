/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP utility functions interface.
 */

/**
 * \defgroup BAP_BROADCAST_ASSISTANT_UTILS BAP BROADCAST
 * @{
 */

#ifndef BAP_BROADCAST_ASSISTANT_UTILS_H_
#define BAP_BROADCAST_ASSISTANT_UTILS_H_

#include "bap_client_lib.h"
#include "csr_sched.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
void bapBroadcastAssistantUtilsSendReadBroadcastReceiveStateCccCfm(phandle_t phandle,
                                                                   BapProfileHandle handle,
                                                                   BapResult result,
                                                                   uint8 sourceId,
                                                                   uint16 size_value,
                                                                   uint8 *value);

void bapBroadcastAssistantUtilsSendBroadcastReceiveStateSetNtfCfm(phandle_t phandle,
                                                                  BapProfileHandle handle,
                                                                  BapResult result,
                                                                  uint8 sourceId);

void bapBroadcastAssistantUtilsSendBroadcastReceiveStateCfm(phandle_t phandle,
                                                            BapProfileHandle handle,
                                                            BapResult result,
                                                            uint8 sourceId,
                                                            BD_ADDR_T sourceAddress,
                                                            uint8 advertiseAddType,
                                                            uint8 advSid,
                                                            uint8 paSyncState,
                                                            uint8 bigEncryption,
                                                            uint8 *badCode,
                                                            uint8 numSubGroups,
                                                            BapSubgroupInfo *subGroupInfo,
                                                            uint32 broadcastId);

void bapBroadcastAssistantUtilsSendBroadcastReceiveStateInd(phandle_t phandle,
                                                            BapProfileHandle handle,
                                                            uint8 sourceId,
                                                            BD_ADDR_T sourceAddress,
                                                            uint8 advertiseAddType,
                                                            uint8 advSid,
                                                            uint8 paSyncState,
                                                            uint8 bigEncryption,
                                                            uint8* badCode,
                                                            uint8 numSubGroups,
                                                            BapSubgroupInfo *subGroupInfo,
                                                            uint32 broadcastId);

void bapBroadcastAssistantUtilsSendStartScanCfm(phandle_t phandle,
                                                BapProfileHandle handle,
                                                BapResult result,
                                                uint16 scanHandle);

void bapBroadcastAssistantUtilsSendStopScanCfm(phandle_t phandle,
                                               BapProfileHandle handle,
                                               BapResult result);

void bapBroadcastAssistantUtilsSendAddSourceCfm(phandle_t phandle,
                                                BapProfileHandle handle,
                                                BapResult result);

void bapBroadcastAssistantUtilsSendModifySourceCfm(phandle_t phandle,
                                                   BapProfileHandle handle,
                                                   BapResult result);

void bapBroadcastAssistantUtilsSendSetBroadcastCodeInd(phandle_t phandle,
                                                       BapProfileHandle handle,
                                                       uint8 sourceId,
                                                       uint8 flags);

void bapBroadcastAssistantUtilsSendRemoveSourceCfm(phandle_t phandle,
                                                   BapProfileHandle handle,
                                                   BapResult result);

void bapBroadcastAssistantUtilsSendControlPointOpCfm(phandle_t phandle,
                                                     BapProfileHandle handle,
                                                     BapResult result);

void bapBroadcastAssistantUtilsSendSyncToSrcStartCfm(phandle_t phandle,
                                                     BapProfileHandle handle,
                                                     uint16 syncHandle,
                                                     uint8 advSid,
                                                     uint8 advClockAccuracy,
                                                     uint8 advPhy,
                                                     uint16 periodicAdvInterval,
                                                     TYPED_BD_ADDR_T  addrt,
                                                     BapResult result);

void bapBroadcastAssistantUtilsSendSyncToSrcCancelCfm(phandle_t phandle,
                                                      BapProfileHandle handle,
                                                      BapResult result);

void bapBroadcastAssistantUtilsSendSyncToSrcTerminateCfm(phandle_t phandle,
                                                         BapProfileHandle handle,
                                                         uint16 syncHandle,
                                                         BapResult result);

void bapBroadcastAssistantUtilsSendSyncLossInd(phandle_t phandle,
                                               BapProfileHandle handle,
                                               uint16 syncHandle);
#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#ifdef __cplusplus
}
#endif

#endif /* BAP_BROADCAST_ASSISTANT_UTILS_H_ */

/**@}*/

