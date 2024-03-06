############################################################################
# Copyright (c) 2008 - 2018 Qualcomm Technologies International, Ltd.
############################################################################
#
# COMPONENT:    makefile
# MODULE:       sco_fw
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

C_SRC += sco_common_funcs.c
C_SRC += sco_fw_output.c
C_SRC += sco_plc100.c

GEN_ASM_HDRS += sco_common_struct.h

GEN_ASM_DEFS += SCO_TERMINAL_BUFFERS
GEN_ASM_DEFS += SCO_COMMON_SEND_OP_DATA

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = sco_fw

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf

