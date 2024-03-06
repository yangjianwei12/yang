############################################################################
# Copyright (c) 2008 - 2020 Qualcomm Technologies International, Ltd.
############################################################################
#
# COMPONENT:    makefile
# MODULE:       Platform
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

C_SRC +=	pl_trace_mask.c
C_SRC +=	pl_error.c
C_SRC +=	profiler_utils.c
C_SRC +=	coredump_table.c
# Map Audio SQIF_FLASH into NVMEM Window 1 of the DM and PM
C_SRC += pl_nvmem.c

S_SRC =		reset.asm
S_SRC +=	crt0.asm
S_SRC +=	error.asm
S_SRC +=	interrupt.asm
S_SRC +=	stack.asm
S_SRC +=	pl_intrinsics_asm.asm
S_SRC +=	profiler.asm

# Files for secondary core build
S_SRC += $(if $(SUPPORTS_MULTI_CORE), crt_p1.asm)

# If not building for target include a small set of C functions for compatibility.
# asm files above are ignored in GCC builds
C_SRC += $(if $(findstring $(TARGET_COMPILER), gcc), pl_intrinsics.c)
C_SRC += $(if $(findstring $(TARGET_COMPILER), gcc), pl_interrupt.c)
C_SRC += $(if $(findstring $(TARGET_COMPILER), gcc), pl_profiler.c)

# Lightweight HW lock API
C_SRC += $(if $(BUILD_HW_LOCK), pl_hwlock.c)

PATCH_SRC += profiler.asm
GEN_PIDS = $(PATCH_DIR)/platform_patch_ids.txt

GEN_ASM_HDRS += pl_trace_masks.h
GEN_ASM_HDRS += profiler_c.h

#########################################################################
# Enter final target file here (only 1 target should be specified)
#########################################################################

TARGET_EXE_ROOT =
TARGET_LIB_ROOT = platform

#########################################################################
# Build tools extra switches
#########################################################################

#########################################################################
# Include the standard definitions and rules
#########################################################################

include $(BUILD_ROOT)/makerules.mkf
