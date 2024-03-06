############################################################################
# CONFIDENTIAL
# Copyright (c) 2021 Qualcomm Technologies International, Ltd.
#
############################################################################
#
# COMPONENT:    makefile
# MODULE:       support_lib/vagm_clk_lib
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
H_PATH += ${KYMERA_ROOT}/support_lib

S_SRC += $(if $(findstring $(SQIF_CONFIG), True), ,$(if $(findstring $(KALSIM_CONFIG), True), ,$(if $(findstring $(CAPABILITY_DOWNLOAD_BUILD), true), dls_vagm_clk_lib_asm.asm, )))

# for some config above sources could be skipped
C_SRC += dls_vagm_clk_lib_dummy.c

# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = vagm_clk_lib

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf
