/*******************************************************************************

Copyright (C) 2010 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_free_downstream_primitive
 *
 *  DESCRIPTION
 *      Free downstream DM primitives, including any referenced memory.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_free_downstream_primitive(DM_UPRIM_T *p_uprim)
{
    if (!p_uprim)
        return;

    /* Action taken depends on the primitive type */
    switch (p_uprim->type)
    {
        case DM_SM_LINK_KEY_REQUEST_RSP:
            pfree(p_uprim->dm_sm_link_key_request_rsp.key);
            break;

        case DM_SM_IO_CAPABILITY_REQUEST_RSP:
            pfree(p_uprim->dm_sm_io_capability_request_rsp.oob_hash_c);
            pfree(p_uprim->dm_sm_io_capability_request_rsp.oob_rand_r);
            break;

        case DM_HCI_WRITE_STORED_LINK_KEY_REQ:
            pdestroy_array((void **) p_uprim->dm_write_stored_link_key_req.link_key_bd_addr, HCI_STORED_LINK_KEY_MAX);
            break;

        case DM_HCI_CHANGE_LOCAL_NAME_REQ:
            pdestroy_array((void **) p_uprim->dm_change_local_name_req.name_part, HCI_LOCAL_NAME_BYTE_PACKET_PTRS);
            break;

        case DM_HCI_WRITE_CURRENT_IAC_LAP_REQ:
            pdestroy_array((void **) p_uprim->dm_write_current_iac_lap_req.iac_lap, HCI_IAC_LAP_PTRS);
            break;

        case DM_HCI_WRITE_EXTENDED_INQUIRY_RESPONSE_DATA_REQ:
            pdestroy_array((void **) p_uprim->dm_write_extended_inquiry_response_data_req.eir_data_part, HCI_EIR_DATA_PACKET_PTRS);
            break;

        case DM_LP_WRITE_ROLESWITCH_POLICY_REQ:
            pfree(p_uprim->dm_lp_write_roleswitch_policy_req.rs_table);
            break;

        case DM_SM_ADD_DEVICE_REQ:
            dm_free_sm_keys(p_uprim->dm_sm_add_device_req.keys);
            break;

#ifdef INSTALL_CONNECTIONLESS_BROADCASTER
        case DM_HCI_SET_CSB_DATA_REQ:
            pdestroy_array((void **) p_uprim->dm_set_csb_data_req.data_part, HCI_SET_CSB_DATA_PACKET_PTRS);
            break;
#endif /* INSTALL_CONNECTIONLESS_BROADCASTER*/

#ifdef INSTALL_ISOC_SUPPORT
        case DM_ISOC_CONFIGURE_CIG_REQ:
            pdestroy_array((void **) p_uprim->dm_isoc_configure_cig_req.cis_config,
                            DM_MAX_SUPPORTED_CIS);
            break;

        case DM_ISOC_CONFIGURE_CIG_TEST_REQ:
            pdestroy_array((void **) p_uprim->dm_isoc_configure_cig_test_req.cis_config_test,
                            DM_MAX_SUPPORTED_CIS);
            break;

        case DM_ISOC_CIS_CONNECT_REQ:
            pdestroy_array((void **) p_uprim->dm_isoc_cis_connect_req.cis_conn,
                            DM_MAX_SUPPORTED_CIS);
            break;
#endif /* INSTALL_ISOC_SUPPORT */

        case DM_VS_COMMAND_REQ:
            pdestroy_array((void **) p_uprim->dm_vs_command_req.vs_data_part, HCI_VS_DATA_BYTE_PACKET_PTRS);
            break;

        case DM_HCI_ULP_EXT_ADV_SET_DATA_REQ:
            pdestroy_array((void **) p_uprim->dm_hci_ulp_set_ea_data_req.adv_data, HCI_ULP_ADV_DATA_BYTE_PTRS);
            break;

        case DM_HCI_ULP_EXT_ADV_SET_SCAN_RESP_DATA_REQ:
            pdestroy_array((void **) p_uprim->dm_hci_ulp_set_ea_scan_resp_data_req.scan_resp_data, HCI_ULP_SCAN_RESP_DATA_BYTE_PTRS);
            break;

        case DM_HCI_ULP_PERIODIC_ADV_SET_DATA_REQ:
            pdestroy_array((void **) p_uprim->dm_hci_ulp_set_pa_data_req.adv_data, HCI_ULP_PERIODIC_ADV_DATA_BYTE_PTRS);
            break;

        case DM_ULP_EXT_SCAN_REGISTER_SCANNER_REQ:
            pdestroy_array((void **) p_uprim->dm_ulp_es_register_scanner_req.ad_structure_info, DM_ULP_AD_STRUCT_INFO_BYTE_PTRS);
            break;

        case DM_ULP_PERIODIC_SCAN_START_FIND_TRAINS_REQ:
            pdestroy_array((void **) p_uprim->dm_ulp_ps_start_find_trains_req.ad_structure_info, DM_ULP_AD_STRUCT_INFO_BYTE_PTRS);
            break;

#ifdef INSTALL_ISOC_SUPPORT
        case DM_ISOC_BIG_CREATE_SYNC_REQ:
            pfree(p_uprim->dm_isoc_big_create_sync_req.bis);
            break;

        case DM_ISOC_SETUP_ISO_DATA_PATH_REQ:
            pdestroy_array((void **) p_uprim->dm_isoc_setup_iso_data_path_req.codec_config_data, HCI_CODEC_CONFIG_DATA_PTRS);
            break;
#endif

        case DM_SET_LINK_BEHAVIOR_REQ:
            bpfree(p_uprim->dm_set_link_behavior_req.conftab);
            break;

        case DM_LP_WRITE_POWERSTATES_REQ:
            pfree(p_uprim->dm_lp_write_powerstates_req.states);
            break;

        case DM_HCI_CONFIGURE_DATA_PATH_REQ:
            pdestroy_array((void **) p_uprim->dm_configure_data_path_req.vendor_specific_config, HCI_CONFIGURE_DATA_PATH_PTRS);
            break;

        default:
            break;
    }

    pfree(p_uprim);
}
