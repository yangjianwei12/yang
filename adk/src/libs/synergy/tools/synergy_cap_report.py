# --------------------------------------------------------------------------------------------------
# Copyright (c) 2022 Qualcomm Technologies International, Ltd
# --------------------------------------------------------------------------------------------------
#
# Generates different level of CAP reports e.g. overview, detailed, individual modules etc.
#
# Usage:
# 1) Loading this script.
#    A) Script can be loaded at the time of attaching the device with the pydbg shell.
#       Below command can be used for loading this script.
#       --------------------------------------------------------------------------------
#       python pydbg.py -f apps1:"<path to earbud.elf>" <path to synergy_cap_report.py>
#       e.g.
#       python pydbg.py -f apps1:"C:\test\earbud.elf" C:\test\Synergy\tools\synergy_cap_report.py
#       --------------------------------------------------------------------------------
#    B) Script can also be used at the time of loading the coredump.
#       Use following command for the same:
#       --------------------------------------------------------------------------------
#       python pydbg.py -d xcd3:"<path to <coredump>.xcd>" -f apps1:"<path to earbud.elf>"
#       <path to synergy_cap_report.py>
#       e.g.
#       python pydbg.py -d xcd3:"C:\test\test_dump.xcd" -f apps1:"C:\test\earbud.elf"
#       C:\test\Synergy\tools\synergy_cap_report.py
#       --------------------------------------------------------------------------------
# 2) Using this script.
#    Use following command on pydbg terminal to get complete help regarding this script:
#    >>> synergy_cap_report()
#    Example command to fetch detailed CAP report:
#    >>> synergy_cap_report.cap_complete_report()
#    Follow help in order to get more information regarding other commands.
# --------------------------------------------------------------------------------------------------
from csr.dev.model.interface import Table

# dictionaries for states
cap_client_role = {0x01:"INITIATOR",
                   0x02:"COMMANDER",
                   0x03:"INITIATOR_COMMANDER"}

cap_client_state = {0x00: "CAP_INVALID",
                   0x01: "CAP_INIT",
                   0x02: "CAP_INIT_STREAM_CTRL",
                   0x03: "CAP_DISCOVER_SUPPORTED",
                   0x04: "CAP_UNICAST_CONNECTED",
                   0x05: "CAP_STREAM_STARTED",
                   0x06: "CAP_STREAM_STOPPED",
                   0x07: "CAP_AUDIO_UPDATED",
                   }

cap_discovery_pending = {0x00:"DISCOVERY_COMPLETED",
                   0x01:"DISCOVERY_PAC_RECORD",
                   0x02:"DISCOVERY_AUDIO_LOC",
                   0x04:"DISCOVERY_SUPPORTED_CONTEXT ",
                   0x08:"DISCOVERY_ASE_STATE ",
                   0x0f:"DISCOVERY_ALL"}

cap_client_operation = {0x00000000: "OP_NONE",
                       0x00000001: "OP_DISCOVER_AUDIO_CAPABILITY ",
                       0x00000002: "OP_UNICAST_CONNECT",
                       0x00000003: "OP_DISCOVER_AUDIO_CONTEXT",
                       0x00000004: "OP_UNICAST_START_STREAM",
                       0x00000005: "OP_BROADCAST_SRC",
                       0x00000006: "OP_BROADCAST_ASSISTANT",
                       0x00000007: "OP_ADD_PAC_RECORD",
                       0x00000008: "OP_SET_VOL",
                       0x00000009: "OP_MUTE_VOL",
                       0x0000000a: "OP_CSIP_CLEANUP",
                       0x0000000b: "OP_UNICAST_AUDIO_UPDATE",
                       0x0000000c: "OP_VCP_INIT",
                       0x0000000d:"OP_UNICAST_DISABLE",
                       0x0000000e: "OP_UNICAST_RELEASE",
                       0x0000000f: "OP_BASS_SCAN_START ",
                       0x00000010: "OP_BASS_SYNC_START ",
                       0x00000011: "OP_BASS_SYNC_TERMINATE ",
                       0x00000012:"OP_BASS_SYNC_CANCEL",
                       0x00000013:"OP_BASS_ADD_SRC",
                       0x00000014: "OP_BASS_MODIFY_SRC",
                       0x00000015: "OP_BASS_REMOVE_SRC ",
                       0x00000016: "OP_BASS_REG_NOTIFY ",
                       0x00000017: "OP_BASS_READ_BRS",
                       0x00000018: "OP_BASS_SCAN_STOP  ",
                       0x00000019: "OP_BASS_SET_CODE ",
                       0x0000001A: "OP_CSIP_READ_LOCK ",
                       0x0000001B: "OP_UNICAST_DISCONNECT",
                       0x0000001C: "OP_DEV_INIT   ",
                       0x0000001D: "OP_MICP_INIT  ",
                       0x0000001E: "OP_MICP_MUTE ",
                       }

