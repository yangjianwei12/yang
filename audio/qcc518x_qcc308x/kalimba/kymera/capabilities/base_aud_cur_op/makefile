############################################################################
# CONFIDENTIAL
# Copyright (c) 2021 Qualcomm Technologies International, Ltd.
#
############################################################################
#
# COMPONENT:    makefile
# MODULE:       base_aud_cur_op
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

C_SRC += base_aud_cur_op.c
S_SRC += base_aud_cur_op_asm.asm

# invoke the structure sharing utility
GEN_ASM_HDRS += base_aud_cur_op.h
GEN_ASM_DEFS += _AUDIO_CURATION_DEF

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = base_aud_cur_op

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf

