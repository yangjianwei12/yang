# Copyright (c) 2022 Qualcomm Technologies International, Ltd.

#Replacing default csr_bt_config.h file with source/csr_bt_config.h
SYNERGY_INCLUDE_PATHS:=config/inc/source $(SYNERGY_INCLUDE_PATHS)
BT_PUBLIC_HEADERS:=$(subst default/csr_bt_config.h,source/csr_bt_config.h,$(BT_PUBLIC_HEADERS))