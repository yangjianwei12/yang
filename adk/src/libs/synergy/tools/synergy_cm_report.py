# --------------------------------------------------------------------------------------------------
# Copyright (c) 2022 Qualcomm Technologies International, Ltd
# --------------------------------------------------------------------------------------------------
#
# Generates different level of CM reports e.g. overview, detailed, individual modules etc.
#
# Usage:
# 1) Loading this script.
#    A) Script can be loaded at the time of attaching the device with the pydbg shell.
#       Below command can be used for loading this script.
#       --------------------------------------------------------------------------------
#       python pydbg.py -f apps1:"<path to earbud.elf>" <path to synergy_cm_report.py>
#       e.g.
#       python pydbg.py -f apps1:"C:\test\earbud.elf" C:\test\Synergy\tools\synergy_cm_report.py
#       --------------------------------------------------------------------------------
#    B) Script can also be used at the time of loading the coredump.
#       Use following command for the same:
#       --------------------------------------------------------------------------------
#       python pydbg.py -d xcd3:"<path to <coredump>.xcd>" -f apps1:"<path to earbud.elf>"
#       <path to synergy_cm_report.py>
#       e.g.
#       python pydbg.py -d xcd3:"C:\test\test_dump.xcd" -f apps1:"C:\test\earbud.elf"
#       C:\test\Synergy\tools\synergy_cm_report.py
#       --------------------------------------------------------------------------------
# 2) Using this script.
#    Use following command on pydbg terminal to get complete help regarding this script:
#    >>> synergy_cm_report()
#    Example command to fetch detailed CM report:
#    >>> synergy_cm_report.cm_detailed_report()
#    Follow help in order to get more information regarding other commands.
# --------------------------------------------------------------------------------------------------
from csr.dev.model.interface import Table


