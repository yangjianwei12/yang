/*!
\copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\brief      Header file for the Broadcast Scan Server volume module.

*/

#ifndef GENERIC_BROADCAST_SCAN_SERVER_VOLUME_H_
#define GENERIC_BROADCAST_SCAN_SERVER_VOLUME_H_

#ifdef INCLUDE_GBSS
#include "generic_broadcast_scan_server_private.h"
#include "generic_broadcast_scan_server_access_ind.h"
#include "audio_sources.h"
#include "volume_messages.h"

#define GENERIC_BROADCAST_SCAN_SERVER_MAX_VOLUME_SETTING_VALUE  0xFF

#define GENERIC_BROADCAST_SCAN_SERVER_ERR_INVALID_CHANGE_COUNTER    (0x80)
#define GENERIC_BROADCAST_SCAN_SERVER_ERR_OPCODE_NOT_SUPPORTED      (0x81)

#define GBSS_DEFAULT_AUDIO_VOLUME 200
#define GBSS_AUDIO_VOLUME_STEP_SIZE 16


void GenericBroadcastScanServer_NotifyGbssVolumeState(void);
const audio_source_observer_interface_t *genericBroadcastScanServer_GetAudioSourceObserverInterface(void);
void genericBroadcastScanServer_HandleWriteGbssVolumeStateCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind);
void genericBroadcastScanServer_HandleWriteGbssVolumeControlPoint(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind);
void genericBroadcastScanServer_HandleReadGbssVolumeState(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind);
void genericBroadcastScanServer_HandleReadGbssVolumeStateCcc(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind);

#endif /* INCLUDE_GBSS */
#endif /* GENERIC_BROADCAST_SCAN_SERVER_VOLUME_H_ */
