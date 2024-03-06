# --------------------------------------------------------------------------------------------------
# Copyright (c) 2023 Qualcomm Technologies International, Ltd
# --------------------------------------------------------------------------------------------------
#
# This script parses the XML schema provided in report_desc.xml to generate reports of
# different synergy modules as per the schema.
#
# Usage:
# 1) Loading this script.
#    A) Script can be loaded at the time of attaching the device with the pydbg shell.
#       Below command can be used for loading this script.
#       --------------------------------------------------------------------------------
#       python pydbg.py -f apps1:"<path to earbud.elf>" <path to report_gen.py>
#       e.g.
#       python pydbg.py -f apps1:"C:\test\earbud.elf" C:\test\Synergy\tools\report_gen.py
#       --------------------------------------------------------------------------------
#    B) Script can also be used at the time of loading the coredump.
#       Use following command for the same:
#       --------------------------------------------------------------------------------
#       python pydbg.py -d xcd3:"<path to <coredump>.xcd>" -f apps1:"<path to earbud.elf>"
#       <path to report_gen.py>
#       e.g.
#       python pydbg.py -d xcd3:"C:\test\test_dump.xcd" -f apps1:"C:\test\earbud.elf"
#       C:\test\Synergy\tools\report_gen.py
#       --------------------------------------------------------------------------------
# 2) Using this script.
#    Script is located at adk/src/libs/synergy/tools/synergy_report_tool/report_gen.py
#
#    Following commands can be used to generate reports.
#    Overview report:
#    >>> report_gen.generate_overview_report()
#
#    Detailed report:
#    >>> report_gen.generate_detailed_report()
#
# Note: more details regarding the script and XML schema format is available in readme.txt file
#       located in the same folder as this script.
# --------------------------------------------------------------------------------------------------
import xml.etree.ElementTree as ET
from csr.dev.model.interface import Table
import os


