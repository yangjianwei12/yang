/*******************************************************************************

Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_free_sm_keys
 *
 *  DESCRIPTION
 *      Free pointers in DM_SM_KEYS_T
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_free_sm_keys(DM_SM_KEYS_T keys)
{
    uint8_t i;
    uint16_t key;

    for (i = 0, key = keys.present;
         i != DM_SM_MAX_NUM_KEYS && key != 0;
         ++i, key >>= DM_SM_NUM_KEY_BITS)
    {
        switch ((DM_SM_KEY_TYPE_T)(key & ((1 << DM_SM_NUM_KEY_BITS) - 1)))
        {
            case DM_SM_KEY_ENC_BREDR:
            case DM_SM_KEY_ENC_CENTRAL:
            case DM_SM_KEY_SIGN:
            case DM_SM_KEY_ID:
                pfree(keys.u[i].none);
                break;

            default:
                break;
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_free_upstream_primitive
 *
 *  DESCRIPTION
 *      Free upstream DM primitives, including any referenced memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_free_upstream_primitive(DM_UPRIM_T *p_uprim)
{
    if(!p_uprim)
    {
        return;
    }
    
    /* Action taken depends on the primitive type */
    switch (p_uprim->type)
    {
#ifndef DISABLE_DM_BREDR
        case DM_HCI_INQUIRY_RESULT_IND:
            pdestroy_array((void **) p_uprim->dm_inquiry_result_ind.result,
                           HCI_MAX_INQ_RESULT_PTRS);
            break;

        case DM_HCI_INQUIRY_RESULT_WITH_RSSI_IND:
            pdestroy_array((void **) p_uprim->dm_inquiry_result_with_rssi_ind.result,
                           HCI_MAX_INQ_RESULT_PTRS);
            break;

        case DM_HCI_RETURN_LINK_KEYS_IND:
            pdestroy_array((void **) p_uprim->dm_return_link_keys_ind.link_key_bd_addr,
                           HCI_STORED_LINK_KEY_MAX);
            break;

        case DM_HCI_REMOTE_NAME_CFM:
            pdestroy_array((void **) p_uprim->dm_remote_name_cfm.name_part,
                           HCI_LOCAL_NAME_BYTE_PACKET_PTRS);
            break;

        case DM_HCI_READ_LOCAL_NAME_CFM:
            pdestroy_array((void **) p_uprim->dm_read_local_name_cfm.name_part,
                           HCI_LOCAL_NAME_BYTE_PACKET_PTRS);
            break;

        case DM_HCI_READ_CURRENT_IAC_LAP_CFM:
            pdestroy_array((void **) p_uprim->dm_read_current_iac_lap_cfm.iac_lap,
                           HCI_IAC_LAP_PTRS);
            break;

        case DM_HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA_CFM:
            pdestroy_array((void **) p_uprim->
                           dm_read_extended_inquiry_response_data_cfm.eir_data_part,
                           HCI_EIR_DATA_PACKET_PTRS);
            break;

        case DM_HCI_EXTENDED_INQUIRY_RESULT_IND:
            pdestroy_array((void **) p_uprim->
                           dm_extended_inquiry_result_ind.eir_data_part,
                           HCI_EIR_DATA_PACKET_PTRS);
            break;
#endif
        case DM_SM_READ_LOCAL_OOB_DATA_CFM:
            pfree(p_uprim->dm_sm_read_local_oob_data_cfm.oob_hash_c);
            pfree(p_uprim->dm_sm_read_local_oob_data_cfm.oob_rand_r);
            break;

        case DM_BAD_MESSAGE_IND:
            pfree(p_uprim->dm_bad_message_ind.message);
            break;

#ifdef BUILD_FOR_HOST
        case DM_DATA_TO_HCI_IND:
            mblk_destroy(p_uprim->dm_data_to_hci_ind.data);
            break;
#endif

        case DM_SM_READ_DEVICE_CFM:
            dm_free_sm_keys(p_uprim->dm_sm_read_device_cfm.keys);
            break;

        case DM_SM_KEYS_IND:
            dm_free_sm_keys(p_uprim->dm_sm_keys_ind.keys);
            break;

#ifdef INSTALL_ULP
        case DM_HCI_ULP_ADVERTISING_REPORT_IND:
            pfree(p_uprim->dm_ulp_advertising_report_ind.data);
            break;

#ifdef INSTALL_ADVERTISING_EXTENSIONS
        case DM_ULP_EXT_SCAN_FILTERED_ADV_REPORT_IND:
            mblk_destroy(p_uprim->dm_ulp_es_filtered_adv_report_ind.adv_data);
            break;

        case DM_ULP_PERIODIC_SCAN_SYNC_ADV_REPORT_IND:
            mblk_destroy(p_uprim->dm_ulp_ps_sync_adv_report_ind.adv_data);
            break;
#endif /* INSTALL_ADVERTISING_EXTENSIONS */
#endif /* INSTALL_ULP */

#ifdef INSTALL_CONNECTIONLESS_LISTENER
        case DM_HCI_CSB_RECEIVE_IND:
            pdestroy_array((void **) p_uprim->dm_csb_received_ind.data_part,
                           HCI_CSB_RECV_PACKET_PTRS);
            break;
#endif /* INSTALL_CONNECTIONLESS_LISTENER */


#ifdef INSTALL_ISOC_SUPPORT
        case DM_ISOC_CREATE_BIG_CFM:
            pfree(p_uprim->dm_isoc_create_big_cfm.bis_handles);
            break;

        case DM_ISOC_CREATE_BIG_TEST_CFM:
            pfree(p_uprim->dm_isoc_create_big_test_cfm.bis_handles);
            break;

        case DM_ISOC_BIG_CREATE_SYNC_CFM:
            pfree(p_uprim->dm_isoc_big_create_sync_cfm.bis_handles);
            break;
#endif
        default:
            break;

    }

    primfree(DM_PRIM, p_uprim);
}
