/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_hci_ulp_enhanced_transmitter_test_req
 *
 *  DESCRIPTION
 *      Build and send a DM_HCI_ULP_ENHANCED_TRANSMITTER_TEST_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_hci_ulp_enhanced_transmitter_test_req(
    uint8_t tx_channel,
    uint8_t length_test_data,
    uint8_t packet_payload,
    uint8_t phy,
    DM_UPRIM_T **pp_prim
    )
{
    DM_HCI_ULP_ENHANCED_TRANSMITTER_TEST_REQ_T *prim = zpnew(DM_HCI_ULP_ENHANCED_TRANSMITTER_TEST_REQ_T);

    prim->common.op_code = DM_HCI_ULP_ENHANCED_TRANSMITTER_TEST_REQ;
    prim->tx_channel = tx_channel;
    prim->length_test_data = length_test_data;
    prim->packet_payload = packet_payload;
    prim->phy = phy;

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
        DM_PutMsg(prim);
    }
}

