# --------------------------------------------------------------------------------------------------
# Copyright (c) 2023 Qualcomm Technologies International, Ltd
# --------------------------------------------------------------------------------------------------
#
# Provides preprocessing capabilities to the report generation script report_gen.py. defs provided
# here are used in report_gen.py script to preprocess the contents of the column being printed in
# a given overview report.
#
# Note:
# Type of the preprocessing is taken from report_desc.xml schema.
# Any developer who wish to add a different processing for a given set of inputs, shall define
# the defs here.
#
# --------------------------------------------------------------------------------------------------

def __get_device_address_hex(address):
    device_addr = hex(address.nap.value << 32 | address.uap.value << 24 | address.lap.value)
    return device_addr


def __get_l2cap_state_str(state):
    # Maximum number of L2CAP states, this is dependent on CsrBtCmStateL2cap. Needs to change if any changes are made to CsrBtCmStateL2cap.
    MAX_L2CAP_STATES = 14

    if state.value < MAX_L2CAP_STATES:
        l2cap_state = {0: "CSR_BT_CM_L2CAP_STATE_IDLE",
                       1: "CSR_BT_CM_L2CAP_STATE_CONNECT",
                       2: "CSR_BT_CM_L2CAP_STATE_CONNECT_INIT",
                       3: "CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT",
                       4: "CSR_BT_CM_L2CAP_STATE_CONNECT_ACCEPT_FINAL",
                       5: "CSR_BT_CM_L2CAP_STATE_SAVE_DISCONNECT",
                       6: "CSR_BT_CM_L2CAP_STATE_RELEASE_INIT",
                       7: "CSR_BT_CM_L2CAP_STATE_RELEASE_INIT_XOVER",
                       8: "CSR_BT_CM_L2CAP_STATE_RELEASE",
                       9: "CSR_BT_CM_L2CAP_STATE_RELEASE_FINAL",
                       10: "CSR_BT_CM_L2CAP_STATE_CONNECTABLE",
                       11: "CSR_BT_CM_L2CAP_STATE_CONNECTED",
                       12: "CSR_BT_CM_L2CAP_STATE_CANCEL_CONNECTABLE",
                       13: "CSR_BT_CM_L2CAP_STATE_LEGACY_DETACH"
                       }
        return str(l2cap_state[state.value]) + '(' + str(hex(state.value)) + ')'
    return "None"


def __get_rfc_state_str(state):
    # Maximum number of RFC states, this is dependent on CsrBtCmStateRfc. Needs to change if any changes are made to CsrBtCmStateRfc.
    MAX_RFC_STATES = 13
    if state.value < MAX_RFC_STATES:
        rfc_state = {0: "CSR_BT_CM_RFC_STATE_IDLE",
                     1: "CSR_BT_CM_RFC_STATE_CONNECT_INIT",
                     2: "CSR_BT_CM_RFC_STATE_CONNECT",
                     3: "CSR_BT_CM_RFC_STATE_ACCESS",
                     4: "CSR_BT_CM_RFC_STATE_CONNECTABLE",
                     5: "CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT",
                     6: "CSR_BT_CM_RFC_STATE_CONNECT_ACCEPT_FINAL",
                     7: "CSR_BT_CM_RFC_STATE_CANCEL_CONNECTABLE",
                     8: "CSR_BT_CM_RFC_STATE_RELEASE_INIT",
                     9: "CSR_BT_CM_RFC_STATE_RELEASE",
                     10: "CSR_BT_CM_RFC_STATE_RELEASE_FINAL",
                     11: "CSR_BT_CM_RFC_STATE_CONNECTED",
                     12: "CSR_BT_CM_RFC_STATE_CANCEL_TIMER"
                     }
        return str(rfc_state[state.value]) + '(' + str(hex(state.value)) + ')'
    return "None"


