############################################################################
# CONFIDENTIAL
# Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
# Unit 400, Cambridge Science Park,
# Milton Road, Cambridge, CB4 0WH.  UK
# Tel: +44 1223 692000  Fax: +44 1223 692001
#
############################################################################
#
# COMPONENT:    makefile
# MODULE:       svad
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

C_SRC +=         svad.c

S_SRC +=          svad_helper_functions.asm

# invoke the structure sharing utility
GEN_ASM_HDRS += svad_capability.h
GEN_ASM_DEFS += svad_extra_op

# RW: need to remove this.  it's temporary because the vad private libray needs it.
H_PATH += $(AUTOGEN_INC_ROOT)/vad

PATCH_SRC = svad_helper_functions.asm
GEN_PIDS = $(PATCH_DIR)/capability_svad_patch_ids.txt

CPS_XML = svad.xml

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = svad

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules_cps.mkf
include $(BUILD_ROOT)/makerules.mkf

