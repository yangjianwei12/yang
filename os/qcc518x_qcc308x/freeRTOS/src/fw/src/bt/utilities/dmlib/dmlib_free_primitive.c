/*******************************************************************************

Copyright (C) 2008 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_free_primitive
 *
 *  DESCRIPTION
 *      Free downstream and upstream DM primitives, including any referenced memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/

void dm_free_primitive(
    DM_UPRIM_T *p_uprim
    )
{
    if (!p_uprim)
        return;

    switch (p_uprim->type)
    {
        case DM_SM_LINK_KEY_REQUEST_RSP:
        case DM_SM_IO_CAPABILITY_REQUEST_RSP:
        case DM_HCI_WRITE_STORED_LINK_KEY_REQ:
        case DM_HCI_CHANGE_LOCAL_NAME_REQ:
        case DM_HCI_WRITE_CURRENT_IAC_LAP_REQ:
        case DM_HCI_WRITE_EXTENDED_INQUIRY_RESPONSE_DATA_REQ:
        case DM_LP_WRITE_ROLESWITCH_POLICY_REQ:
        case DM_SM_ADD_DEVICE_REQ:
#ifdef INSTALL_CONNECTIONLESS_BROADCASTER
        case DM_HCI_SET_CSB_REQ:
#endif /* INSTALL_CONNECTIONLESS_BROADCASTER */

#ifdef INSTALL_ISOC_SUPPORT
        case DM_ISOC_CONFIGURE_CIG_REQ:
        case DM_ISOC_CONFIGURE_CIG_TEST_REQ:
        case DM_ISOC_CIS_CONNECT_REQ:
        case DM_ISOC_BIG_CREATE_SYNC_REQ:
        case DM_ISOC_SETUP_ISO_DATA_PATH_REQ:
#endif /* INSTALL_ISOC_SUPPORT */

        case DM_HCI_ULP_EXT_ADV_SET_DATA_REQ:
        case DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ:
        case DM_HCI_ULP_PERIODIC_ADV_SET_DATA_REQ:
        case DM_VS_COMMAND_REQ:
        case DM_SET_LINK_BEHAVIOR_REQ:
        case DM_LP_WRITE_POWERSTATES_REQ:
        case DM_HCI_CONFIGURE_DATA_PATH_REQ:
        {
            dm_free_downstream_primitive(p_uprim);
            break;
        }

        case DM_HCI_INQUIRY_RESULT_IND:
        case DM_HCI_INQUIRY_RESULT_WITH_RSSI_IND:
        case DM_HCI_RETURN_LINK_KEYS_IND:
        case DM_HCI_REMOTE_NAME_CFM:
        case DM_HCI_READ_LOCAL_NAME_CFM:
        case DM_HCI_READ_CURRENT_IAC_LAP_CFM:
        case DM_HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA_CFM:
        case DM_HCI_EXTENDED_INQUIRY_RESULT_IND:
        case DM_SM_READ_LOCAL_OOB_DATA_CFM:
#ifdef INSTALL_ULP
        case DM_HCI_ULP_ADVERTISING_REPORT_IND:
#ifdef INSTALL_ADVERTISING_EXTENSIONS
        case DM_ULP_EXT_SCAN_FILTERED_ADV_REPORT_IND:
        case DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
#endif /* INSTALL_ADVERTISING_EXTENSIONS */
#endif
        case DM_BAD_MESSAGE_IND:
#ifdef BUILD_FOR_HOST
        case DM_DATA_TO_HCI_IND:
#endif
        case DM_SM_READ_DEVICE_CFM:
        case DM_SM_KEYS_IND:

#ifdef INSTALL_ISOC_SUPPORT
        case DM_ISOC_CREATE_BIG_CFM:
        case DM_ISOC_CREATE_BIG_TEST_CFM:
        case DM_ISOC_BIG_CREATE_SYNC_CFM:
#endif /* INSTALL_ISOC_SUPPORT */

#ifdef INSTALL_CONNECTIONLESS_LISTENER
        case DM_HCI_CSB_RECEIVE_IND:
#endif /* INSTALL_CONNECTIONLESS_LISTENER */
        case DM_VS_COMMAND_CFM:
        case DM_VS_EVENT_IND:
        {
            dm_free_upstream_primitive(p_uprim);
            break;
        }

        default:
        {
            primfree(DM_PRIM, p_uprim);
            break;
        }
    }
}