def __get_pacs_data(pacs_data):
    GATT_PACS_MAX_CONNECTIONS = 3
    res = ''
    for i in range(0, GATT_PACS_MAX_CONNECTIONS):
        res = res + 'connection id ' + str(i + 1) + ': ' + str(pacs_data[i].cid) + report_gen.enter
    return res


def __get_gatt_ascs_connection(gatt_ascs_connection):
    GATT_ASCS_NUM_CONNECTIONS_MAX = 3
    res = ''
    for i in range(0, GATT_ASCS_NUM_CONNECTIONS_MAX):
        if (gatt_ascs_connection[i].value != 0):
            res = res + 'connection id ' + str(i + 1) + ': ' + str(gatt_ascs_connection[i][0].cid) + report_gen.enter
            for j in range(0, 6):
                res = res + report_gen.tab + '|-aseId ' + str(j + 1) + ': ' + str(
                    gatt_ascs_connection[i][0].ase[j].aseId) + report_gen.enter
        else:
            res = res + 'connection id ' + str(i + 1) + ': 0x00000000' + report_gen.enter
    return res


def __get_gatt_bass_server_ccc_data_t(connected_clients):
    BASS_SERVER_MAX_CONNECTIONS = 3
    res = ''
    for i in range(0, BASS_SERVER_MAX_CONNECTIONS):
        res = res + 'connection id ' + str(i + 1) + ': ' + str(connected_clients[i].cid) + report_gen.enter
    return res


def __get_gatt_mics_client_data(connected_clients):
    MICS_MAX_CONNECTIONS = 3
    res = ''
    for i in range(0, MICS_MAX_CONNECTIONS):
        res = res + 'connection id ' + str(i + 1) + ': ' + str(connected_clients[i].cid) + report_gen.enter
    return res


def __get_mcp_mcs_srvc_hndl_data(mcs_service_handle):
    res = ''
    i = 1
    while (mcs_service_handle.value):
        res = res + 'service handle ' + str(i) + ': ' + str(mcs_service_handle[0].srvcHndl) + report_gen.enter
        mcs_service_handle = mcs_service_handle[0].next
        i = i + 1
    if (i == 1):
        res = res + 'empty'
    return res


def __get_handler_from_app_handle(app_handle):
    # mask for the identification of application and synergy tasks, part of synergy_adapter.c
    TASK_ID_MASK = 0xFF00
    # Maximum number of application tasks supported, part of synergy_adapter.c
    APP_TRAP_TASKS_MAX = 96
    handler = "(None)"
    index = app_handle.value
    if index < TASK_ID_MASK:
        # these are synergy tasks
        task_handler_string = str(apps1.fw.env.globalvars['SynergyTaskHandler'])
        search_index = task_handler_string.find("[" + str(index) + "]")
        if search_index != -1:
            search_string = task_handler_string[search_index + 1:]
            new_start_index = search_string.find("(")
            new_end_index = search_string.find(")")

            if (new_start_index != -1) and (new_end_index != -1):
                handler = search_string[new_start_index:new_end_index + 1]
    else:
        # application tasks needs to be fetched.
        my_index = index & ~TASK_ID_MASK
        if my_index < APP_TRAP_TASKS_MAX:
            if apps1.fw.env.cus["synergy_adapter.c"].localvars["trapTaskList"][my_index].value != 0:
                result_string = str(apps1.fw.env.cus["synergy_adapter.c"].localvars["trapTaskList"][my_index].deref)
                start_index = result_string.rfind("(")
                if start_index != -1:
                    handler = result_string[start_index:]
    return handler


def __get_lea_profile_data(profile_name):
    profile_mapper = {
        "CSR_BT_PACS_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_ASCS_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_BASS_SERVER_IFACEQUEUE": 0x00,
        # "CSR_BT_CAS_SERVER_IFACEQUEUE"  : 0x00,
        "CSR_BT_CSIS_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_VCS_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_MCS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_MCP_IFACEQUEUE": 0x00,
        "CSR_BT_BAP_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_TBS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_CCP_IFACEQUEUE": 0x00,
        "CSR_BT_VOCS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_AICS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_TBS_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_MICS_SERVER_IFACEQUEUE": 0x00,
        # "CSR_BT_PAC_IFACEQUEUE"         : 0x00,
        # "PAC_IFACEQUEUE_SEC_INSTANCE"   : 0x00,
        "CSR_BT_TMAS_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_TMAP_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_TMAS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_MCS_SERVER_IFACEQUEUE": 0x00,
        "CSR_BT_ASCS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_PACS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_CSIS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_BASS_CLIENT_IFACEQUEUE": 0x00,
        "CSR_BT_CSIP_IFACEQUEUE": 0x00,
        "CSR_BT_VCP_IFACEQUEUE": 0x00,
        # "CSR_BT_VCS_IFACEQUEUE"         : 0x00,
        "CSR_BT_MICP_IFACEQUEUE": 0x00,
        "CSR_BT_MICS_CLIENT_IFACEQUEUE": 0x00,
        # "CSR_BT_GMAS_SERVER_IFACEQUEUE" : 0x00,
        # "CSR_BT_GMAS_CLIENT_IFACEQUEUE" : 0x00,
        # "CSR_BT_GMAP_CLIENT_IFACEQUEUE" : 0x00,
        "CSR_BT_PBP_IFACEQUEUE": 0x00
    }

    count = 0
    for i in profile_mapper:
        a = (list(profile_mapper.keys())[count])
        task_value = apps1.fw.env.globalvars[a]
        profile_mapper[i] = (task_value.value)
        count = count + 1

    number_of_services = apps1.fw.var.service_handle_data.deref.value["size"]
    service_handles = apps1.fw.env.cast(apps1.fw.var.service_handle_data.deref.instances.address, 'uint32',
                                        array_len=number_of_services)
    service_handles = list(service_handles)

    lib_task_list = []
    for i in range(len(service_handles)):
        temp = service_handles[i]
        lib_task = apps1.fw.env.cast(int(str(service_handles[i]), 16), "uint16")
        if lib_task.value != 0x0000:
            lib_task_list.append([(lib_task.value), (temp.value)])

    # print(lib_task_list)
    if (profile_name == "pacss"):
        ifc_queue = "CSR_BT_PACS_SERVER_IFACEQUEUE"
        profile = "GPACSS_T"
    elif (profile_name == "ascss"):
        ifc_queue = "CSR_BT_ASCS_SERVER_IFACEQUEUE"
        profile = "GattAscsServer"
    elif (profile_name == "bassss"):
        ifc_queue = "CSR_BT_BASS_SERVER_IFACEQUEUE"
        profile = "GBASSSS"

    elif (profile_name == "csiss"):
        ifc_queue = "CSR_BT_CSIS_SERVER_IFACEQUEUE"
        profile = "GCSISS_T"
    elif (profile_name == "vcs"):
        ifc_queue = "CSR_BT_VCS_SERVER_IFACEQUEUE"
        profile = "GVCS"
    elif (profile_name == "mcs"):
        ifc_queue = "CSR_BT_MCS_CLIENT_IFACEQUEUE"
        profile = "GMCS_T"

    elif (profile_name == "mcp"):
        ifc_queue = "CSR_BT_MCP_IFACEQUEUE"
        profile = "MCP"
    elif (profile_name == "bap"):
        ifc_queue = "CSR_BT_BAP_SERVER_IFACEQUEUE"
        profile = "BAP"
    elif (profile_name == "gtbsc"):
        ifc_queue = "CSR_BT_TBS_CLIENT_IFACEQUEUE"
        profile = "GTBSC"

    elif (profile_name == "ccp"):
        ifc_queue = "CSR_BT_CCP_IFACEQUEUE"
        profile = "CCP"
    elif (profile_name == "vocs"):
        ifc_queue = "CSR_BT_VOCS_CLIENT_IFACEQUEUE"
        profile = "GVOCS"
    elif (profile_name == "aics"):
        ifc_queue = "CSR_BT_AICS_CLIENT_IFACEQUEUE"
        profile = "GAICS"

    #  Commmented one profiles need to verify before enabling
    # elif (lib_task_list[i][0] == profile_mapper["CSR_BT_PAC_IFACEQUEUE"]):
    # profile_data = apps1.fw.env.cast(lib_task_list[i][1],"GAscsC")
    # return(profile_data)

    # elif (lib_task_list[i][0] == profile_mapper["PAC_IFACEQUEUE_SEC_INSTANCE""]):
    # profile_data = apps1.fw.env.cast(lib_task_list[i][1],"GAscsC")
    # return(profile_data)

    elif (profile_name == "tbs"):
        ifc_queue = "CSR_BT_TBS_SERVER_IFACEQUEUE"
        profile = "GTBS_T"
    elif (profile_name == "mics"):
        ifc_queue = "CSR_BT_MICS_SERVER_IFACEQUEUE"
        profile = "GMICS_T"

    # elif(profile_name == "aics"):
    # ifc_queue = "CSR_BT_PAC_IFACEQUEUE"
    # profile = "GAICS"

    # elif(profile_name == "aics"):
    # ifc_queue = "PAC_IFACEQUEUE_SEC_INSTANCE"
    # profile = "GAICS"

    elif (profile_name == "tmas"):
        ifc_queue = "CSR_BT_TMAS_SERVER_IFACEQUEUE"
        profile = "GTMAS"
    elif (profile_name == "tmap"):
        ifc_queue = "CSR_BT_TMAP_CLIENT_IFACEQUEUE"
        profile = "TMAP"
    elif (profile_name == "tmasc"):
        ifc_queue = "CSR_BT_TMAS_CLIENT_IFACEQUEUE"
        profile = "GTMASC"

    elif (profile_name == "mcs"):
        ifc_queue = "CSR_BT_MCS_SERVER_IFACEQUEUE"
        profile = "GMCS_T"

    elif (profile_name == "cap"):
        ifc_queue = "CSR_BT_CAP_CLIENT_IFACEQUEUE"
        profile = "CapClientGroupInstance"

    elif (profile_name == "ascsc"):
        ifc_queue = "CSR_BT_ASCS_CLIENT_IFACEQUEUE"
        profile = "GAscsC"

    elif (profile_name == "pacsc"):
        ifc_queue = "CSR_BT_PACS_CLIENT_IFACEQUEUE"
        profile = "GPacsC"
    elif (profile_name == "csisc"):
        ifc_queue = "CSR_BT_CSIS_CLIENT_IFACEQUEUE"
        profile = "GCsisC"
    elif (profile_name == "bassc"):
        ifc_queue = "CSR_BT_BASS_CLIENT_IFACEQUEUE"
        profile = "GBASSC"
    elif (profile_name == "csip"):
        ifc_queue = "CSR_BT_CSIP_IFACEQUEUE"
        profile = "Csip"

    elif (profile_name == "vcp"):
        ifc_queue = "CSR_BT_VCP_IFACEQUEUE"
        profile = "VCP"
    # elif(profile_name == "ASCSC"):
    # ifc_queue = "CSR_BT_ASCS_CLIENT_IFACEQUEUE"
    # profile = "GAscsC"
    # elif(profile_name == "PACSC"):
    # ifc_queue = "CSR_BT_PACS_CLIENT_IFACEQUEUE"
    # profile = "GPacsC"

    # elif (profile_name == "vcs"):
    # ifc_queue = "CSR_BT_VCS_IFACEQUEUE"
    # profile = "GPacsC"
    elif (profile_name == "micp"):
        ifc_queue = "CSR_BT_MICP_IFACEQUEUE"
        profile = "MICP"
    elif (profile_name == "micsc"):
        ifc_queue = "CSR_BT_MICS_CLIENT_IFACEQUEUE"
        profile = "GMICSC"
    elif (profile_name == "gmas"):
        ifc_queue = "CSR_BT_GMAS_SERVER_IFACEQUEUE"
        profile = "GMAS"

    elif (profile_name == "gmap"):
        ifc_queue = "CSR_BT_GMAS_SERVER_IFACEQUEUE"
        profile = "GMAP"

    elif (profile_name == "pbp"):
        ifc_queue = "CSR_BT_PBP_IFACEQUEUE"
        profile = "PBP"

    try:
        for i in range(0, len(lib_task_list)):
            if lib_task_list[i][0] == profile_mapper[ifc_queue]:
                profile_data = apps1.fw.env.cast(lib_task_list[i][1], profile)
                return profile_data
    except Exception as e:
        print(str(e))
        pass
    return "None"


