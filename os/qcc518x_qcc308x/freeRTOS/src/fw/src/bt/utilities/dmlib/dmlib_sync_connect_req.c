/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"


/*! \brief Build and send a DM_SYNC_CONNECT_REQ primitive.
    \param phandle Handle/queue of registration.
    \param pv_cbarg Semi-opaque context of registration.
    \param p_bd_addr Pointer to Bluetooth address of remote device.
    \param tx_bdw Required TX Bandwidth.
    \param rx_bdw Required RX Bandwidth.
    \param max_latency Maximum permitted latency.
    \param voice_settings Voice settings.
    \param retx_effort Retransmission effort.
    \param packet_type Permitted packet types.
*/
void dm_sync_connect_req(phandle_t phandle,
                         context_t pv_cbarg,
                         BD_ADDR_T *p_bd_addr,
                         uint32_t tx_bdw,
                         uint32_t rx_bdw,
                         uint16_t max_latency,
                         uint16_t voice_settings,
                         uint8_t retx_effort,
                         hci_pkt_type_t packet_type)
{
    DM_SYNC_CONFIG_T *config;
    DM_SYNC_CONNECT_REQ_T *prim;
   
    prim = pnew(DM_SYNC_CONNECT_REQ_T);
    config = prim->u.config = pnew(DM_SYNC_CONFIG_T);

    config->tx_bdw                  = tx_bdw;
    config->rx_bdw                  = rx_bdw;
    config->max_latency             = max_latency;
    config->voice_settings          = voice_settings;
    config->retx_effort             = retx_effort;
    config->packet_type             = packet_type;

    prim->type                      = DM_SYNC_CONNECT_REQ;
    prim->phandle                   = phandle;
    prim->pv_cbarg                  = pv_cbarg;
    prim->length                    = 0;
    bd_addr_copy(&prim->bd_addr, p_bd_addr);

    DM_PutMsg(prim);

}
