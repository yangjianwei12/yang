############################################################################
# Copyright (c) 2008 - 2017 Qualcomm Technologies International, Ltd.
############################################################################
#
# COMPONENT:    makefile
# MODULE:       mips_monitor
# DESCRIPTION:  Library makefile.
#
############################################################################

#########################################################################
# Define root directory (relative so we can be installed anywhere)
#########################################################################

BUILD_ROOT = ../../build
include $(BUILD_ROOT)/roots.mkf

#########################################################################
# Enter source files and directories and header directories here.
#
# Makerules will add the standard interface paths
#########################################################################

C_SRC+=mips_monitor.c
S_SRC+=mips_monitor_proc.asm

GEN_ASM_HDRS += mips_monitor_struct.h
GEN_ASM_DEFS += MIPS_MONITOR_OP_DATA

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = mips_monitor

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules_cps.mkf

include $(BUILD_ROOT)/makerules.mkf