def __get_hf_conn_inst_svc_state_str(state):
    # Maximum number of HF service states per conn instance, this is dependent on HfServiceState_t. Needs to change if any changes are made to HfServiceState_t.
    MAX_HF_CONN_INST_SVC_STATES = 3

    if state.value < MAX_HF_CONN_INST_SVC_STATES:
        hf_conn_inst_svc_state = {0: "sdcState_s",
                                  1: "btConnect_s",
                                  2: "serviceConnect_s",
                                 }
        return str(hf_conn_inst_svc_state[state.value])+'('+ str(hex(state.value))+')'
    return "None"

def __get_hf_conn_inst_sub_state_str(state):
    # Maximum number of HF link states per conn instance.
    MAX_HF_CONN_INST_SUB_STATES = 5

    if state.value < MAX_HF_CONN_INST_SUB_STATES:
        hf_conn_inst_sub_state = {0: "Activate_s",
                                  1: "Connect_s",
                                  2: "Connected_s",
                                  3: "LpEnabled_s",
                                  4: "ServiceSearch_s",
                                 }
        return str(hf_conn_inst_sub_state[state.value])+'('+ str(hex(state.value))+')'
    return "None"

def __get_hf_conn_type_str(state):
    # Maximum number of HF/HS connection types.
    MAX_HF_HS_CONN_TYPE = 3

    if state.value < MAX_HF_HS_CONN_TYPE:
        hf_conn_inst_conn_type = {0: "CSR_BT_HF_CONNECTION_UNKNOWN",
                                  1: "CSR_BT_HF_CONNECTION_HF",
                                  2: "CSR_BT_HF_CONNECTION_HS",
                                 }
        return str(hf_conn_inst_conn_type[state.value])+'('+ str(hex(state.value))+')'
    return "None"

