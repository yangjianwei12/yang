############################################################################
# Copyright (c) 2008 - 2021 Qualcomm Technologies International, Ltd.
############################################################################
#
# COMPONENT:    makefile
# PROJECT:      silence_detect
# DESCRIPTION:  Capability makefile.
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

C_SRC+=silence_detect.c

GEN_ASM_HDRS += silence_detect_struct.h
GEN_ASM_DEFS += silence_detect_algo_object

CPS_XML = silence_detect.xml

#########################################################################
# Enter final target file here (only 1 target should be specified)
########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = silence_detect

#########################################################################
# Include the standard definitions and rules
#########################################################################
include $(BUILD_ROOT)/makerules_cps.mkf
include $(BUILD_ROOT)/makerules.mkf