class report_gen:
    file_name = os.path.join(os.path.dirname(__file__), 'report_desc.xml')
    preprocess_file_path = os.path.join(os.path.dirname(__file__), 'report_pre.py')
    global_command_prefix = 'apps1.fw.env.globalvars'
    enter = "\n"
    tab = '    '

    def __init__(self):
        print('------ Report Generation Tool ------')
        # TODO add help here.

    def __print_global_simple_member(var_name, member_name):
        command = "var =" + report_gen.global_command_prefix + "['" + var_name + "']." + member_name
        exec(command + report_gen.enter + "print(var)")

    def __prepare_global_variable_statement(global_var_name, global_type, global_file_name):
        if global_type != 'static':
            command = 'global_var = apps1.fw.env.globalvars[' + '"' + global_var_name + '"]' + report_gen.enter
        else:
            command = 'global_var = apps1.fw.env.cus[' + '"' + global_file_name + '"].localvars[' + '"' + global_var_name + '"]' + report_gen.enter
        return command

    def __get_num_columns(table):
        total_columns = 0
        for column in table:
            total_columns = total_columns + 1
        return total_columns

    def __prepare_table_header(table, total_columns):
        command = 'table = Table' + '(['
        for column in table:
            command = command + '"' + column.get('title', default='') + '"'
            if total_columns > 0:
                total_columns = total_columns - 1
                command = command + ','
        command = command + '])' + report_gen.enter
        return command

    def __insert_preprocessing_code():
        file = open(report_gen.preprocess_file_path, "r")
        command = file.read()
        return command

    def __preprocess_data(column_type, column_obj, column_var):
        command = ''
        if column_type == 'CsrBtDeviceAddr':
            command = '__get_device_address_hex(' + column_obj + column_var + ')'
        elif column_type == 'CsrBtCmStateL2cap':
            command = '__get_l2cap_state_str(' + column_obj + column_var + ')'
        elif column_type == 'CsrBtCmStateRfc':
            command = '__get_rfc_state_str(' + column_obj + column_var + ')'
        elif column_type == 'TaskHandler':
            command = 'str(hex(' + column_obj + column_var + '.value))' + ' + ' + '__get_handler_from_app_handle(' + column_obj + column_var + ')'
        elif column_type == 'HfgMainState_t':
            command = '__get_hfg_main_inst_state_str(' + column_obj + column_var + ')'
        elif column_type == 'HfgConnectionState_t':
            command = '__get_hfg_conn_inst_state_str(' + column_obj + column_var + ')'
        elif column_type == 'HfgAtState_t':
            command = '__get_hfg_at_state_str(' + column_obj + column_var + ')'
        elif column_type == 'pacs_data':
            command = '__get_pacs_data(' + column_obj + column_var + ')'
        elif column_type == 'gatt_ascs_connection':
            command = '__get_gatt_ascs_connection(' + column_obj + column_var + ')'
        elif column_type == 'gatt_bass_server_ccc_data_t':
            command = '__get_gatt_bass_server_ccc_data_t(' + column_obj + column_var + ')'
        elif column_type == 'gatt_mics_client_data':
            command = '__get_gatt_mics_client_data(' + column_obj + column_var + ')'
        elif column_type == 'mcp_mcs_srvc_hndl':
            command = '__get_mcp_mcs_srvc_hndl_data(' + column_obj + column_var + ')'
        elif column_type == 'HfStates_t':
            command = '__get_hf_conn_inst_sub_state_str(' + column_obj + column_var + ')'
        elif column_type == 'CsrBtHfConnectionType':
            command = '__get_hf_conn_type_str(' + column_obj + column_var + ')'
        elif column_type == 'ServiceStateType':
            command = '__get_hf_conn_inst_svc_state_str(' + column_obj + column_var + ')'
        elif column_type == 'avrcpSdpState':
            command = '__get_avrcp_sdp_state_str(' + column_obj + column_var + ')'
        elif column_type == 'avrcpAppState':
            command = '__get_avrcp_app_state_str(' + column_obj + column_var + ')'
        elif column_type == 'avrcpChannelState':
            command = '__get_avrcp_channel_state_str(' + column_obj + column_var + ')'
        elif column_type == 'avrcpConnDirection':
            command = '__get_avrcp_conn_direction_str(' + column_obj + column_var + ')'
        else:
            command = column_obj + column_var
        return command

    def __insert_tab(level):
        tabs = ''
        for i in range(level):
            tabs += report_gen.tab
        return tabs

    def __insert_try_block(level):
        return report_gen.enter + report_gen.__insert_tab(level) + 'try:' + report_gen.enter

    def __insert_except_block(level):
        except_block = report_gen.enter + report_gen.__insert_tab(level) + 'except Exception as e:'
        except_block += report_gen.enter + report_gen.__insert_tab(level) + report_gen.tab + 'print(str(e))'
        except_block += report_gen.enter + report_gen.__insert_tab(level) + report_gen.tab + 'pass' + report_gen.enter
        return except_block

    @staticmethod
    def generate_overview_report():
        print(report_gen.file_name)
        with open(report_gen.file_name) as my_file:
            tree = ET.parse(my_file)
            report_desc = tree.getroot()
            for report in report_desc:
                if report.tag == 'REPORT':
                    report_title = report.get('title', default='')
                    report_type = report.get('report_type', default='')
                    global_var_name = report.get('global_name', default='')
                    global_type = report.get('global_type', default='')
                    global_file_name = report.get('global_file_name', default='')

                    print("\n############################## " + report_title + " ##############################\n")

                    for table in report:
                        if table.tag == 'TABLE':
                            data_type = table.get('data_type', default='')

                            # Generating header of the table
                            command = ''
                            command += report_gen.__insert_preprocessing_code() + report_gen.enter

                            command += report_gen.__insert_try_block(0)

                            if report_type != 'lea_report':
                                command += report_gen.__insert_tab(1) + report_gen.__prepare_global_variable_statement(
                                    global_var_name, global_type,
                                    global_file_name)

                            table_title = table.get('title', default='')
                            total_columns = report_gen.__get_num_columns(table)
                            print("------------- " + table_title + " -------------")
                            command += report_gen.__insert_try_block(1)
                            command += report_gen.__insert_tab(2) + report_gen.__prepare_table_header(table,
                                                                                                      total_columns - 1)

                            # Generating data of the table, based on what is the data_type
                            if data_type == 'regular':
                                temp_columns = total_columns - 1
                                column_obj = 'global_var.'
                                command += report_gen.__insert_tab(2) + 'table.add_row(['
                                for column in table:
                                    column_data_type = column.get('data_type', default='')
                                    column_var = column.get('name', default='')
                                    command += report_gen.__preprocess_data(column_data_type, column_obj,
                                                                            column_var)
                                    if temp_columns > 0:
                                        temp_columns = temp_columns - 1
                                        command = command + ","
                                command += '])' + report_gen.enter
                                command += report_gen.__insert_tab(2) + "print(table)" + report_gen.enter
                            elif data_type == 'Instanced_CsrCmnListSimple_t' or data_type == 'CsrCmnListSimple_t' or data_type == 'CsrCmnListSimple_t_ptr' or data_type == 'linked_list' or data_type == 'linked_list_ptr':
                                list_ptr = table.get('name', default='')
                                element_type = table.get('lvl1_element_type', default='')
                                deref = '' if data_type != 'CsrCmnListSimple_t_ptr' and data_type != 'linked_list_ptr' else '.deref'
                                first = '' if data_type == 'linked_list_ptr' else '.first'
                                command += report_gen.__insert_tab(
                                    2) + 'if global_var.' + list_ptr + '.value != 0:' + report_gen.enter
                                command += report_gen.__insert_tab(
                                    3) + 'element_pointer = global_var.' + list_ptr + deref + first + report_gen.enter
                                column_obj = ''
                                # looping through elements of the linked list.
                                command += report_gen.__insert_tab(
                                    3) + 'while element_pointer.value != 0:' + report_gen.enter
                                command += report_gen.__insert_tab(
                                    4) + 'element = apps1.fw.env.cast(element_pointer,' + '"' + element_type + '")' + report_gen.enter
                                if data_type == 'Instanced_CsrCmnListSimple_t':
                                    inst_type = table.get('lvl2_element_type', default='')
                                    inst_ptr = table.get('lvl2_element_ptr', default='')
                                    # validating level2 pointer if it is NULL then we should not go ahead with adding table row.
                                    command += report_gen.__insert_tab(
                                        4) + 'if element.' + inst_ptr + '.value != 0:' + report_gen.enter
                                    command += report_gen.__insert_tab(
                                        5) + 'inst = apps1.fw.env.cast(element.' + inst_ptr + ',' + '"' + inst_type + '")' + report_gen.enter
                                    column_obj = 'inst.'
                                    command += report_gen.__insert_tab(5) + 'table.add_row(['
                                else:
                                    command += report_gen.__insert_tab(4) + 'table.add_row(['
                                    column_obj = 'element.'

                                temp_columns = total_columns - 1

                                for column in table:
                                    column_data_type = column.get('data_type', default='')
                                    column_var = column.get('name', default='')

                                    command += report_gen.__preprocess_data(column_data_type, column_obj,
                                                                            column_var)

                                    if temp_columns > 0:
                                        temp_columns = temp_columns - 1
                                        command += ","
                                command += '])' + report_gen.enter

                                if data_type == 'Instanced_CsrCmnListSimple_t':
                                    command += report_gen.__insert_tab(
                                        4) + 'element_pointer = element.next' + report_gen.enter
                                else:
                                    command += report_gen.__insert_tab(4) + 'element_pointer = element.' + table.get(
                                        'next_ptr', default='') + report_gen.enter
                                command += report_gen.__insert_tab(3) + "print(table)" + report_gen.enter
                                command += report_gen.__insert_tab(2) + 'else:' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'print("Empty")' + report_gen.enter
                            elif data_type == 'array':
                                length = table.get('length', default='')
                                array_ptr = table.get('name', default='')
                                column_obj = 'inst.'
                                temp_columns = total_columns - 1
                                command += report_gen.__insert_tab(
                                    2) + 'for i in range(' + length + '):' + report_gen.enter
                                command += report_gen.__insert_tab(
                                    3) + 'inst = global_var.' + array_ptr + '[i]' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'table.add_row(['

                                for column in table:
                                    column_data_type = column.get('data_type', default='')
                                    column_var = column.get('name', default='')

                                    command += report_gen.__preprocess_data(column_data_type, column_obj,
                                                                            column_var)

                                    if temp_columns > 0:
                                        temp_columns = temp_columns - 1
                                        command += ","
                                command += '])' + report_gen.enter
                                command += report_gen.__insert_tab(2) + 'print(table)' + report_gen.enter
                            elif data_type == 'lea_data':
                                profile_name = table.get('profile_name', default='')

                                temp_columns = total_columns - 1
                                command += report_gen.__insert_tab(
                                    2) + 'profile_obj = __get_lea_profile_data("' + profile_name + '")' + report_gen.enter
                                command += report_gen.__insert_tab(2) + 'if profile_obj.value != 0: ' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'table.add_row(['
                                for column in table:
                                    column_data_type = column.get('data_type', default='')
                                    column_var = column.get('name', default='')

                                    command += report_gen.__preprocess_data(column_data_type, 'profile_obj.',
                                                                            column_var)
                                    if temp_columns > 0:
                                        temp_columns = temp_columns - 1
                                        command += ","
                                command += '])' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'print(table)' + report_gen.enter
                                command += report_gen.__insert_tab(2) + 'else:' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'print("NULL")' + report_gen.enter
                            command += report_gen.__insert_except_block(1)
                            command += report_gen.__insert_except_block(0)
                            #print(command)
                            exec(command)
                            command = ''
            my_file.close()

    @staticmethod
    def generate_detailed_report():
        with open(report_gen.file_name) as report_file:
            tree = ET.parse(report_file)
            report_desc = tree.getroot()
            for report in report_desc:
                if report.tag == 'REPORT':
                    report_title = report.get('title', default='')
                    global_var_name = report.get('global_name', default='')
                    global_type = report.get('global_type', default='')
                    global_file_name = report.get('global_file_name', default='')
                    print("\n##############################" + report_title + " ##############################\n")
                    for table in report:
                        add_to_detailed_report = table.get('add_to_detailed_report', default='')
                        if add_to_detailed_report == 'true':
                            data_type = table.get('data_type', default='')
                            report_type = report.get('report_type', default='')
                            command = ""
                            command += report_gen.__insert_preprocessing_code() + report_gen.enter
                            command += report_gen.__insert_try_block(0)
                            if report_type != 'lea_report':
                                command += report_gen.__insert_tab(1) + report_gen.__prepare_global_variable_statement(
                                    global_var_name, global_type,
                                    global_file_name)
                            print('\n------------- ' + table.get('title', default='') + ' -------------\n')

                            if data_type == 'regular':
                                table_object = table.get('name', default='')

                                if table_object != '':
                                    command += report_gen.__insert_tab(1) + 'print(global_var.' + table.get('name',
                                                                                                            default='') + ')' + report_gen.enter
                                command += report_gen.__insert_tab(1) + 'print()' + report_gen.enter
                                command += report_gen.__insert_tab(1) + 'print()' + report_gen.enter
                            elif data_type == 'Instanced_CsrCmnListSimple_t' or data_type == 'CsrCmnListSimple_t' or data_type == 'CsrCmnListSimple_t_ptr' or data_type == 'linked_list' or data_type == 'linked_list_ptr':
                                list_ptr = table.get('name', default='')
                                element_type = table.get('lvl1_element_type', default='')
                                deref = '' if data_type != 'CsrCmnListSimple_t_ptr' and data_type != 'linked_list_ptr' else '.deref'
                                first = '' if data_type == 'linked_list' or data_type == 'linked_list_ptr' else '.first'

                                command += report_gen.__insert_tab(
                                    1) + 'if global_var.' + list_ptr + '.value != 0:' + report_gen.enter
                                command += report_gen.__insert_tab(
                                    2) + 'element_pointer = global_var.' + list_ptr + deref + first + report_gen.enter
                                column_obj = ''
                                # looping through elements of the linked list.
                                command += report_gen.__insert_tab(
                                    2) + 'while element_pointer.value != 0:' + report_gen.enter
                                command += report_gen.__insert_tab(
                                    3) + 'element = apps1.fw.env.cast(element_pointer,' + '"' + element_type + '")' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'print(element)' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'print()' + report_gen.enter
                                command += report_gen.__insert_tab(3) + 'print()' + report_gen.enter

                                if data_type == 'Instanced_CsrCmnListSimple_t':
                                    command += report_gen.__insert_tab(
                                        3) + 'element_pointer = element.next' + report_gen.enter
                                else:
                                    command += report_gen.__insert_tab(3) + 'element_pointer = element.' + table.get(
                                        'next_ptr', default='') + report_gen.enter
                            elif data_type == 'array':
                                command += report_gen.__insert_tab(1) + 'for i in range(' + table.get('length',
                                                                                                      default='') + '):' + report_gen.enter
                                command += report_gen.__insert_tab(2) + 'print(global_var.' + table.get('name',
                                                                                                        default='') + '[i])' + report_gen.enter
                                command += report_gen.__insert_tab(2) + 'print()' + report_gen.enter
                                command += report_gen.__insert_tab(2) + 'print()' + report_gen.enter
                            elif data_type == 'lea_data':
                                profile_name = table.get('profile_name', default='')
                                command +=  report_gen.__insert_tab(1) + 'profile_obj = __get_lea_profile_data("' + profile_name + '")' + report_gen.enter
                                command +=  report_gen.__insert_tab(1) + 'print(profile_obj)'
                            command += report_gen.__insert_except_block(0)
                            # print(command)
                            exec(command)
                            command = ''
                    if global_var_name != '':
                        print('\n------------- Complete Report Variable: ' + global_var_name + ' -------------\n')
                        command += report_gen.__insert_try_block(0)
                        command += report_gen.__insert_tab(
                            1) + 'print(apps1.fw.env.globalvars[' + '"' + global_var_name + '"])'
                        command += report_gen.__insert_except_block(0)
                        exec(command)
                    command = ''
                    print('-------------------------- END OF ' + report_title + ' --------------------------\n\n')
            report_file.close()


### must end with the following line
go_interactive(locals())