def __get_hfg_main_inst_state_str(state):
    # Maximum number of HFG Main instance states, this is dependent on HfgMainState_t. Needs to change if any changes are made to HfgMainState_t.
    MAX_HFG_MAIN_INST_STATES = 5

    if state.value < MAX_HFG_MAIN_INST_STATES:
        hfg_main_inst_state = {0: "MainNull_s",
                               1: "MainIdle_s",
                               2: "MainActive_s",
                               3: "MainDeactivate_s",
                               4: "MainNum_s"
                               }
        return str(hfg_main_inst_state[state.value]) + '(' + str(hex(state.value)) + ')'
    return "None"


def __get_hfg_conn_inst_state_str(state):
    # Maximum number of HFG Connection instance states, this is dependent on HfgConnectionState_t. Needs to change if any changes are made to HfgConnectionState_t.
    MAX_HFG_CONN_INST_STATES = 6

    if state.value < MAX_HFG_CONN_INST_STATES:
        hfg_conn_inst_state = {0: "Activate_s",
                               1: "Connect_s",
                               2: "ServiceSearch_s",
                               3: "Connected_s",
                               4: "ConnectionNum_s",
                               5: "Unused_s"
                               }
        return str(hfg_conn_inst_state[state.value]) + '(' + str(hex(state.value)) + ')'
    return "None"


def __get_hfg_at_state_str(atState):
    # Maximum number of AT SLC states, this is dependent on HfgAtState_t. Needs to change if any changes are made to HfgAtState_t.
    MAX_HFG_AT_SLC_STATES = 11

    if atState.value < MAX_HFG_AT_SLC_STATES:
        hfg_at_state = {0: "At0Idle_s",
                        1: "At1Brsf_s",
                        2: "At2Bac_s",
                        3: "At3CindQuestion_s",
                        4: "At4CindStatus_s",
                        5: "At5Cmer_s",
                        6: "At6ChldQuery_s",
                        7: "At7BindSupport_s",
                        8: "At8BindQuery_s",
                        9: "At9BindStatus_s",
                        10: "At10End_s"
                        }
        return str(hfg_at_state[atState.value]) + '(' + str(hex(atState.value)) + ')'
    return "None"
    
def __get_avrcp_sdp_state_str(sdpState):
    # Maximum number of SDP states the AVRCP main and connection instance can be in.
    MAX_AVRCP_SDP_STATES = 4
    
    if sdpState.value < MAX_AVRCP_SDP_STATES:
        avrcp_sdp_state = {0:"AVRCP_STATE_SDP_IDLE",
                           1:"AVRCP_STATE_SDP_ACTIVE",
                           2:"AVRCP_STATE_SDP_DONE",
                           3:"AVRCP_STATE_SDP_PENDING"
                           }
        return str(avrcp_sdp_state[sdpState.value])+'('+str(hex(sdpState.value))+')'
    return "None"
    
def __get_avrcp_app_state_str(appState):
    # Maximum number of APP states the AVRCP main instance can be in.
    MAX_AVRCP_APP_STATES = 3
    
    if appState.value < MAX_AVRCP_APP_STATES:
        avrcp_app_state = {0:"AVRCP_STATE_APP_IDLE",
                           1:"AVRCP_STATE_APP_BUSY",
                           2:"AVRCP_STATE_APP_NUM"
                           }
        return str(avrcp_app_state[appState.value])+'('+str(hex(appState.value))+')'
    return "None"
    
def __get_avrcp_channel_state_str(channelState):
    # Maximum number of channel connection state the AVRCP connection instance can be in.
    MAX_AVRCP_CHANNEL_STATES = 7
    
    if channelState.value < MAX_AVRCP_CHANNEL_STATES:
        avrcp_channel_state = {0:"AVRCP_STATE_CONN_DISCONNECTED",
                               1:"AVRCP_STATE_CONN_PENDING",
                               2:"AVRCP_STATE_CONN_CONNECTING",
                               3: "AVRCP_STATE_CONN_CONNECTED",
                               4: "AVRCP_STATE_CONN_DISCONNECTING",
                               5: "AVRCP_STATE_CONN_CANCELLING",
                               6: "AVRCP_STATE_CONN_DISC2RECONNECT"
                               }
        return str(avrcp_channel_state[channelState.value])+'('+str(hex(channelState.value))+')'
    return "None"
    
def __get_avrcp_conn_direction_str(connDirection):
    # Maximum number of directions the AVRCP connection can be in.
    MAX_AVRCP_CONN_DIRECTION_STATES = 3
    
    if connDirection.value < MAX_AVRCP_CONN_DIRECTION_STATES:
        avrcp_conn_direction = {0:"AVRCP_CONN_DIR_INVALID",
                                1:"AVRCP_CONN_DIR_INCOMING",
                                2:"AVRCP_CONN_DIR_OUTGOING"
                               }
        return str(avrcp_conn_direction[connDirection.value])+'('+str(hex(connDirection.value))+')'
    return "None"
