/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*! \brief Build and send a DM_SYNC_CONNECT_RSP primitive.
    \param p_bd_addr Pointer to Bluetooth address of remote device.
    \param response Acceptance or rejection of connection.
    \param tx_bdw Required TX Bandwidth.
    \param rx_bdw Required RX Bandwidth.
    \param max_latency Maximum permitted latency.
    \param voice_settings Voice settings.
    \param retx_effort Retransmission effort.
    \param packet_type Permitted packet types.
*/
void dm_sync_connect_rsp(BD_ADDR_T *p_bd_addr,
                         uint8_t response,
                         uint32_t tx_bdw,
                         uint32_t rx_bdw,
                         uint16_t max_latency,
                         uint16_t voice_settings,
                         uint8_t retx_effort,
                         hci_pkt_type_t packet_type)
{
    DM_SYNC_CONNECT_RSP_T *prim = pnew(DM_SYNC_CONNECT_RSP_T);

    prim->type                      = DM_SYNC_CONNECT_RSP;
    prim->response                  = response;
    prim->config.tx_bdw             = tx_bdw;
    prim->config.rx_bdw             = rx_bdw;
    prim->config.max_latency        = max_latency;
    prim->config.voice_settings     = voice_settings;
    prim->config.retx_effort        = retx_effort;
    prim->config.packet_type        = packet_type;
    bd_addr_copy(&prim->bd_addr, p_bd_addr);
    DM_PutMsg(prim);
}