class synergy_cm_report:
    # Number of supported ACL connections, derived from configuration in csr_bt_cm_main.h
    num_supported_acl_connections = 3
    # Value of DM lockMsg when the queue is unlocked, derived from csr_bt_cm_main.h
    cm_dm_queue_unlock = 0xFFFF
    # mask for the identification of application and synergy tasks
    task_id_mask = 0xFF00
    # main cm object
    cm_data = apps1.fw.env.globalvars['csrBtCmData']
    # acl connection element
    acl_element = cm_data.roleVar.aclVar

    # dictionaries for states
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

    acl_role = {0: "CSR_BT_MASTER_ROLE",
                1: "CSR_BT_SLAVE_ROLE",
                2: "CSR_BT_UNDEFINED_ROLE"}

    def __init__(self):
        help_table = Table(['Command', 'Parameters', 'Usage'])
        help_table.add_row(['cm_report', 'None',
                            'This is used to get the overview of CM report with limited fields in tabular form'])
        help_table.add_row(['cm_detailed_report', 'None',
                            'Print raw data of different CM lists (e.g. L2CAP connection elements list)'])
        help_table.add_row(['cm_complete_report', 'None',
                            'This is the combination of commands cm_report and cm_detailed_report'])
        print("HELP:")
        print(help_table)
        print(
            "Example usage: \nsynergy_cm_report.cm_report()\n")

    @staticmethod
    def __get_device_address_hex(address):
        device_addr = hex(address.nap.value << 32 | address.uap.value << 24 | address.lap.value)
        return device_addr

    @staticmethod
    def __get_handler_from_index(index):
        handler = ""
        if index < synergy_cm_report.task_id_mask:
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
            my_index = index & ~synergy_cm_report.task_id_mask
            result_string = str(apps1.fw.env.cus["synergy_adapter.c"].localvars["trapTaskList"][my_index].deref)
            start_index = result_string.rfind("(")

            if start_index != -1:
                handler = result_string[start_index:]

        return handler

    @staticmethod
    def cm_acl_report(complete=False):
        if not complete:
            # print complete ACL report
            acl_table = Table(
                ["Bluetooth Device Address", "Role", "Incoming?", "Encryption Type", "ACL Requested by App?"])

            for i in range(synergy_cm_report.num_supported_acl_connections):
                device_addr = synergy_cm_report.__get_device_address_hex(synergy_cm_report.acl_element[i].deviceAddr)
                if device_addr != '0x0':
                    acl_table.add_row(
                        [device_addr, str(hex(synergy_cm_report.acl_element[i].role.value)) + " " + str(synergy_cm_report.acl_role[synergy_cm_report.acl_element[i].role.value]),
                         synergy_cm_report.acl_element[i].incoming,
                         synergy_cm_report.acl_element[i].encryptType,
                         synergy_cm_report.acl_element[i].aclRequestedByApp])

            print("\nACL CONNECTIONS\n")
            print(acl_table)
        else:
            # print ACL Connections overview
            print("\nACL CONNECTION ELEMENT LIST\n")
            for i in range(synergy_cm_report.num_supported_acl_connections):
                print(synergy_cm_report.acl_element[i])
                print("\n\n")

    @staticmethod
    def cm_l2cap_report(complete=False):
        if not complete:
            l2cap_table = Table(
                ["Device Address", "BT Connection ID", "Local PSM", "Remote PSM", "Application Handle", "Context",
                 "L2CAP State"])
            l2cap_element_pointer = synergy_cm_report.cm_data.l2caVar.connList.first
            while l2cap_element_pointer.value != 0:
                l2cap_element = apps1.fw.env.cast(l2cap_element_pointer, "cmL2caConnElement")
                l2cap_inst = apps1.fw.env.cast(l2cap_element.cmL2caConnInst, "cmL2caConnInstType")
                device_addr = synergy_cm_report.__get_device_address_hex(l2cap_inst.deviceAddr)
                handler = synergy_cm_report.__get_handler_from_index(l2cap_inst.appHandle.value)
                l2cap_table.add_row(
                    [device_addr, l2cap_inst.btConnId, l2cap_inst.psm, l2cap_inst.remotePsm,
                     str(hex(l2cap_inst.appHandle.value)) + " " + handler,
                     l2cap_inst.context, str(hex(l2cap_inst.state.value)) + " " + str(
                        synergy_cm_report.l2cap_state[l2cap_inst.state.value])])
                l2cap_element_pointer = l2cap_element.next

            print("\nL2CAP CONNECTIONS\n")
            print(l2cap_table)
        else:
            print("\nL2CAP Connection Element List\n")
            l2cap_element_pointer = synergy_cm_report.cm_data.l2caVar.connList.first
            while l2cap_element_pointer.value != 0:
                l2cap_element = apps1.fw.env.cast(l2cap_element_pointer, "cmL2caConnElement")
                print(l2cap_element)
                print("\n\n")
                l2cap_element_pointer = l2cap_element.next

    @staticmethod
    def cm_rfc_report(complete=False):
        if not complete:
            rfc_table = Table(
                ["Device Address", "BT Connection ID", "Local Server Channel", "Remote Server Channel", "dlci",
                 "Application Handle", "Context", "State"])
            rfc_element_pointer = synergy_cm_report.cm_data.rfcVar.connList.first
            while rfc_element_pointer.value != 0:
                rfc_element = apps1.fw.env.cast(rfc_element_pointer, "cmRfcConnElement")
                rfc_inst = apps1.fw.env.cast(rfc_element.cmRfcConnInst, "cmRfcConnInstType")
                device_addr = synergy_cm_report.__get_device_address_hex(rfc_inst.deviceAddr)
                handler = synergy_cm_report.__get_handler_from_index(rfc_inst.appHandle.value)
                rfc_table.add_row(
                    [device_addr, rfc_inst.btConnId, rfc_inst.serverChannel, rfc_inst.remoteServerChan, rfc_inst.dlci,
                     str(hex(rfc_inst.appHandle.value)) + " " + handler, rfc_inst.context, str(hex(rfc_inst.state.value)) + " " +
                     str(synergy_cm_report.rfc_state[rfc_inst.state.value])])
                rfc_element_pointer = rfc_element.next

            print("\nRFC CONNECTIONS\n")
            print(rfc_table)
        else:
            print("\nRFC Connection Element List\n")
            rfc_element_pointer = synergy_cm_report.cm_data.rfcVar.connList.first
            while rfc_element_pointer.value != 0:
                rfc_element = apps1.fw.env.cast(rfc_element_pointer, "cmRfcConnElement")
                print(rfc_element)
                print("\n\n")
                rfc_element_pointer = rfc_element.next

    @staticmethod
    def cm_le_report(complete=False):
        le_cache_pointer = synergy_cm_report.cm_data.leVar.connCache
        if not complete:
            le_table = Table(["Device Address", "Is Master?", "Connection Interval", "Connection Latency", "Supervision Timeout", "Clock Accuracy"])
            while le_cache_pointer.value != 0:
                le_inst = apps1.fw.env.cast(le_cache_pointer, "leConnVar")
                device_addr = synergy_cm_report.__get_device_address_hex(le_inst.addr.addr)
                le_table.add_row([device_addr, le_inst.master, le_inst.connParams.conn_interval,
                                  le_inst.connParams.conn_latency, le_inst.connParams.supervision_timeout,
                                  le_inst.connParams.clock_accuracy])
                le_cache_pointer = le_inst.next

            print("\nLE CONNECTIONS\n")
            print(le_table)
        else:
            print("\nLE Connections List\n")
            while le_cache_pointer.value != 0:
                le_inst = apps1.fw.env.cast(le_cache_pointer, "leConnVar")
                print(le_inst)
                print("\n\n")
                le_cache_pointer = le_inst.next

    @staticmethod
    def cm_dm_report():
        dm_table = Table(["Last Locked By", "Application Handle", "Is Locked?"])
        is_dm_locked = "FALSE"
        if synergy_cm_report.cm_data.dmVar.lockMsg.value != synergy_cm_report.cm_dm_queue_unlock:
            is_dm_locked = "TRUE"
        handler = synergy_cm_report.__get_handler_from_index(synergy_cm_report.cm_data.dmVar.appHandle.value)
        dm_table.add_row(
            [synergy_cm_report.cm_data.dmVar.lockMsg,
             str(hex(synergy_cm_report.cm_data.dmVar.appHandle.value)) + " " + handler, is_dm_locked])
        print("\nDM Queue\n")
        print(dm_table)

    @staticmethod
    def cm_sm_report():
        sm_table = Table(["Last Locked By", "Application Handle", "Is Locked?"])
        is_sm_locked = "FALSE"

        if synergy_cm_report.cm_data.smVar.smInProgress.value != 0:
            is_sm_locked = "TRUE"

        handler = synergy_cm_report.__get_handler_from_index(synergy_cm_report.cm_data.smVar.appHandle.value)
        sm_table.add_row(
            [synergy_cm_report.cm_data.smVar.smMsgTypeInProgress,
             str(hex(synergy_cm_report.cm_data.smVar.appHandle.value)) + " " + handler,
             is_sm_locked])

        print("\nSM Queue\n")
        print(sm_table)

    @staticmethod
    def cm_sdc_list(complete=False):
        sdc_list_pointer = synergy_cm_report.cm_data.sdcVar.sdcSearchList.first
        if not complete:
            sdc_table = Table(["Device Address", "Application Handle", "UUID Count", "Obtained Server Channels?"])
            while sdc_list_pointer.value != 0:
                sdc_element = apps1.fw.env.cast(sdc_list_pointer, "sdcSearchElement")
                device_addr = synergy_cm_report.__get_device_address_hex(sdc_element.deviceAddr)
                handler = synergy_cm_report.__get_handler_from_index(sdc_element.appHandle.value)
                sdc_table.add_row(
                    [device_addr, str(hex(sdc_element.appHandle.value)) + " " + handler, sdc_element.uuidCount,
                     sdc_element.obtainServerChannels])
                sdc_list_pointer = sdc_element.elem.next
            print("\nSDC Elements\n")
            print(sdc_table)
        else:
            print("\nSDC Element List\n")
            if sdc_list_pointer.value != 0:
                while sdc_list_pointer.value != 0:
                    sdc_element = apps1.fw.env.cast(sdc_list_pointer, "sdcSearchElement")
                    print(sdc_element)
                    print("\n\n")
                    sdc_list_pointer = sdc_element.elem.next
            else:
                print("<Empty>\n")

    @staticmethod
    def cm_pending_list():
        print("\nPending Message list\n")
        if synergy_cm_report.cm_data.pendingMsgs.value != 0:
            pending_list_pointer = synergy_cm_report.cm_data.pendingMsgs.deref.first
            while pending_list_pointer.value != 0:
                pending_list = apps1.fw.env.cast(pending_list_pointer, "cmPendingMsg_t")
                print(pending_list)
                pending_list_pointer = pending_list.elem.next
        else:
            print("<Empty>\n")

    @staticmethod
    def cm_report():
        print("\nCM REPORT")
        print("=========================================================================")
        # ACL connections
        synergy_cm_report.cm_acl_report(complete=False)
        # L2CAP connections
        synergy_cm_report.cm_l2cap_report(complete=False)
        # RFC connections
        synergy_cm_report.cm_rfc_report(complete=False)
        # LE connections
        synergy_cm_report.cm_le_report(complete=False)
        # Dm Queue
        synergy_cm_report.cm_dm_report()
        # SM Queue
        synergy_cm_report.cm_sm_report()
        # SDC List
        synergy_cm_report.cm_sdc_list(complete=False)
        print("=========================================================================")

    @staticmethod
    def cm_detailed_report():
        print("CM DETAILED REPORT\n")
        # l2cap report
        synergy_cm_report.cm_l2cap_report(complete=True)
        # rfc report
        synergy_cm_report.cm_rfc_report(complete=True)
        # le report
        synergy_cm_report.cm_le_report(complete=True)
        # SDC report
        synergy_cm_report.cm_sdc_list(complete=True)
        # Pending message list
        synergy_cm_report.cm_pending_list()

        print("Complete Connection Manager Data")
        print("=========================================================================")
        print(synergy_cm_report.cm_data)
        print("=========================================================================")

    @staticmethod
    def cm_complete_report():
        print("CM Complete Report\n")
        synergy_cm_report.cm_report()
        synergy_cm_report.cm_detailed_report()

### must end with the following line
go_interactive(locals())
