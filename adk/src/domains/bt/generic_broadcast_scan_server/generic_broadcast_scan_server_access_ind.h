/*!
   \copyright  Copyright (c) 2023 Qualcomm Technologies International, Ltd.\n
               All Rights Reserved.\n
               Qualcomm Technologies International, Ltd. Confidential and Proprietary.
   \version    
   \addtogroup generic_broadcast_scan_server
   \brief      Header file for the Gaia Broadcast Scan Server module.
   @{
*/

#ifndef GENERIC_GAIA_BROADCAST_SCAN_SERVER_ACCESS_IND_H_
#define GENERIC_GAIA_BROADCAST_SCAN_SERVER_ACCESS_IND_H_

#ifdef INCLUDE_GBSS
#include "generic_broadcast_scan_server_private.h"

#define CLIENT_CONFIG_NOT_SET                 ((gbss_client_config)0x00)
#define CLIENT_CONFIG_NOTIFY                  ((gbss_client_config)0x01)
#define CLIENT_CONFIG_INDICATE                ((gbss_client_config)0x02)
//#define CLIENT_CHARACTERISTIC_CONFIG_OFFSET (1)

void genericBroadcastScanServer_HandleWriteAccessInd(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind);
void genericBroadcastScanServer_HandleReadAccessInd(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessReadInd *access_ind);

void genericBroadcastScanServer_StoreClientConfig(gatt_cid_t cid, void *config, uint8 config_size);

bool genericBroadcastScanServer_ValidateCccLen(gbss_srv_data_t *GBSS, const CsrBtGattDbAccessWriteInd *access_ind);
#endif /* INCLUDE_GBSS */
#endif /* GENERIC_GAIA_BROADCAST_SCAN_SERVER_ACCESS_IND_H_ */

/*! @} */