cap_client_usecase = {0x0000: "CONTEXT_NA",
                       0x0001: "CONTEXT_UNSPECIFIED",
                       0x0002: "CONTEXT_CONVERSATIONAL",
                       0x0004: "CONTEXT_MEDIA",
                       0x0008: "CONTEXT_GAME",
                       0x0010: "CONTEXT_INSTRUCTIONAL",
                       0x0020: "CONTEXT_VOICE_ASSISTANT",
                       0x0040: "CONTEXT_LIVE",
                       0x0080: "CONTEXT_SOUND_EFFECTS",
                       0x0100: "CONTEXT_NOTIFICATIONS",
                       0x0200: "CONTEXT_RINGTONE",
                       0x0400: "CONTEXT_ALERTS",
                       0x0800: "CONTEXT_EMERGENCY_ALARM",
                       0x8000:"CONTEXT_GAME_WITH_VBC"
                   }

cap_client_ase_state = {0x00: "NA",
                       0x01: "CODEC_CONFIGURED",
                       0x02: "QOS_CONFIGURED",
                       0x03: "ENABLING ",
                       0x04: "STREAMING ",
                       0x05: "DISABLING ",
                       0x06: "RELEASING ",
                       0x07: "INVALID",
                   }

cap_client_datapath_dir = {0x0000:"NA",
                   0x0001:"INPUT ",
                   0x0002:"OUTPUT"}

cap_client_streaming = {0x0000:"NOT_STREAMING",
                   0x0001:"STREAMING "}

cap_client_csis = {0x0000:"NON_CSIS",
                   0x0001:"CSIS_DEV_COUNT_1",
                   0x0002:"CSIS_DEV_COUNT_2",
                   0x0003:"CSIS_DEV_COUNT_3",
                   0x0004:"CSIS_DEV_COUNT_4",
                   0x0005:"CSIS_DEV_COUNT_5"}

