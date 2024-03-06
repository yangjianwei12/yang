############################################################################
# CONFIDENTIAL
# Copyright (c) 2021 Qualcomm Technologies International, Ltd.
#
############################################################################
#
# COMPONENT:    makefile
# MODULE:       support_lib/ml_engine_lib
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
H_PATH += ${KYMERA_ROOT}/capabilities/ml_engine
H_PATH += ${KYMERA_ROOT}/components/ml_modules/common
H_PATH += ${KYMERA_ROOT}/components/ml_modules/ml_engine
H_PATH += ${KYMERA_ROOT}/components/ml_modules/ml_runtime
H_PATH += ${KYMERA_ROOT}/support_lib

S_SRC += $(if $(findstring $(SQIF_CONFIG), True), ,$(if $(findstring $(KALSIM_CONFIG), True), ,$(if $(findstring $(CAPABILITY_DOWNLOAD_BUILD), true), dsl_ml_engine_lib_asm.asm, )))
C_SRC += $(if $(findstring $(INSTALL_DOWNLOAD_ML_ENGINE_LIB_SUPPORT), True), ,$(if $(findstring $(SQIF_CONFIG), True), ,$(if $(findstring $(KALSIM_CONFIG), True), ,$(if $(findstring $(CAPABILITY_DOWNLOAD_BUILD), true), dsl_ml_engine_lib.c, ))))
# Dummy File to keep compiler happy for kalsim builds
C_SRC += dsl_ml_engine_lib_dummy.c

# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = ml_engine_lib

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf
