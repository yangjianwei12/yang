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
 * \defgroup BAP_BROADCAST_SRC_UTILS BAP BROADCAST
 * @{
 */

#ifndef BAP_BROADCAST_SRC_UTILS_H_
#define BAP_BROADCAST_SRC_UTILS_H_

#include "bap_client_lib.h"
#include "csr_sched.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_SOURCE
void bapBroadcastSrcConfigureStreamCfmSend(phandle_t phandle,
                                           BapProfileHandle handle,
                                           BapResult result);

void bapBroadcastSrcEnableStreamCfmSend(phandle_t phandle,
                                        BapProfileHandle handle,
                                        BapResult result,
                                        uint8 bigId,
                                        uint32 bigSyncDelay,
                                        BapBigParam *bigParameters,
                                        uint8 numBis,
                                        uint16 *bisHandles);

void bapBroadcastSrcEnableStreamTestCfmSend(phandle_t phandle,
                                            BapProfileHandle handle,
                                            BapResult result,
                                            uint8 bigId,
                                            uint32 bigSyncDelay,
                                            BapBigParam *bigTestParameters,
                                            uint8 numBis,
                                            uint16 *bisHandles);

void bapBroadcastSrcDisableStreamCfmSend(phandle_t phandle,
                                         BapProfileHandle handle,
                                         BapResult result,
                                         uint8 bigId);


void bapBroadcastSrcReleaseStreamCfmSend(phandle_t phandle,
                                         BapProfileHandle handle,
                                         BapResult result,
                                         uint8 bigId);


void bapBroadcastSrcUpdateMetadataCfmSend(phandle_t phandle,
                                          BapProfileHandle handle,
                                          BapResult result,
                                          uint8 bigId);


uint8 bapBroadcastSrcGetCodecConfigParam(uint8 *codecConfig,
                                           BapCodecConfiguration *codecParam);

uint8 bapBroadcastSrcGetCodecId(uint8 *codecIdInfo,
                                  BapCodecIdInfo *codecId);


uint8 bapBroadcastSrcGetMetadata(uint8 *metadataInfo,
                                   BapMetadata *metadata);

#endif /* INSTALL_LEA_BROADCAST_SOURCE */
#ifdef __cplusplus
}
#endif

#endif /* BAP_BROADCAST_SRC_UTILS_H_ */

/**@}*/

