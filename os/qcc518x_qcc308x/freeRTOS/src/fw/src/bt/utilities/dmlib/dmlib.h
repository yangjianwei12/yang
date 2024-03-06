/*******************************************************************************

Copyright (C) 2007 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

(C) COPYRIGHT Cambridge Consultants Ltd 1999

DESCRIPTION:       Device Manager access library - provides functions for building and sending
                   downstream DM primitives.

REVISION:          $Revision: #14 $
*******************************************************************************/
#ifndef _DMLIB_H_
#define _DMLIB_H_

#include "qbl_adapter_types.h"
#include INC_DIR(bluestack,bluetooth.h)
#include INC_DIR(bluestack,dm_prim.h)
#include INC_DIR(bluestack,l2cap_prim.h)

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/** This type describes the sm key updation status */
typedef enum
{
    /*! SM key update status failed */
    DM_SM_UPDATE_KEY_STATUS_FAILED,

    /*! SM key update status in progress */
    DM_SM_UPDATE_KEY_STATUS_INPROGRESS,

    /*! SM key update status completed */
    DM_SM_UPDATE_KEY_STATUS_SUCCESS

}DM_SM_UPDATE_SM_KEY_STATUS;

typedef void (*dm_update_sm_key_state_cb)(bool_t status);

void dm_send_primitive(
    DM_UPRIM_T *p_prim
    );

extern void dm_free_upstream_primitive(
    DM_UPRIM_T *p_uprim
    );

void dm_free_downstream_primitive(
    DM_UPRIM_T *p_uprim
    );

void dm_free_primitive(
    DM_UPRIM_T *p_uprim
    );

extern void dm_free_sm_keys(
    DM_SM_KEYS_T keys
    );

void dm_am_register_req(
    phandle_t phandle
    );

void dm_write_cached_page_mode_req(
    BD_ADDR_T *p_bd_addr,
    page_scan_mode_t page_scan_mode,
    page_scan_rep_mode_t page_scan_rep_mode,
    DM_UPRIM_T **pp_prim
    );

void dm_write_cached_clock_offset_req(
    BD_ADDR_T *p_bd_addr,
    uint16_t clock_offset,
    DM_UPRIM_T **pp_prim
    );

void dm_clear_param_cache_req(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_acl_open_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t         flags,
    DM_UPRIM_T **pp_prim
    );

void dm_acl_close_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t flags,
    uint8_t reason,
    DM_UPRIM_T **pp_prim
    );

void dm_set_default_link_policy_req(
    link_policy_settings_t default_lp_in,
    link_policy_settings_t default_lp_out,
    DM_UPRIM_T **pp_prim
    );

