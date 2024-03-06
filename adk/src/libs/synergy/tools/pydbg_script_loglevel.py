#--------------------------------------------------------------------------------------------------
# Copyright (c) 2022 Qualcomm Technologies International, Ltd
#--------------------------------------------------------------------------------------------------
#
# Utilities to get and set the log levels for all the Synergy tasks.
#
# Usage: 
# 1) Run pydbg with log level script. 
#    After that you can call the functions defined in this script from pydbg shell
# --------------------------------------------------------------------------------
#    python pydbg.py -f apps1:"<path to earbud.elf>" <path to pydbg_script_loglevel.py>
# e.g.
#    python pydbg.py -f apps1:"C:\test\earbud.elf>" C:\test\Synergy\tools\pydbg_script_loglevel.py"
# 
# 2) Get Log Level for all modules
# ---------------------------------------------
# >>>SynergyGetLogLevels()
#
#    01: BT_CM           SYNERGY_DEBUG_LOG_LEVEL_INFO (0x03)
#    02: BT_HF           SYNERGY_DEBUG_LOG_LEVEL_INFO (0x03)
#    03: BT_AV           SYNERGY_DEBUG_LOG_LEVEL_INFO (0x03)
#    04: BT_AVRCP        SYNERGY_DEBUG_LOG_LEVEL_INFO (0x03)
#    ...
#    ...
#
# 3) Setting log level for HF to Verbose
# ---------------------------------------------
# >>>SynergySetLogLevels(2,4)
#
#    02: BT_HF           SYNERGY_DEBUG_LOG_LEVEL_VERBOSE (0x04)
#
# 4) Setting log level for all modules to Verbose
# ---------------------------------------------
# >>>SynergySetLogLevels(0,4)
#
#    01: BT_CM           SYNERGY_DEBUG_LOG_LEVEL_VERBOSE (0x04)
#    02: BT_HF           SYNERGY_DEBUG_LOG_LEVEL_VERBOSE (0x04)
#    03: BT_AV           SYNERGY_DEBUG_LOG_LEVEL_VERBOSE (0x04)
#    04: BT_AVRCP        SYNERGY_DEBUG_LOG_LEVEL_VERBOSE (0x04)
#    ...
#    ...
#--------------------------------------------------------------------------------------------------

from csr.wheels.bitsandbobs import CLang

###########################################################################
# Print the current log levels for each synergy task. 
###########################################################################
def SynergyGetLogLevels():
    mydatasize = apps1.fw.env.globalvars['g_log_registrations_size']
    max_len = 0
    names = []
    levels = []
    for x in range(0,mydatasize.value):
        str_address = apps1.fw.env.globalvars['g_log_registrations'][x].originName.address
        str_val = CLang.get_string(apps1.dm[str_address:str_address+0x10])
        str_level = apps1.fw.env.globalvars['g_log_registrations'][x].logLevel
        names.append(str_val)
        levels.append(str_level)
        if len(str_val) > max_len: max_len = len(str_val)
    
    for x in range(0,mydatasize.value):
        index = x + 1
        print(f'{"%02d"%index}: {names[x]}{(max_len - len(names[x]))* " "} {levels[x]}')

###########################################################################
# Set the log level for a synergy task.
# Parameters:
#     index: index of the task as printed by SynergyGetLogLevels call. 
#            If we provide '0' as the index, level is updated for all the 
#            tasks. 
#     level: Levels (0-5)
#            0    SYNERGY_DEBUG_LOG_LEVEL_CRITICAL,
#            1    SYNERGY_DEBUG_LOG_LEVEL_ERROR,
#            2    SYNERGY_DEBUG_LOG_LEVEL_WARN,
#            3    SYNERGY_DEBUG_LOG_LEVEL_INFO,
#            4    SYNERGY_DEBUG_LOG_LEVEL_VERBOSE,
#            5    SYNERGY_DEBUG_LOG_LEVEL_TASK
###########################################################################
def SynergySetLogLevels(index, level):
    if index > 0 :
        x = index-1
        apps1.fw.env.globalvars['g_log_registrations'][x].logLevel.value = level
        str_address = apps1.fw.env.globalvars['g_log_registrations'][x].originName.address
        str_level = apps1.fw.env.globalvars['g_log_registrations'][x].logLevel
        str_val = CLang.get_string(apps1.dm[str_address:str_address+0x10])
        print(f'{index}: {str_val}\t :{str_level}')
    else:
        mydatasize = apps1.fw.env.globalvars['g_log_registrations_size']
        for i in range(0,mydatasize.value):
            apps1.fw.env.globalvars['g_log_registrations'][i].logLevel.value = level
        SynergyGetLogLevels()

### must end with the following line
go_interactive(locals())