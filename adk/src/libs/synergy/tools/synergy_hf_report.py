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
#       python pydbg.py -f apps1:"<path to earbud.elf>" <path to synergy_hf_report.py>
#       e.g.
#       python pydbg.py -f apps1:"C:\test\earbud.elf" C:\test\Synergy\tools\synergy_hf_report.py
#       --------------------------------------------------------------------------------
#    B) Script can also be used at the time of loading the coredump.
#       Use following command for the same:
#       --------------------------------------------------------------------------------
#       python pydbg.py -d xcd3:"<path to <coredump>.xcd>" -f apps1:"<path to earbud.elf>"
#       <path to synergy_hf_report.py>
#       e.g.
#       python pydbg.py -d xcd3:"C:\test\test_dump.xcd" -f apps1:"C:\test\earbud.elf"
#       C:\test\Synergy\tools\synergy_hf_report.py
#       --------------------------------------------------------------------------------
# 2) Using this script.
#    Use following command on pydbg terminal to get complete help regarding this script:
#    >>> synergy_hf_report()
#    Example command to fetch detailed HF report:
#    >>> synergy_hf_report.hf_main_inst_report()
#    Follow help in order to get more information regarding other commands.
# --------------------------------------------------------------------------------------------------
from csr.dev.model.interface import Table

class synergy_hf_report:
    # main hf object
    hf_main_inst = apps1.fw.env.globalvars['csrBtHfInstance']

    def __init__(self):
        help_table = Table(['Command', 'Parameters', 'Usage'])
        help_table.add_row(['hf_main_inst_report', 'None', 'Get HF main instance info with limited fields in tabular form'])
        help_table.add_row(['hf_conn_inst_report', 'None', 'Get each HF/HS connection instance info with limited fields in tabular form'])
        help_table.add_row(['hf_save_queue_report', 'None', 'Get the saved messages in the HF save queue'])
        help_table.add_row(['hf_report', 'None', 'Get detailed info of HF instances'])

        print("HELP:")
        print(help_table)
        print("Example usage: \nsynergy_hf_report.hf_main_inst_report()\n")

    @staticmethod
    def __get_device_address_hex(address):
        device_addr = hex(address.nap.value << 32 | address.uap.value << 24 | address.lap.value)
        return device_addr

    @staticmethod
    def hf_main_inst_report():
        print("\nHf Main Instance Info\n")
        hf_main_table = Table(["appHandle", "currentDeviceAddress", "index",
                               "state", "localSupportedFeatures", "hfServerChannel",
                               "hsServerChannel", "maxHFConnections", "maxHSConnections",
                               "maxTotalSimultaneousConnections", "atRespWaitTime"])

        device_addr = synergy_hf_report.__get_device_address_hex(synergy_hf_report.hf_main_inst.currentDeviceAddress)
        hf_main_table.add_row(
                   [synergy_hf_report.hf_main_inst.appHandle, device_addr, synergy_hf_report.hf_main_inst.index,
                    synergy_hf_report.hf_main_inst.state, synergy_hf_report.hf_main_inst.localSupportedFeatures, synergy_hf_report.hf_main_inst.hfServerChannel,
                    synergy_hf_report.hf_main_inst.hsServerChannel, synergy_hf_report.hf_main_inst.maxHFConnections, synergy_hf_report.hf_main_inst.maxHSConnections,
                    synergy_hf_report.hf_main_inst.maxTotalSimultaneousConnections, synergy_hf_report.hf_main_inst.atRespWaitTime])

        print(hf_main_table)

    @staticmethod
    def hf_conn_inst_report():
        print("\nHf Connections Instance Info\n")
        maxHFConnections = int(synergy_hf_report.hf_main_inst.maxHFConnections.value)
        maxHSConnections = int(synergy_hf_report.hf_main_inst.maxHSConnections.value)
        tot_number = maxHFConnections + maxHSConnections

        hf_conn_table1 = Table(["currentDeviceAddress", "hfConnId", "supportedFeatures", "remoteVersion", "scoHandle", "instId",
                                "lastAtCmdSent", "atSequenceState", "serviceState", "linkState", "state", "audioPending", "codecToUse"])
        hf_conn_table2 = Table(["linkType", "lastAudioReq", "searchOngoing", "incomingSlc"])

        for i in range(tot_number):
            device_addr = synergy_hf_report.__get_device_address_hex(synergy_hf_report.hf_main_inst.linkData[i].currentDeviceAddress)
            hf_conn_table1.add_row(
                        [device_addr,
                         synergy_hf_report.hf_main_inst.linkData[i].hfConnId, synergy_hf_report.hf_main_inst.linkData[i].supportedFeatures,
                         synergy_hf_report.hf_main_inst.linkData[i].remoteVersion, synergy_hf_report.hf_main_inst.linkData[i].scoHandle,
                         synergy_hf_report.hf_main_inst.linkData[i].instId, synergy_hf_report.hf_main_inst.linkData[i].lastAtCmdSent,
                         synergy_hf_report.hf_main_inst.linkData[i].atSequenceState, synergy_hf_report.hf_main_inst.linkData[i].serviceState,
                         synergy_hf_report.hf_main_inst.linkData[i].linkState, synergy_hf_report.hf_main_inst.linkData[i].state,
                         synergy_hf_report.hf_main_inst.linkData[i].audioPending, synergy_hf_report.hf_main_inst.linkData[i].codecToUse])
            hf_conn_table2.add_row(
                        [synergy_hf_report.hf_main_inst.linkData[i].linkType, synergy_hf_report.hf_main_inst.linkData[i].lastAudioReq,
                         synergy_hf_report.hf_main_inst.linkData[i].searchOngoing, synergy_hf_report.hf_main_inst.linkData[i].incomingSlc])
        print("\nCONNECTIONS INFO\n")
        print(hf_conn_table1)
        print(hf_conn_table2)

    @staticmethod
    def hf_save_queue_report():
        print("\nSaved Message list\n")
        if synergy_hf_report.hf_main_inst.saveQueue.value != 0:
            save_queue_ptr = apps1.fw.env.cast(synergy_hf_report.hf_main_inst.saveQueue, "CsrMessageQueueType")
            while save_queue_ptr.value != 0:
                print(save_queue_ptr)
                save_queue_ptr = save_queue_ptr.nextQueuePool
        else:
            print("<Empty>\n")

    @staticmethod
    def hf_report():
        print("HF Report\n")
        synergy_hf_report.hf_main_inst_report()
        synergy_hf_report.hf_conn_inst_report()
        synergy_hf_report.hf_save_queue_report()

### must end with the following line
go_interactive(locals())
