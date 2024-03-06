/*!
Copyright (c) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file
*/

#include "dmlib_private.h"
#include INC_DIR(dm,dm_sync_manager.h)

/*! \brief Override handler for the given eSCO connection handle.

    Updates the 'handler' in DM_SYNC_T for the given eSCO connection handle.
    Usually, this API is called on a device where the DM_SYNC_T connection 
    stuctures are unmarshalled and the 'handler' is not updated using 
    DM_SYNC_CONNECT_REQ_T primitve.

    \param handle HCI connection handle of eSCO link.
    \param phandle Handle/queue of registration.
    \param pv_cbarg Semi-opaque context of registration.
*/
bool_t dm_sync_override_handler(hci_connection_handle_t handle,
                                phandle_t phandle,
                                context_t pv_cbarg)
{
#ifdef INSTALL_DM_SYNC_MODULE
    DM_INCOMING_SYNC_HANDLER_T *handler;
    DM_SYNC_T *p_sync = dm_sync_find_by_handle(handle, NULL);

    if (p_sync)
    {
        /*  Look for a registered handler matching phandle and pv_cbarg */
        if ((handler = find_handler(phandle, pv_cbarg)) != NULL)
        {
            p_sync->handler = handler;
            return TRUE;
        }
    }
#else
    QBL_UNUSED(handle);
    QBL_UNUSED(phandle);
    QBL_UNUSED(pv_cbarg);
#endif /* INSTALL_DM_SYNC_MODULE */
    return FALSE;
}