class synergy_cap_report:
    temp = apps1.fw.env.vars["capInstanceMain"]
    # main cap object
    cap_instance_data = apps1.fw.env.cast( temp ,"CAP_INST")

    def __init__(self):
        help_table = Table(['Command', 'Parameters', 'Usage'])
        help_table.add_row(['cap_report', 'None',
                            'This is used to get the overview of CAP report with limited fields in tabular form'])
        help_table.add_row(['cap_detailed_report', 'None',
                            'Print raw data of different CAP lists (e.g. L2CAP connection elements list)'])
         
        print("HELP:")
        print(help_table)
        print(
            "Example usage: \nsynergy_cm_report.cap_client_report(True)\nsynergy_cap_report.cap_report()\nsynergy_cap_report()\n")

    def __active_group_id():
        return cap_instance_data.activeGroupId

    @staticmethod
    def cap_instance_report(complete=False):
        if not complete:
            # print complete ACL report
            cap_table = Table(
                ["Active Group Id","Device Count", "BAP Count(Pending Op)", "VCP Count(Pending Op)", "CSIP Count(Pending Op)", "MICP Count(Pending Op)","CIS Count(Pending Op)","Stream State", "Is Sink"])
            
            synergy_cap_report.__active_group_id = classmethod(synergy_cap_report.__active_group_id)
            #active_group = synergy_cap_report.__active_group_id()
            if synergy_cap_report.__active_group_id != 0x00:
                    cap_table.add_row(
                        [synergy_cap_report.cap_instance_data.activeGroupId,synergy_cap_report.cap_instance_data.deviceCount, synergy_cap_report.cap_instance_data.bapRequestCount,synergy_cap_report.cap_instance_data.vcpRequestCount,synergy_cap_report.cap_instance_data.csipRequestCount,
                        synergy_cap_report.cap_instance_data.micpRequestCount,synergy_cap_report.cap_instance_data.cisReqCount, cap_client_streaming[int(str(synergy_cap_report.cap_instance_data.streaming),16)], synergy_cap_report.cap_instance_data.isSink])

            print("\nCAP INSTANCE REPORT\n")
            print(cap_table)
        else:
            # print ACL Connections overview
            print("\n Need to check CAP Report\n")

    @staticmethod
    def cap_group_report(complete=False):
        if not complete:
            temp = apps1.fw.env.vars["capInstanceMain"]
            cap_instance_data = apps1.fw.env.cast( temp ,"CAP_INST")
            synergy_cap_report.__active_group_id = classmethod(synergy_cap_report.__active_group_id)
            # Get all the service instance for the profiles  
            serive_handle_object = apps1.fw.env.cast(apps1.fw.var.service_handle_data.deref.instances.address, 'uint32', array_len=11)
            # Type of a <class 'csr.dev.env.env_helpers._Array'>
            # Convert it into list
            serive_handle_object = list(serive_handle_object)
            #print(synergy_cap_report.__active_group_id)
            cap_table = Table(
                       ["Group Id","CAP role", "Requested CID","Set Size", "Use Case","CAP State", "CIS Count" , "SRC ASE Count", "CAP Pending capability" ,"Pending Cap Op"])
            for i in range(len(serive_handle_object)):
                temp = serive_handle_object[i]
                profile_Address = apps1.fw.env.cast(int(str(serive_handle_object[i]),16),"CapClientGroupInstance")
                found_cap = 0;
                if (int(str(profile_Address.groupId),16) == (int(str(cap_instance_data.activeGroupId),16))):      
                    found_cap = 1
                    cap_table.add_row(
                        [profile_Address.groupId,cap_client_role[int(str(profile_Address.role),16)], profile_Address.requestCid,cap_client_csis[int(str(profile_Address.setSize),16)],cap_client_usecase[int(str(profile_Address.useCase),16)],cap_client_state[int(str(profile_Address.capState),16)], profile_Address.totalCisCount,profile_Address.numOfSourceAses,
                         cap_discovery_pending[int(str(profile_Address.pendingCap),16)],cap_client_operation[int(str(profile_Address.pendingOp),16)]])
                    break
            
            print("\nCAP Active GROUP REPORT:\n")
            print(cap_table)
        else:
            # print ACL Connections overview
            print("\n Need to check CAP Report\n")

    @staticmethod
    def cap_group_bap_report(complete=False):
        if not complete:
            temp = apps1.fw.env.vars["capInstanceMain"]
            cap_instance_data = apps1.fw.env.cast( temp ,"CAP_INST")
            synergy_cap_report.__active_group_id = classmethod(synergy_cap_report.__active_group_id)
            # Get all the service instance for the profiles  
            serive_handle_object = apps1.fw.env.cast(apps1.fw.var.service_handle_data.deref.instances.address, 'uint32', array_len=11)
            # Type of a <class 'csr.dev.env.env_helpers._Array'>
            # Convert it into list
            serive_handle_object = list(serive_handle_object)
            bap_table = Table(
                       ["Bap Handle", "Bap Status","Sink Ase Count","Src Ase Count", "ASE in Use", "Released Ases", "CIG Id","CIS Count", "Data Path Req Count","serverSourceStreamCount"])
            for i in range(len(serive_handle_object)):
                temp = serive_handle_object[i]
                cap = apps1.fw.env.cast(int(str(serive_handle_object[i]),16),"CapClientGroupInstance")
                found_cap = 0;
                if (int(str(cap.groupId),16) == (int(str(cap_instance_data.activeGroupId),16))):      
                    temp = int(str(cap.bapList.first),16)
                    bap=apps1.fw.env.cast(temp,"BapInstElement")
                    for i in range(int(str(cap.bapList.count),16)):
                        bap_table.add_row(
                            [bap.bapHandle, bap.recentStatus, bap.sinkAseList.count,bap.sourceAseList.count, bap.asesInUse, bap.releasedAses, bap.cigId, bap.cisCount, bap.datapathReqCount, bap.serverSourceStreamCount])
                    break
            
            print("\nBAP: \n")
            print(bap_table)
        else:
            # print ACL Connections overview
            print("\n Need to check CAP Report\n")

    @staticmethod
    def cap_group_bap_ase_report(complete=False):
        if not complete:
            temp = apps1.fw.env.vars["capInstanceMain"]
            cap_instance_data = apps1.fw.env.cast( temp ,"CAP_INST")
            synergy_cap_report.__active_group_id = classmethod(synergy_cap_report.__active_group_id)
            # Get all the service instance for the profiles  
            serive_handle_object = apps1.fw.env.cast(apps1.fw.var.service_handle_data.deref.instances.address, 'uint32', array_len=11)
            # Type of a <class 'csr.dev.env.env_helpers._Array'>
            # Convert it into list
            serive_handle_object = list(serive_handle_object)
            #"Sink ASE Id's(CIS Handle,UseCase,Dir)"
            #"SRC ASE Id's(CIS Handle,UseCase,Dir)"
            bap_sink_ase_table = Table(["Sink ASE Id's(ASE ID,CIS ID,State)"])
            bap_sink_ase_table1 = Table(["Sink ASE Id's(CIS Handle, UseCase, Dir)"])
            bap_src_ase_table = Table(["SRC ASE Id's(ASE ID, CIS ID, State)"])
            bap_src_ase_table1 = Table(["SRC ASE Id's(CIS Handle, UseCase, Dir)"])
            bap_sink_ases_table = Table(["Sink ASE ID","CIS ID", "CIS Handle","UseCase","ASE State", "Datapath(Direction)"])
            bap_src_ases_table = Table(["Src ASE ID","CIS ID", "CIS Handle","UseCase","ASE State", "Datapath(Direction)"])
            for i in range(len(serive_handle_object)):
                temp = serive_handle_object[i]
                cap = apps1.fw.env.cast(int(str(serive_handle_object[i]),16),"CapClientGroupInstance")
                found_cap = 0;
                if (int(str(cap.groupId),16) == (int(str(cap_instance_data.activeGroupId),16))):      
                    temp = int(str(cap.bapList.first),16)
                    bap=apps1.fw.env.cast(temp,"BapInstElement")
                    for i in range(int(str(cap.bapList.count),16)):
                        bapSinkAseList = []
                        bapSrcAseList = []
                        bapSinkAseList1 = []
                        bapSrcAseList1 = []
                        tempSink = int(str(bap.sinkAseList.first),16)
                        tempSrc = int(str(bap.sourceAseList.first),16)
                        #get sink ase info
                        for i in range(int(str(bap.sinkAseList.count),16)):
                            #temp = int(str(bap.sinkAseList.first),16)
                            bapSink=apps1.fw.env.cast(tempSink,"BapAseElement")
                            bapSinkAseList.append((bapSink.aseId,bapSink.cisId, cap_client_ase_state[int(str(bapSink.state),16)]))
                            bapSinkAseList1.append((bapSink.cisHandle,cap_client_usecase[int(str(bapSink.useCase),16)], cap_client_datapath_dir[int(str(bapSink.datapathDirection),16)]))
                            bap_sink_ases_table.add_row([bapSink.aseId,bapSink.cisId, bapSink.cisHandle, cap_client_usecase[int(str(bapSink.useCase),16)],cap_client_ase_state[int(str(bapSink.state),16)], cap_client_datapath_dir[int(str(bapSink.datapathDirection),16)]])
                            tempSink = bapSink.next
                        #get src ase info
                        for i in range(int(str(bap.sourceAseList.count),16)):
                            bapSrc=apps1.fw.env.cast(tempSrc,"BapAseElement")
                            bapSrcAseList.append((bapSrc.aseId,bapSrc.cisId,cap_client_ase_state[int(str(bapSrc.state),16)]))
                            bapSrcAseList1.append((bapSrc.cisHandle,cap_client_usecase[int(str(bapSrc.useCase),16)], cap_client_datapath_dir[int(str(bapSrc.datapathDirection),16)]))
                            bap_src_ases_table.add_row([bapSrc.aseId,bapSrc.cisId, bapSrc.cisHandle,cap_client_usecase[int(str(bapSrc.useCase),16)], cap_client_ase_state[int(str(bapSrc.state),16)], cap_client_datapath_dir[int(str(bapSrc.datapathDirection),16)]])
                            tempSrc = bapSrc.next
                            bap_sink_ase_table.add_row(
                                [bapSinkAseList])
                            bap_sink_ase_table1.add_row(
                                [bapSinkAseList1])
                            bap_src_ase_table.add_row(
                                [bapSinkAseList])
                            bap_src_ase_table1.add_row(
                                [bapSinkAseList1])
                    break
        
            print("\nSINK/SRC ASE's: \n")
            print(bap_sink_ases_table)
            print(bap_src_ases_table)
            #print(bap_sink_ase_table)
            #print(bap_sink_ase_table1)
            #print(bap_src_ase_table)
            #print(bap_src_ase_table1)
        else:
            # print ACL Connections overview
            print("\n Need to check CAP Report\n")
    @staticmethod
    def cap_complete_report():
        print("\nCAP Complete REPORT")
        print("=============================================================================================================================================================")

        synergy_cap_report.cap_instance_report(complete=False)
        synergy_cap_report.cap_group_report(complete=False)
        synergy_cap_report.cap_group_bap_report(complete=False)
        synergy_cap_report.cap_group_bap_ase_report(complete=False)
        print("=============================================================================================================================================================")

### must end with the following line
go_interactive(locals())
