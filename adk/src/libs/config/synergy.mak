
ifeq (synergy, $(LIBTYPE))
    CONFIG_FEATURES+=CONFIG_SYNERGY

    CONFIG_DIRS_FILTER+= connection gatt_tmas_server gatt_cas_server gatt a2dp hfp avrcp gatt_client gatt_server gatt_manager gatt_battery_server gatt_device_info_server gatt_gap_server gatt_ascs_server gatt_aics_client gatt_bass_client gatt_bass_server gatt_cas_server gatt_csis_server gatt_pacs_server gatt_telephone_bearer_server gatt_vcs_client gatt_vcs_server gatt_service_discovery gatt_vocs_client vcp bap_server spp_common sppc spps ccp gatt_transmit_power_server gatt_transport_discovery_server aghfp hid mapc obex pbapc gatt_battery_client gatt_device_info_client gatt_heart_rate_client gatt_hid_client gatt_imm_alert_client gatt_scan_params_client gatt_telephone_bearer_client gatt_ama_server gatt_heart_rate_server gatt_logging_server gatt_imm_alert_server gatt_link_loss_server gatt_running_speed_cadence_server
    SYNERGY_DIRS = a2dp_synergy connection_synergy hfp_synergy avrcp_synergy gatt_synergy aghfp_synergy  

    TEMP_CONFIG_DIRS_FILTER := $(CONFIG_DIRS_FILTER)
    CONFIG_DIRS_FILTER = $(filter-out $(SYNERGY_DIRS), $(TEMP_CONFIG_DIRS_FILTER))
endif