void dm_set_link_behavior_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t conftab_length,
    uint16_t *conftab,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_inquiry(
    uint24_t lap,
    uint8_t inquiry_length,
    uint8_t num_responses,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_inquiry_cancel(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_periodic_inquiry(
    uint16_t max_period_length,
    uint16_t min_period_length,
    uint24_t lap,
    uint8_t inquiry_length,
    uint8_t num_responses,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_enhanced_flush(
    BD_ADDR_T *p_bd_addr,
    flushable_packet_type_t  packet_type,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_exit_periodic_inquiry(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_change_link_key(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_change_packet_type_sco(
    hci_connection_handle_t handle,
    hci_pkt_type_t pkt_type
    );

void dm_hci_change_packet_type_acl(
    BD_ADDR_T *p_bd_addr,
    hci_pkt_type_t pkt_type
    );

void dm_hci_master_link_key(
    hci_key_flag_t link_key_type,   /* 0 = regular link key, 1 = temp link key */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_refresh_encryption_key(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_remote_name_request(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_remote_features(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_create_connection_cancel(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_remote_name_req_cancel(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_remote_ext_features(
    BD_ADDR_T *p_bd_addr,
    uint8_t page_num,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_lmp_handle(
    hci_connection_handle_t handle,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_remote_version(
    TP_BD_ADDR_T *tp_addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_clock_offset(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_host_num_completed_packets(
     uint8_t num_handles,
     /* Array of pointers to 16 HANDLE_COMPLETE_T structures */
     HANDLE_COMPLETE_T *ap_handle_completes[],
     DM_UPRIM_T **pp_prim
     );

void dm_hci_host_num_completed_packets_sco(
    uint8_t num_handles,
    /* Array of pointers to 16 HANDLE_COMPLETE_T structures */
    HANDLE_COMPLETE_T *ap_handle_completes[]
    );

void dm_hci_hold_mode(
    BD_ADDR_T *p_bd_addr,
    uint16_t max_interval,
    uint16_t min_interval,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_sniff_mode(
    BD_ADDR_T *p_bd_addr,
    uint16_t max_interval,
    uint16_t min_interval,
    uint16_t attempt,
    uint16_t timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_exit_sniff_mode(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_qos_setup_req(
    BD_ADDR_T *p_bd_addr,
    uint8_t flags,              /* Reserved */
    hci_qos_type_t service_type,
    uint32_t token_rate,         /* in bytes per second */
    uint32_t peak_bandwidth,     /* peak bandwidth in bytes per sec */
    uint32_t latency,            /* in microseconds */
    uint32_t delay_variation,    /* in microseconds */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_role_discovery(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_switch_role(
    BD_ADDR_T *p_bd_addr,
    hci_role_t role,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_lp_settings(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_lp_settings(
    BD_ADDR_T *p_bd_addr,
    link_policy_settings_t link_policy_settings,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_default_link_policy_settings(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_default_link_policy_settings(
    link_policy_settings_t default_lps,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_flow_specification(
    hci_connection_handle_t handle,
    BD_ADDR_T *p_bd_addr,
    uint8_t flags,
    uint8_t flow_direction,
    uint8_t service_type,
    uint32_t token_rate,
    uint32_t token_bucket_size,
    uint32_t peak_bandwidth,
    uint32_t access_latency,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_set_event_mask(
    hci_event_mask_t event_mask_low,
    hci_event_mask_t event_mask_high,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_reset(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_set_event_filter(
    filter_type_t filter_type,
    filter_condition_type_t filter_condition_type,
    CONDITION_T *p_condition,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_flush(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_pin_type(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_pin_type(
    pin_type_t pin_type,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_stored_link_key(
    BD_ADDR_T *p_bd_addr,       /* Optional, can be NULL */
    read_all_flag_t read_all,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_stored_link_key(
    uint8_t number_keys,
    /* Array of pmalloc()ed LINK_KEY_BD_ADDR_T pointers */
    LINK_KEY_BD_ADDR_T *ap_link_key_bd_addr[],
    DM_UPRIM_T **pp_prim
    );

void dm_hci_delete_stored_link_key(
    BD_ADDR_T *p_bd_addr,       /* Optional, can be NULL */
    delete_all_flag_t flag,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_change_local_name(
    uint8_t *sz_name,   /* Nul-terminated name string */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_local_name(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_local_ext_features(
    uint8_t page_num,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_conn_accept_to(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_conn_accept_to(
    uint16_t conn_accept_timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_page_to(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_page_to(
    uint16_t page_timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_scan_enable(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_scan_enable(
    uint8_t scan_enable,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_pagescan_activity(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_pagescan_activity(
    uint16_t pagescan_interval,
    uint16_t pagescan_window,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_inquiryscan_activity(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_inquiryscan_activity(
    uint16_t inqscan_interval,
    uint16_t inqscan_window,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_auth_enable(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_encryption_mode(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_class_of_device(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_class_of_device(
    uint24_t dev_class,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_voice_setting(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_voice_setting(
    uint16_t voice_setting,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_auto_flush_timeout(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_auto_flush_timeout(
    BD_ADDR_T *p_bd_addr,
    uint16_t timeout,       /* N x 0.625msec */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_num_bcast_txs(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_num_bcast_txs(
    uint8_t num,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_hold_mode_activity(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_hold_mode_activity(
    uint8_t activity,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_tx_power_level(
    TP_BD_ADDR_T *tp_addrt,
    uint8_t type,       /* 0=current 1=Max */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_sco_flow_control_enable(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_sco_flow_control_enable(
    uint8_t enable,     /* 0=off, 1=on */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_link_superv_timeout(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_link_superv_timeout(
    BD_ADDR_T *p_bd_addr,
    uint16_t timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_num_supported_iac(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_current_iac_lap(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_current_iac_lap(
    uint8_t num_iac,
    uint24_t *a_iacs,   /* Array of IACs */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_pagescan_period_mode(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_pagescan_period_mode(
    uint8_t mode,       /* HCI_PAGESCAN_PERIOD_MODE_P0/1/2 */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_pagescan_mode(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_pagescan_mode(
    uint8_t mode,       /* HCI_PAGE_SCAN_MODE_MANDATORY etc */
    DM_UPRIM_T **pp_prim
    );

void dm_hci_set_afh_channel_class(
    uint8_t * map,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_inquiry_scan_type(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_inquiry_scan_type(
    uint8_t mode,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_inquiry_mode(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_inquiry_mode(
    uint8_t mode,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_page_scan_type(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_page_scan_type(
    uint8_t mode,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_afh_channel_class_m(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_afh_channel_class_m(
    uint8_t class_mode,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_local_version(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_local_features(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_country_code(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_bd_addr(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_failed_contact_counter(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_reset_contact_counter(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_get_link_quality(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_rssi(
    TP_BD_ADDR_T *tp_addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_afh_channel_map(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_clock(
    uint8_t whichClock,
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_enable_device_ut_mode(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_extended_inquiry_response_data(
    uint8_t     fec_required,
    uint8_t     eir_data_length,
    uint8_t     *eir_data,
    DM_UPRIM_T  **pp_prim
    );

void dm_hci_read_extended_inquiry_response_data(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_sniff_sub_rate(
    BD_ADDR_T               *p_bd_addr,
    uint16_t                max_remote_latency,
    uint16_t                min_remote_timeout,
    uint16_t                min_local_timeout,
    DM_UPRIM_T              **pp_prim
    );

void dm_hci_write_inquiry_transmit_power_level_req(
    int8_t tx_power,
    DM_UPRIM_T **pp_prim
    );
 
void dm_hci_read_inquiry_response_tx_power_level_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_secure_connections_host_support( DM_UPRIM_T **pp_prim );

void dm_hci_write_secure_connections_host_support(
    uint8_t      secure_connections_host_support,
    DM_UPRIM_T **pp_prim );

void dm_sm_io_capability_request_rsp(
    TP_BD_ADDR_T *tp_addrt,
    uint8_t     io_capability,
    uint8_t     authentication_requirements,
    uint8_t     oob_data_present,
    uint8_t     *oob_hash_c,
    uint8_t     *oob_rand_r,
    uint16_t    key_distribution,
    DM_UPRIM_T  **pp_prim
    );

void dm_sm_io_capability_request_neg_rsp(
    TP_BD_ADDR_T *tp_addrt,
    hci_error_t reason,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_user_confirmation_request_rsp(
    TP_BD_ADDR_T *tp_addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_user_confirmation_request_neg_rsp(
    TP_BD_ADDR_T *tp_addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_user_passkey_request_rsp(
    TP_BD_ADDR_T *tp_addrt,
    uint32_t numeric_value,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_user_passkey_request_neg_rsp(
    TP_BD_ADDR_T *tp_addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_read_local_oob_data_req(
    PHYSICAL_TRANSPORT_T tp_type,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_link_key_request_rsp(
    BD_ADDR_T *p_bd_addr,
    uint8_t   key_type,
    uint8_t   *key,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_send_keypress_notification_req(
    TP_BD_ADDR_T *tp_addrt,
    uint8_t   notification_type,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_bonding_req(
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_bonding_cancel_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t flags,
    DM_UPRIM_T **pp_prim
    );

void dm_set_bt_version(
    uint8_t     version,
    DM_UPRIM_T  **pp_prim
    );

void dm_set_ble_connection_parameters_req(uint16_t scan_interval,
                                         uint16_t scan_window,
                                         uint16_t conn_interval_min,
                                         uint16_t conn_interval_max,
                                         uint16_t conn_latency,
                                         uint16_t supervision_timeout,
                                         uint16_t conn_attempt_timeout,
                                         uint16_t conn_latency_max,
                                         uint16_t supervision_timeout_min,
                                         uint16_t supervision_timeout_max,
                                         uint8_t own_address_type,
                                         DM_UPRIM_T  **pp_prim
                                        );

void dm_set_ble_central_connection_parameters_update_req(uint16_t conn_interval_min,
                                         uint16_t conn_interval_max,
                                         uint16_t conn_latency_min,
                                         uint16_t conn_latency_max,
                                         uint16_t supervision_timeout_min,
                                         uint16_t supervision_timeout_max,
                                         DM_UPRIM_T  **pp_prim
                                        );


void dm_lp_write_roleswitch_policy_req(
    uint16_t    version,
    uint16_t    length,
    uint16_t    *rs_table,
    DM_UPRIM_T  **pp_prim
    );

void dm_lp_write_powerstates_req(
    BD_ADDR_T       *p_bd_addr,
    uint16_t        num_states,
    LP_POWERSTATE_T *states,
    DM_UPRIM_T      **pp_prim
    );

void dm_lp_write_always_master_devices_req(
    uint16_t operation,
    BD_ADDR_T *bd_addr,
    DM_UPRIM_T  **pp_prim
    );

void dm_ampm_register_req(const phandle_t phandle);

void dm_ampm_connect_rsp(const DM_AMPM_CONNECT_IND_T *const p_ind_prim,
                         const hci_return_t status);

void dm_ampm_connect_channel_rsp(const DM_AMPM_CONNECT_CHANNEL_IND_T *const p_ind_prim,
                                 const amp_link_id_t logical_link_id,
                                 const uint8_t physical_link_id,
                                 const phandle_t hci_data_queue,
                                 const hci_return_t status);

void dm_ampm_disconnect_channel_rsp(const DM_AMPM_DISCONNECT_CHANNEL_IND_T *const p_ind_prim,
                                    const hci_return_t status);

void dm_ampm_disconnect_req(const BD_ADDR_T *const p_bd_addr,
                            const l2ca_controller_t local_amp_id,
                            const hci_error_t reason,
                            const uint8_t active_links);

void dm_ampm_read_bd_addr_req(void);

void dm_ampm_read_data_block_size_rsp(const l2ca_controller_t local_amp_id,
                                      const uint8_t status,
                                      const bool_t fragmentable,
                                      const uint16_t max_pdu_length,
                                      const uint16_t max_acl_data_packet_length,
                                      const uint16_t data_block_length,
                                      const uint16_t total_num_data_blocks);

void dm_ampm_number_completed_data_blocks_req(const l2ca_controller_t local_amp_id,
                                              const uint16_t total_num_data_blocks,
                                              const uint8_t number_of_handles,
                                              DM_AMPM_NCB_T **num_completed_blks_ptr);

void dm_ampm_verify_physical_link_rsp(const BD_ADDR_T *const p_bd_addr,
                                      const uint16_t identifier,
                                      const bool_t exists,
                                      const l2ca_fs_flush_t link_supervision_timeout,
                                      const l2ca_fs_flush_t best_effort_flush_timeout);

extern void dm_data_from_hci_req(const l2ca_controller_t controller,
                                 const uint8_t physical_handle,
                                 const uint16_t logical_handle,
                                 MBLK_T *data,
                                 DM_UPRIM_T **pp_prim);

/* API-preservation for dm_sm_init_req */
#define dm_sm_init_req(options, \
                       mode, \
                       security_level_default, \
                       config, \
                       write_auth_enable, \
                       mode3_enc, \
                       sm_key_state, \
                       sm_div_state, \
                       pp_prim) \
        dm_sm_init_req_le_enc((options), \
                          (mode), \
                          (security_level_default), \
                          (config), (write_auth_enable), \
                          (mode3_enc), \
                          (sm_key_state), \
                          (sm_div_state), \
                          1, \
                          MAX_ENC_KEY_SIZE_VAL, \
                          MIN_ENC_KEY_SIZE_VAL, \
                          (pp_prim))

/* API-preservation for dm_sm_init_req_le */
#define dm_sm_init_req_le(options, \
                       mode, \
                       security_level_default, \
                       config, write_auth_enable, \
                       mode3_enc, \
                       sm_key_state, \
                       sm_div_state, \
                       sm_sign_counter, \
                       pp_prim) \
        dm_sm_init_req_le_enc((options), \
                          (mode), \
                          (security_level_default), \
                          (config), (write_auth_enable), \
                          (mode3_enc), \
                          (sm_key_state), \
                          (sm_div_state), \
                          (sm_sign_counter), \
                          MAX_ENC_KEY_SIZE_VAL, \
                          MIN_ENC_KEY_SIZE_VAL, \
                          (pp_prim))

void dm_sm_init_req_le_enc(
    uint16_t options,
    dm_security_mode_t mode,
    dm_security_level_t security_level_default,
    uint16_t config,
    uint16_t write_auth_enable,
    uint8_t mode3_enc,
    DM_SM_KEY_STATE_T *sm_key_state,
    uint16_t sm_div_state,
    uint32_t sm_sign_counter,
    uint8_t max_key_size_val,
    uint8_t min_key_size_val,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_register_req(
    phandle_t phandle,
    context_t context,
    dm_protocol_id_t protocol_id,
    uint16_t channel,
    bool_t outgoing_ok,
    dm_security_level_t security_level,
    psm_t psm,  /* Zero if don't care about connectionless security */
    DM_UPRIM_T **pp_prim
    );

void dm_sm_service_register_req(
    phandle_t phandle,
    context_t context,
    dm_protocol_id_t protocol_id,
    uint16_t channel,
    bool_t outgoing_ok,
    dm_security_level_t security_level,
    uint8_t min_enc_key_size,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_register_outgoing_req(
    phandle_t phandle,
    context_t context,
    BD_ADDR_T *p_bd_addr,
    dm_protocol_id_t protocol_id,
    uint16_t remote_channel,
    dm_security_level_t outgoing_security_level,
    psm_t psm,  /* Zero if don't care about connectionless security */
    DM_UPRIM_T **pp_prim
    );
void dm_sm_service_register_outgoing_req(
    phandle_t phandle,
    context_t context,
    BD_ADDR_T *p_bd_addr,
    dm_protocol_id_t protocol_id,
    uint16_t remote_channel,
    dm_security_level_t outgoing_security_level,
    uint8_t min_enc_key_size,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_unregister_req(
    phandle_t phandle,
    context_t context,
    dm_protocol_id_t protocol_id,
    uint16_t channel,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_unregister_outgoing_req(
    phandle_t phandle,
    context_t context,
    BD_ADDR_T *p_bd_addr,
    dm_protocol_id_t protocol_id,
    uint16_t remote_channel,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_access_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *p_addrt,
    dm_protocol_id_t protocol_id,
    uint16_t channel,
    bool_t incoming,
    uint32_t  context,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_pin_request_rsp(
    BD_ADDR_T *p_bd_addr,
    uint8_t pin_length,
    uint8_t *p_pin,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_authorise_rsp(
    TYPED_BD_ADDR_T *p_bd_addr,
    dm_protocol_id_t protocol_id,
    uint16_t channel,
    bool_t incoming,
    uint16_t authorisation,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_authenticate_req(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_encrypt_req(
    BD_ADDR_T *p_bd_addr,
    bool_t encrypt,
    DM_UPRIM_T **pp_prim
    );

extern void dm_sync_register_req(phandle_t phandle, context_t pv_cbarg);

extern void dm_sync_unregister_req(phandle_t phandle, context_t pv_cbarg);

extern void dm_sync_connect_req(phandle_t phandle,
                                context_t pv_cbarg,
                                BD_ADDR_T *p_bd_addr,
                                uint32_t tx_bdw,
                                uint32_t rx_bdw,
                                uint16_t max_latency,
                                uint16_t voice_settings,
                                uint8_t retx_effort, 
                                hci_pkt_type_t packet_type);

extern void dm_sync_connect_rsp(BD_ADDR_T *p_bd_addr,
                                uint8_t response,
                                uint32_t tx_bdw,
                                uint32_t rx_bdw,
                                uint16_t max_latency,
                                uint16_t voice_settings,
                                uint8_t retx_effort,
                                hci_pkt_type_t packet_type);

extern void dm_sync_renegotiate_req(hci_connection_handle_t handle,
                                    uint16_t max_latency,
                                    uint8_t retx_effort,
                                    hci_pkt_type_t packet_type);

extern void dm_sync_disconnect_req(hci_connection_handle_t handle,
                                   hci_reason_t reason);

extern bool_t dm_sync_override_handler(hci_connection_handle_t handle,
                                       phandle_t phandle,
                                       context_t pv_cbarg);

void dm_hci_ulp_add_device_to_white_list_req(
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_clear_white_list_req(
    DM_UPRIM_T **pp_prim
    );

void dm_ble_update_connection_update_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t conn_interval_min,
    uint16_t conn_interval_max,
    uint16_t conn_latency,
    uint16_t supervision_timeout,
    uint16_t minimum_ce_length,
    uint16_t maximum_ce_length,
    DM_UPRIM_T **pp_prim
    );

void dm_ble_connection_par_update_rsp(
    l2ca_identifier_t signal_id,
    TYPED_BD_ADDR_T bd_addrt,
    uint16_t conn_interval_min,
    uint16_t conn_interval_max,
    uint16_t conn_latency,
    uint16_t supervision_timeout,
    uint16_t result,
    DM_UPRIM_T **pp_prim
    );


void dm_hci_ulp_create_connection_cancel_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_encrypt_req(
    uint8_t *aes_key,
    uint8_t *plaintext_data,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_rand_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_read_advertising_channel_tx_power_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_read_channel_map_req(
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_read_local_supported_features_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_read_remote_used_features_req(
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_read_resolving_list_size_req(    
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_read_supported_states_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_read_white_list_size_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_receiver_test_req(
    uint8_t rx_channel,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_remove_device_from_white_list_req(
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_advertise_enable_req(
    uint8_t advertising_enable,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_advertising_data_req(
    uint8_t advertising_data_len,
    uint8_t *advertising_data,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_advertising_parameters_req(
    uint16_t adv_interval_min,
    uint16_t adv_interval_max,
    uint8_t advertising_type,
    uint8_t own_address_type,
    TYPED_BD_ADDR_T *direct_address,
    uint8_t advertising_channel_map,
    uint8_t advertising_filter_policy,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_event_mask_req(
    hci_event_mask_t *ulp_event_mask,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_host_channel_classification_req(
    uint8_t *channel_map,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_privacy_mode_req(
    TYPED_BD_ADDR_T *peer_addrt,
    uint8_t privacy_mode,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_random_address_req(
    BD_ADDR_T *random_address,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_scan_enable_req(
    uint8_t scan_enable,
    uint8_t filter_duplicates,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_scan_parameters_req(
    uint8_t scan_type,
    uint16_t scan_interval,
    uint16_t scan_window,
    uint8_t own_address_type,
    uint8_t scanning_filter_policy,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_scan_response_data_req(
    uint8_t scan_response_data_len,
    uint8_t *scan_response_data,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_test_end_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_transmitter_test_req(
    uint8_t tx_channel,
    uint8_t length_test_data,
    uint8_t packet_payload,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_auto_configure_local_address_req(
    DM_SM_RANDOM_ADDRESS_T permanent_address_type,
    TP_BD_ADDR_T *static_addrt,
    uint16_t rpa_timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_security_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    l2ca_conflags_t connection_flags,
    context_t context,
    uint16_t security_requirements,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_key_request_rsp(
    TYPED_BD_ADDR_T *addrt,
    uint16_t security_requirements,
    DM_SM_KEY_TYPE_T key_type,
    DM_SM_UKEY_T key,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_key_request_neg_rsp(
    TYPED_BD_ADDR_T *addrt,
    DM_SM_KEY_TYPE_T key_type,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_add_device_req(
    DM_UPRIM_T **pp_prim,
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    DM_SM_TRUST_T trust,
    uint16_t security_requirements,
    uint16_t encryption_key_size,
    unsigned int keys_present,
    ...
    );

void dm_sm_read_device_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_read_random_address_req(
    TP_BD_ADDR_T *tp_peer_addrt,
    uint16_t flags,
    DM_UPRIM_T **pp_prim 
    );

void dm_sm_remove_device_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dmlib_sm_generate_nonresolvable_private_address_req(
    DM_UPRIM_T **pp_prim
    );

void dmlib_sm_data_sign_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    context_t context,
    bool_t verify,
    uint16_t length,
    uint8_t *data,
    DM_UPRIM_T **pp_prim);

void dm_hci_read_encryption_key_size(
    const BD_ADDR_T *bd_addr,
    DM_UPRIM_T **pp_prim);


void dm_handle_device_black_list_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t flags,
    DM_UPRIM_T **pp_prim);

void dm_sm_oob_remote_data_request_rsp(
    TYPED_BD_ADDR_T     *addrt,
    uint8_t     *oob_hash_c,
    uint8_t     *oob_rand_r,
    DM_UPRIM_T  **pp_prim
    );

bool_t dm_get_public_address(
    const TP_BD_ADDR_T * const resolvable_addr,
    TP_BD_ADDR_T * public_addr );

bool_t dm_get_local_irk(
    DM_SM_KEY_ID_T *irk
    );

hci_role_t dm_acl_get_role(
    const TP_BD_ADDR_T *const tp_addrt
    );

hci_bt_mode_t dm_acl_get_mode(
    const TP_BD_ADDR_T *const tp_addrt
    );

hci_connection_handle_t dm_get_hci_handle(
    const TP_BD_ADDR_T *const tp_addrt
    );

DM_SM_UPDATE_SM_KEY_STATUS dm_update_sm_key_state(
    DM_SM_KEY_STATE_T *sm_key_state,
    dm_update_sm_key_state_cb cb
    );

void dm_hci_read_authenticated_payload_timeout(
    TP_BD_ADDR_T *tp_addrt,
    DM_UPRIM_T  **pp_prim
    );

void dm_sm_write_authenticated_payload_timeout(
    TP_BD_ADDR_T     *tp_addrt,
    uint16_t          authenticated_payload_timeout,
    DM_SM_APT_ROUTE_T route_event,
    DM_UPRIM_T      **pp_prim
    );

void dm_hci_set_reserved_lt_addr_req(
    uint8_t lt_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_delete_reserved_lt_addr_req(
    uint8_t lt_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_set_csb_req(
    uint8_t enable,
    uint8_t lt_addr,
    uint8_t lpo_allowed,
    hci_pkt_type_t packet_type,
    uint16_t interval_min,
    uint16_t interval_max,
    uint16_t supervision_timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_set_csb_data_req(
    uint8_t lt_addr,
    uint8_t fragment,
    uint8_t data_length,
    uint8_t *data_part,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_write_synchronization_train_params_req(
    uint16_t interval_min,
    uint16_t interval_max,
    uint32_t sync_train_timeout,
    uint8_t service_data,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_read_synchronization_train_params_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_start_synchronization_train_req(
    DM_UPRIM_T **pp_prim
    );

void dm_hci_truncated_page_req(
    BD_ADDR_T *p_bd_addr,
    page_scan_rep_mode_t page_scan_rep_mode,
    clock_offset_t clock_offset,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_truncated_page_cancel_req(
    BD_ADDR_T *p_bd_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_set_csb_receive_req(
    uint8_t enable,
    BD_ADDR_T *p_bd_addr,
    uint8_t lt_addr,
    uint16_t interval,
    uint32_t clock_offset,
    uint32_t next_csb_clock,
    uint16_t supervision_timeout,
    uint8_t remote_timing_accuracy,
    uint8_t skip,
    hci_pkt_type_t packet_type,
    uint8_t *afh_channel_map,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_receive_synchronization_train_req(
    BD_ADDR_T *p_bd_addr,
    uint16_t sync_scan_timeout,
    uint16_t sync_scan_window,
    uint16_t sync_scan_interval,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_csb_tx_timeout_rsp(
    uint8_t lt_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_csb_rx_timeout_rsp(
    BD_ADDR_T *p_bd_addr,
    uint8_t lt_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_controller_ready_ntf(
    const uint8_t status,
    DM_UPRIM_T **pp_prim
    );

void dm_ulp_set_phy_req(
    phandle_t phandle,
    const TP_BD_ADDR_T *p_tp_bd_addr,
    phy_rate_t min_tx,
    phy_rate_t max_tx,
    phy_rate_t min_rx,
    phy_rate_t max_rx,
    phy_pref_t flags,
    DM_UPRIM_T **pp_prim);

void dm_ulp_set_default_phy_req(
    phandle_t phandle,
    phy_rate_t min_tx,
    phy_rate_t max_tx,
    phy_rate_t min_rx,
    phy_rate_t max_rx,
    phy_pref_t flags,
    DM_UPRIM_T **pp_prim);

void dm_ulp_read_phy_req(
    phandle_t phandle,
    const TP_BD_ADDR_T *p_tp_bd_addr,
    DM_UPRIM_T **pp_prim);

void dm_vs_register_req(
    phandle_t phandle
    );

void dm_vs_command_req(
    phandle_t phandle,
    uint32_t context,
    uint16_t opcode_ocf,
    uint8_t data_length,
    uint8_t *data,
    uint16_t flow_control_flags,
    bool_t credit_not_required,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_enhanced_receiver_test_req(
    uint8_t rx_channel,
    uint8_t phy,
    uint8_t mod_index,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_enhanced_transmitter_test_req(
    uint8_t tx_channel,
    uint8_t length_test_data,
    uint8_t packet_payload,
    uint8_t phy,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_configure_data_path_req(
    uint8_t data_path_direction,
    uint8_t data_path_id,
    uint8_t vendor_specific_config_len,
    uint8_t *vendor_specific_config[],
    DM_UPRIM_T **pp_prim
    );

void dm_sm_read_local_irk_req(
    phandle_t phandle,
    TYPED_BD_ADDR_T *addrt,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_sirk_operation_req(
    phandle_t phandle,
    context_t context,
    TP_BD_ADDR_T *tp_addrt,
    uint8_t flags,
    uint8_t sirk_key[],
    DM_UPRIM_T **pp_prim
    );

void dm_sm_config_req(
    phandle_t phandle,
    uint16_t config_mask,
    uint8_t length,
    void *params,
    DM_UPRIM_T **pp_prim
    );

void dm_sm_generate_cross_trans_key_request_rsp(
        TP_BD_ADDR_T     *tp_addrt,
        uint8_t           identifier,
        uint16_t          flags,
        DM_UPRIM_T      **pp_prim);

void dm_ulp_get_adv_scan_capabilities_req(
    DM_UPRIM_T **pp_prim
    );

void dm_ulp_set_data_related_address_changes_req(
    uint8_t adv_handle,
    uint8_t flags,
    uint8_t change_reasons,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_sets_info_req(
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_set_random_address_req(
    uint8_t      adv_handle,
    uint16_t     action,
    BD_ADDR_T   *adv_random_addr,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_register_app_adv_set_req(
    uint8_t adv_handle,
    uint32_t flags,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_unregister_app_adv_set_req(
    uint8_t adv_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_set_params_req(
    uint8_t  adv_handle,
    uint16_t adv_event_properties,
    uint32_t primary_adv_interval_min,
    uint32_t primary_adv_interval_max,
    uint8_t  primary_adv_channel_map,
    uint8_t  own_addr_type,
    TYPED_BD_ADDR_T *peer_addr,
    uint8_t  adv_filter_policy,
    uint16_t primary_adv_phy,
    uint8_t  secondary_adv_max_skip,
    uint16_t secondary_adv_phy,
    uint16_t adv_sid,
    uint32_t reserved,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_set_params_v2_req(
    uint8_t  adv_handle,
    uint16_t adv_event_properties,
    uint32_t primary_adv_interval_min,
    uint32_t primary_adv_interval_max,
    uint8_t  primary_adv_channel_map,
    uint8_t  own_addr_type,
    TYPED_BD_ADDR_T *peer_addr,
    uint8_t  adv_filter_policy,
    uint16_t primary_adv_phy,
    uint8_t  secondary_adv_max_skip,
    uint16_t secondary_adv_phy,
    uint16_t adv_sid,
    int8_t   adv_tx_pwr,
    uint8_t  scan_req_notify_enable,
    uint8_t  primary_adv_phy_options,
    uint8_t  secondary_adv_phy_options,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ext_adv_set_data_req(
    uint8_t adv_handle,
    uint8_t operation,
    uint8_t reserved,
    uint8_t advertising_data_len,
    uint8_t *advertising_data,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ext_adv_set_scan_resp_data_req(
    uint8_t adv_handle,
    uint8_t operation,
    uint8_t reserved,
    uint8_t scan_resp_data_len,
    uint8_t *scan_resp_data,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_enable_req(
    uint8_t adv_handle,
    uint8_t enable,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_multi_enable_req(
    uint8_t enable,
    uint8_t num_sets,
    EA_ENABLE_CONFIG_T config[],
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_read_max_adv_data_len_req(
    uint8_t adv_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_adv_get_addr_req(
    uint8_t adv_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_scan_get_global_params_req(
    phandle_t phandle,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_scan_set_global_params_req(
    phandle_t phandle,
    uint8_t flags,
    uint8_t own_address_type,
    uint8_t scanning_filter_policy,
    uint8_t filter_duplicates,
    uint16_t scanning_phys,
    ES_SCANNING_PHY_T phys[],
    DM_UPRIM_T **pp_prim
    );

void dm_ext_scan_register_scanner_req(
    phandle_t phandle,
    uint32_t  flags,
    uint16_t  adv_filter,
    uint16_t  adv_filter_sub_field1,
    uint32_t  adv_filter_sub_field2,
    uint16_t  ad_structure_filter,
    uint16_t  ad_structure_filter_sub_field1,
    uint32_t  ad_structure_filter_sub_field2,
    uint16_t  ad_structure_info_len,
    uint8_t  *ad_structure_info,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_scan_unregister_scanner_req(
    phandle_t phandle,
    uint8_t scan_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_ext_scan_configure_scanner_req(
    uint8_t scan_handle,
    uint8_t use_only_global_params,
    uint16_t scanning_phys,
    DM_ULP_EXT_SCAN_PHY_T phys[],
    DM_UPRIM_T **pp_prim
    );

void dm_ext_scan_enable_scanners_req(
    phandle_t phandle,
    uint8_t enable,
    uint8_t num_of_scanners,
    DM_ULP_EXT_SCAN_SCANNERS_T scanners[],
    DM_UPRIM_T **pp_prim
    );

void dm_ext_scan_get_ctrl_scan_info_req(
    phandle_t phandle,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_adv_set_params_req(
    uint8_t adv_handle,
    uint32_t flags,
    uint16_t periodic_adv_interval_min,
    uint16_t periodic_adv_interval_max,
    uint16_t periodic_adv_properties,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_adv_set_data_req(
    uint8_t adv_handle,
    uint8_t operation,
    uint8_t advertising_data_len,
    uint8_t *advertising_data,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_adv_read_max_adv_data_len_req(
    uint8_t adv_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_adv_start_req(
    uint8_t adv_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_adv_stop_req(
    uint8_t adv_handle,
    uint8_t stop_advertising,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_adv_enable_req(
    uint8_t adv_handle,
    uint16_t flags,
    uint8_t enable,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_adv_set_transfer_req(
    phandle_t       phandle,
    TYPED_BD_ADDR_T *addrt,
    uint16_t        service_data,
    uint8_t         adv_handle,
    DM_UPRIM_T      **pp_prim
    );

void dm_periodic_scan_start_find_trains_req(
    phandle_t    phandle,
    uint32_t     flags,
    uint16_t     scan_for_x_seconds,
    uint16_t     ad_structure_filter,
    uint16_t     ad_structure_filter_sub_field1,
    uint32_t     ad_structure_filter_sub_field2,
    uint16_t     ad_structure_info_len,
    uint8_t     *ad_structure_info,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_scan_stop_find_trains_req(
    phandle_t phandle,
    uint8_t scan_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_scan_sync_to_train_req(
    phandle_t       phandle,
    uint8_t         report_periodic,
    uint16_t        skip,
    uint16_t        sync_timeout,
    uint8_t         sync_cte_type,
    uint16_t        attempt_sync_for_x_seconds,
    uint8_t         number_of_periodic_trains,
    DM_ULP_PERIODIC_SCAN_TRAINS_T  periodic_trains[],
    DM_UPRIM_T    **pp_prim
    );

void dm_periodic_scan_sync_transfer_req(
    phandle_t       phandle,
    TYPED_BD_ADDR_T *addrt,
    uint16_t        service_data,
    uint16_t        sync_handle,
    DM_UPRIM_T      **pp_prim
    );

void dm_periodic_scan_sync_transfer_params_req(
    phandle_t       phandle,
    TYPED_BD_ADDR_T *addrt,
    uint8_t         mode,
    uint16_t        skip,
    uint16_t        sync_timeout,
    uint8_t         cte_type,
    DM_UPRIM_T      **pp_prim
    );

void dm_periodic_scan_sync_to_train_cancel_req(
    phandle_t phandle,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_scan_sync_adv_report_enable_req(
    phandle_t phandle,
    uint16_t sync_handle,
    uint8_t enable,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_scan_sync_terminate_req(
    phandle_t phandle,
    uint16_t sync_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_periodic_scan_sync_lost_rsp(
    uint16_t sync_handle,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_register_req(
    phandle_t phandle,
    uint16_t isoc_type,
    context_t reg_context,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_configure_cig_req(
    phandle_t phandle,
    context_t context,
    uint8_t cig_id,
    uint24_t sdu_interval_m_to_s,
    uint24_t sdu_interval_s_to_m,
    uint8_t sca,
    uint8_t packing,
    uint8_t framing,
    uint16_t max_transport_latency_m_to_s,
    uint16_t max_transport_latency_s_to_m,
    uint8_t cis_count,
    DM_CIS_CONFIG_T *cis_config[],
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_configure_cig_test_req(
    phandle_t            phandle,
    context_t            context,
    uint24_t             sdu_interval_m_to_s,
    uint24_t             sdu_interval_s_to_m,
    uint16_t             iso_interval,
    uint8_t              cig_id,
    uint8_t              ft_m_to_s,
    uint8_t              ft_s_to_m,
    uint8_t              sca,
    uint8_t              packing,
    uint8_t              framing,
    uint8_t              cis_count,
    DM_CIS_CONFIG_TEST_T *cis_config[],
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_remove_cig_req(
    uint8_t cig_id,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_cis_connect_req(
    phandle_t phandle,
    context_t con_context,
    uint8_t cis_count,
    DM_CIS_CONNECTION_T *cis_conn[],
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_cis_connect_rsp(
    phandle_t phandle,
    context_t con_context,
    hci_connection_handle_t cis_handle,
    uint8_t status,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_cis_disconnect_req(
    hci_connection_handle_t cis_handle,
    uint8_t reason,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_create_big_req(
    phandle_t   phandle,
    context_t   con_context,
    DM_BIG_CONFIG_PARAM_T   *big_config,
    uint8_t     big_handle,
    uint8_t     adv_handle,
    uint8_t     num_bis,
    uint8_t     encryption,
    uint8_t     *broadcast_code,
    DM_UPRIM_T  **pp_prim
    );

void dm_isoc_create_big_test_req(
    phandle_t   phandle,
    context_t   con_context,
    DM_BIG_TEST_CONFIG_PARAM_T   *big_config,
    uint8_t     big_handle,
    uint8_t     adv_handle,
    uint8_t     num_bis,
    uint8_t     encryption,
    uint8_t     *broadcast_code,
    DM_UPRIM_T  **pp_prim);

void dm_isoc_terminate_big_req(
    uint8_t     big_handle,
    uint8_t     reason,
    DM_UPRIM_T  **pp_prim
    );

void dm_isoc_big_create_sync_req(
    phandle_t   phandle,
    context_t   con_context,
    uint8_t     big_handle,
    uint16_t    sync_handle,
    uint8_t     encryption,
    uint8_t     *broadcast_code,
    uint8_t     mse,
    uint16_t    big_sync_timeout,
    uint8_t     num_bis,
    uint8_t     *bis,
    DM_UPRIM_T  **pp_prim
    );

void dm_isoc_big_terminate_sync_req(
    uint8_t     big_handle,
    DM_UPRIM_T  **pp_prim
    );

void dm_isoc_iso_transmit_req(
    hci_connection_handle_t handle,
    uint8_t payload_type,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_iso_receive_req(
    hci_connection_handle_t handle,
    uint8_t payload_type,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_iso_read_test_counters_req(
    hci_connection_handle_t handle,
    DM_UPRIM_T **pp_prim
    );

void dm_isoc_setup_iso_data_path_req(
     hci_connection_handle_t handle,
     uint8_t  data_path_direction,
     uint8_t  data_path_id,
     uint8_t  codec_id[],
     uint24_t controller_delay,
     uint8_t  codec_config_length,
     uint8_t  *codec_config_data,
     DM_UPRIM_T **pp_prim
     );

void dm_isoc_remove_iso_data_path_req(
     hci_connection_handle_t handle,
     uint8_t data_path_direction,
     DM_UPRIM_T **pp_prim
     );

void dm_isoc_read_iso_link_quality_req(
    hci_connection_handle_t handle,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_set_default_subrate_req(
    uint16_t subrate_min,
    uint16_t subrate_max,
    uint16_t max_latency,
    uint16_t continuation_num,
    uint16_t supervision_timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_hci_ulp_subrate_change_req(
    TYPED_BD_ADDR_T *addrt,
    uint16_t subrate_min,
    uint16_t subrate_max,
    uint16_t max_latency,
    uint16_t continuation_num,
    uint16_t supervision_timeout,
    DM_UPRIM_T **pp_prim
    );

void dm_enhanced_read_transmit_power_level_req(
    TP_BD_ADDR_T *p_tp_bd_addr,
    uint8_t phy,
    DM_UPRIM_T **pp_prim);

void dm_read_remote_transmit_power_level_req(
    TP_BD_ADDR_T *p_tp_bd_addr,
    uint8_t phy,
    DM_UPRIM_T **pp_prim);

void dm_set_path_loss_reporting_parameters_req(
    TP_BD_ADDR_T    *p_tp_bd_addr,
    int8_t          high_threshold,
    int8_t          high_hysteresis,
    int8_t          low_threshold,
    int8_t          low_hysteresis,
    uint16_t        min_time_spent,
    DM_UPRIM_T      **pp_prim);

void dm_set_path_loss_reporting_enable_req(
    TP_BD_ADDR_T    *p_tp_bd_addr,
    uint8_t         enable,
    DM_UPRIM_T      **pp_prim);

void dm_set_transmit_power_reporting_enable_req(
    TP_BD_ADDR_T    *p_tp_bd_addr,
    uint8_t         local_enable,
    uint8_t         remote_enable,
    DM_UPRIM_T      **pp_prim);

void dm_crypto_generate_public_private_key_req(
    phandle_t       phandle,
    uint8_t         key_type,
    context_t       context,
    DM_UPRIM_T      **pp_prim
    );

void dm_crypto_generate_shared_secret_key_req(
    phandle_t       phandle,
    uint8_t         key_type,
    context_t       context,
    uint16_t        *local_private_key,
    uint16_t        *remote_public_key,
    DM_UPRIM_T      **pp_prim
    );

void dm_crypto_encrypt_req(
    phandle_t       phandle,
    uint8_t         flags,
    context_t       context,
    uint16_t        *data,
    uint16_t        *encryption_key,
    DM_UPRIM_T      **pp_prim
    );

void dm_crypto_hash_req(
    phandle_t       phandle,
    uint8_t         operation,
    uint8_t         flags,
    context_t       context,
    uint16_t        data_size,
    uint16_t        *data,
    DM_UPRIM_T      **pp_prim
    );

void dm_crypto_decrypt_req(
    phandle_t       phandle,
    uint8_t         flags,
    context_t       context,
    uint16_t        *cipher_text,
    uint16_t        *decryption_key,
    DM_UPRIM_T      **pp_prim
    );

void dm_crypto_aes_ctr_req(
    phandle_t           phandle,
    context_t           context,
    uint32_t            counter,
    dm_crypto_flags_t   flags,
    uint16_t            *secret_key,
    uint16_t            *nonce,
    uint16_t            data_len,
    uint16_t            *data,
    DM_UPRIM_T          **pp_prim
    );

void dm_write_sc_host_support_override_req(
    phandle_t    phandle,
    BD_ADDR_T    *p_bd_addr,
    uint8_t       host_support_override,
    DM_UPRIM_T   **pp_prim
    );

void dm_read_sc_host_support_override_max_bd_addr_req(
    phandle_t     phandle,
    DM_UPRIM_T    **pp_prim
    );

#ifdef __cplusplus
}
#endif 

#endif /* ndef _DMLIB_H */
