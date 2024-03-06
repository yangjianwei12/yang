/*******************************************************************************

Copyright (C) 2010 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

#if !defined(BUILD_FOR_HOST) || defined(QBLUESTACK_HOST)
#ifndef DM_HCI_IFACEQUEUE
/* Define this to a known queue on-chip just to allow us to compile */
#define DM_HCI_IFACEQUEUE DM_IFACEQUEUE
#endif
#endif


/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_data_from_hci_req
 *
 *  DESCRIPTION
 *      Build and send a DM_DATA_FROM_HCI_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
extern void dm_data_from_hci_req(const l2ca_controller_t controller,
                                 const uint8_t physical_handle,
                                 const uint16_t logical_handle,
                                 MBLK_T *data,
                                 DM_UPRIM_T **pp_prim)
{
    DM_DATA_FROM_HCI_REQ_T *prim = pnew(DM_DATA_FROM_HCI_REQ_T);

    prim->type = DM_DATA_FROM_HCI_REQ;
    prim->controller = controller;
    prim->physical_handle = physical_handle;
    prim->logical_handle = logical_handle;
    prim->data = data;

    if (pp_prim == NULL)
        DM_HCI_PutMsg(prim);
    else
        *pp_prim = (DM_UPRIM_T*)prim;
}
