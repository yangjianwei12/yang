############################################################################
# Copyright (c) 2021 Qualcomm Technologies, Inc. and/or its subsidiaries
############################################################################
#
# COMPONENT:    makefile
# MODULE:       Kalimba ML algorithm library
# DESCRIPTION:  Library makefile.
#
############################################################################


#########################################################################
# Define root directory (relative so we can be installed anywhere)
#########################################################################

BUILD_ROOT = ../../kymera/build
include $(BUILD_ROOT)/roots.mkf

#########################################################################
# Enter source files and directories and header directories here.
#
# Makerules will add the standard interface paths
#########################################################################
C_PATH += ./preproc
C_PATH += ./postproc
C_PATH += ./math
C_PATH += ../ml/math

S_PATH += ./math

H_PATH += ./math
H_PATH += ./preproc
H_PATH += ./postproc

LIB_ROOT = ../../lib
H_PATH += $(LIB_ROOT)/ml/math
H_PATH += $(LIB_ROOT)/ml/utils
H_PATH += $(LIB_ROOT)/ml/common

# All assembly source
S_SRC  = math_lib.asm
C_SRC += vector_ops_fixed_kalimba.c
C_SRC +=  math_lib_c.c
C_SRC +=  mel.c
C_SRC +=  normalization.c
C_SRC +=  spectral.c
C_SRC +=  taper.c
C_SRC +=  pre_passthrough.c
C_SRC +=  post_passthrough.c
C_SRC +=  svad_post_processing.c
# PCEN disabled by default. To enable, uncomment the source file below
# C_SRC +=  pcen.c
# MFCC disabled by default. To enable, uncomment the source file below
# C_SRC +=  mfcc.c

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = ml_algos

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf
