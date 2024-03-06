# Copyright (c) 2018-2023 Qualcomm Technologies International, Ltd.
#   %%version

ifdef MAKEFILE_RULES_DIR
    BLUELAB = $(MAKEFILE_RULES_DIR)
endif
include $(BLUELAB)/Makefile.rules

FIXPATH=$(subst /,\,$1)
ifeq ($(OS),Windows_NT)
DEL=del /S /Q
else
DEL=rm -rf
endif

BUILDOUTPUT_PATH?=../..
INSTALL_DIR=$(BUILDOUTPUT_PATH)/installed_libs
LIB_SRC_DIR=../
LIB_DIR=$(INSTALL_DIR)/lib/default_$(CHIP_TYPE)/native
SYNERGY_INTERFACE_DIR=$(INSTALL_DIR)/include/profiles/default_$(CHIP_TYPE)
SYNERGY_PLATFORM_INTERFACE_DIR=$(SYNERGY_INTERFACE_DIR)/platform
SYNERGY_LEA_SERVICE_DIR=./bt/profile_managers/le_audio/services/
SYNERGY_LE_SERVICE_DIR=./bt/profile_managers/gatt_services/
OUTPUT_DIR=output_default

#generate source_list.py
define gen_source_list
$(shell $(python) ./tools/gen_build_files.py)
endef

#generate enum header files for synergy prims
define gen_enum_headers
enums := $(shell $(python) ./tools/enum_generator.py)
endef

#get source file lists from source_list.py
define read_parameter
$(1):=$(shell $(python) -c "from source_list import $(1); print(' '.join($(1)))")
endef

$(eval $(call gen_enum_headers))
$(eval $(call gen_source_list))
$(eval $(call read_parameter,SYNERGY_INCLUDE_PATHS))
$(eval $(call read_parameter,OS_INCLUDE_PATHS))
$(eval $(call read_parameter,PLATFORM_PUBLIC_HEADERS))
$(eval $(call read_parameter,BT_PUBLIC_HEADERS))
$(eval $(call read_parameter,BT_LEA_PUBLIC_HEADERS))
$(eval $(call read_parameter,BT_PRIVATE_HEADERS))
$(eval $(call read_parameter,LEA_LIB_HEADERS))
$(eval $(call read_parameter,FRAMEWORK_LIB_SOURCES))
$(eval $(call read_parameter,PROFILES_LIB_SOURCES))
$(eval $(call read_parameter,CORESTACK_LIB_SOURCES))
$(eval $(call read_parameter,LEA_LIB_SOURCES))
$(eval $(call read_parameter,SERVICE_LIB_HEADERS))
$(eval $(call read_parameter,SERVICE_LIB_SOURCES))
$(eval $(call read_parameter,LEA_DBI_FILES))
$(eval $(call read_parameter,LE_DBI_FILES))
LEA_DBI_FILES_DIR = $(foreach inc,$(LEA_DBI_FILES),$(inc))
LE_DBI_FILES_DIR = $(foreach inc,$(LE_DBI_FILES),$(inc))
SYNERGY_INCLUDE_PATHS := $(subst QBL_LIB_PATH,../../../../os/$(CHIP_TYPE)/$(OS_VARIANT)/src/fw/src/bt/utilities,$(SYNERGY_INCLUDE_PATHS))
CORESTACK_LIB_SOURCES := $(subst QBL_LIB_PATH,../../../../os/$(CHIP_TYPE)/$(OS_VARIANT)/src/fw/src/bt/utilities,$(CORESTACK_LIB_SOURCES))


t := $(foreach f,$(BT_LEA_PUBLIC_HEADERS),$(wildcard $f))
BT_LEA_PUBLIC_HEADERS := $t
t := $(foreach f,$(LEA_LIB_HEADERS),$(wildcard $f))
LEA_LIB_HEADERS := $t
t := $(foreach f,$(LEA_LIB_SOURCES),$(wildcard $f))
LEA_LIB_SOURCES := $t
t := $(foreach f,$(LEA_DBI_FILES),$(wildcard $f))
LEA_DBI_FILES := $t
t := $(foreach f,$(BT_PUBLIC_HEADERS),$(wildcard $f))
BT_PUBLIC_HEADERS := $t $(enums)
t := $(foreach f,$(BT_PRIVATE_HEADERS),$(wildcard $f))
BT_PRIVATE_HEADERS := $t
t := $(foreach f,$(PROFILES_LIB_SOURCES),$(wildcard $f))
PROFILES_LIB_SOURCES := $t
t := $(foreach f,$(LE_DBI_FILES),$(wildcard $f))
LE_DBI_FILES := $t

# VM_APP_TYPE could be set in BUILD_ARGUMENTS to configure synergy library for different applications.
# If VM_APP_TYPE is set to "xyz", Makefile expects following file to be present in Synergy folder - './config/xyz.mak'. 
# Typically this mak file is used to change the 'csr_bt_config.h' used in the build, allowing us to select 
# features as per the application. If VM_APP_TYPE is not set './config/inc/default/csr_bt_config.h' is used.
ifneq (,$(VM_APP_TYPE))
include ./config/$(VM_APP_TYPE).mak
OUTPUT_DIR=output_$(VM_APP_TYPE)
$(info Using $(VM_APP_TYPE) Configuration)
else
$(info Using Default Configuration)
VM_APP_TYPE=default
endif

config_changed := $(shell $(python) ./tools/has_config_changed.py $(VM_APP_TYPE))
ifeq (True,$(config_changed))
$(info $(shell $(DEL) "$(subst /,\,$(SYNERGY_INTERFACE_DIR)/csr_bt_config.h)"))
$(info $(shell $(DEL) "$(subst /,\,$(LIB_DIR)/libcsr_frw.a)"))
$(info $(shell $(DEL) "$(subst /,\,$(LIB_DIR)/libcsr_bt_profiles.a)"))
$(info $(shell $(DEL) "$(subst /,\,$(LIB_DIR)/libcsr_bt_le_audio.a)"))
$(info $(shell $(DEL) "$(subst /,\,$(LIB_DIR)/libcsr_bt_corestack_libs.a)"))
$(info $(shell $(DEL) "$(subst /,\,$(LIB_DIR)/libsynergy_service.a)"))
endif

MAKE_TOOLS_DIR=../tools/make
#BUILD_DIR=./build
BUILD_DIR=.
INCLUDE_LIB_PATHS=-I$(LIB_SRC_DIR)/handover_if -I$(LIB_SRC_DIR)/vmtypes
INCLUDE_PATHS=$(foreach inc,$(SYNERGY_INCLUDE_PATHS),-I$(inc)) $(INCLUDE_LIB_PATHS)
INCLUDE_OS_PATHS=$(foreach inc,$(OS_INCLUDE_PATHS),-I$(inc))

WARNING_OPTS = -Wall -WAccuracyLoss -WnoConstantTest -WCharPointerUnaligned -WnoAccuracyLoss -WnoArgSizeLarger -WnoPointerUnaligned -WnoExplicitQualifier -WnoCharPointerUnaligned -WnoUnsignedCompare -Werror
CFLAGS_COMMON = -g -minim -Xassembler -apply-b179745-workaround $(WARNING_OPTS)
CFLAGS_COMMON += -k$(CHIP_NAME) -DHYDRA -DTRAPSET_BLUESTACK=1 -DTRAPSET_RFCOMM=1 -DTRAPSET_STREAM=1 -DTRAPSET_AUDIO=1 -DTRAPSET_CORE=1 -DEXCLUDE_VSC

# Define HCI flag required by CM but not available with Hydra OS hci.h
CFLAGS_COMMON += -DHCI_CIS_MAX=((uint8_t)0x08) -DHCI_ULP_BROADCAST_CODE_SIZE=16 -DBLUESTACK_LIB

# Flags for building synergy
CFLAGS = $(CFLAGS_COMMON) 
CFLAGS += $(INCLUDE_PATHS)
CFLAGS += $(INCLUDE_OS_PATHS)

# Flags for building synergy service
CFLAGS_SERVICE = $(CFLAGS_COMMON)
CFLAGS_SERVICE += $(INCLUDE_OS_PATHS)
CFLAGS_SERVICE += $(INCLUDE_PATHS)

# User can set "BUILD_LEA=FALSE" in BUILD_ARGUMENTS in the x2p file to disable the LEA compilation
# By default it is set as "BUILD_LEA=TRUE".
ifneq (,$(wildcard ./bt/profile_managers/le_audio))
ifeq (TRUE,$(BUILD_LEA))
CFLAGS += -DCSR_ENABLE_LEA
CFLAGS_SERVICE += -DCSR_ENABLE_LEA
LEA_ENABLED := 1
endif
else
# Release package does not contain LEA code.
LEA_ENABLED :=
endif

# Enable Extended TD DB support for 12 devices
ifneq (qcc517x_qcc307x,$(CHIP_TYPE))
CFLAGS += -DINSTALL_EXTENDED_TD_DB
endif

# To toggle debug logging, user should set LOGGING_ENABLED as TRUE or FALSE in BUILD_ARGUMENTS in the x2p file
ifeq (TRUE,$(LOGGING_ENABLED))
CFLAGS += -DCSR_LOG_ENABLE -DBUILD_LOG_LEVEL=$(LOG_LEVEL)
CFLAGS_SERVICE += -DCSR_LOG_ENABLE -DBUILD_LOG_LEVEL=$(LOG_LEVEL)
else
endif

define delete_file
ifneq (,$(wildcard $(1)))
  tmp:= $$(shell $(DEL) $(1))
endif
endef

define remove_output_hdrs
  file:=$(subst /,\,$(SYNERGY_INTERFACE_DIR)/$(notdir $(1)))
  $(call delete_file,$(file))
endef

# $(1): Public header files to be copied
# $(2): Destination folder
define copy_header_rules

copy_header :: $(2)/$(notdir $(1))

$(2)/$(notdir $(1)): $(1)
	-$(mkdir) $${@D}
	$(copyfile) $$^ $${@}

endef # copy_header_rules

DBI_LE_H_FILES=$(subst .dbi,.h,$(LE_DBI_FILES))
define le_gattdb_rules
# $(1) = directory containing dbi files
# $(2) = dbi filename 
dbh_$(1) := $$(patsubst $(1)%.dbi,$(1)/$(subst .dbi,.h,$(2)),$$(wildcard $(1)*.dbi))
# Rule for pre-processing of the .dbi files to generate .dbi_ files
ifneq (,$$(dbh_$(1)))
$(1)%.h : $(1)%.dbi_
	$$(info dogattdbi $(1), $(2))
	$$(call dogattdbi,$(1))
$(1)%.dbi_ $(1)depend/%db.d : $(1)%.dbx
	$$(info dogattdbi_pre $(1), $(2))
	$$(call dogattdbi_pre,$(1))
$(1)%.dbx : $(1)%.dbi
	$$(info preprocessing $(1), $(2))
	$$(cpre) -DGATT_DBI_LIB $(INCLUDE_PATHS) -MD -MT $$@ -MF $(1)/$(subst .dbi,.d,$(2)) $$< -o $(1)/$(subst .dbi,.dbx,$(2))
	$$(sed) -r -i -e '/^\./d' $(1)/$(subst .dbi,.dbx,$(2))
endif
endef # le_gattdb_rules

DBI_LEA_H_FILES=$(subst .dbi,.h,$(LEA_DBI_FILES))
define lea_gattdb_rules
# $(1) = directory containing dbi files
# $(2) = dbi filename
dbh_$(1) := $$(patsubst $(1)%.dbi,$(1)/$(subst .dbi,.h,$(2)),$$(wildcard $(1)*.dbi))
# Rule for pre-processing of the .dbi files to generate .dbi_ files
ifneq (,$$(dbh_$(1)))
$(1)%.h : $(1)%.dbi_
	$$(info dogattdbi $(1), $(2))
	$$(call dogattdbi,$(1))
$(1)%.dbi_ $(1)depend/%db.d : $(1)%.dbx
	$$(info dogattdbi_pre $(1), $(2))
	$$(call dogattdbi_pre,$(1))
$(1)%.dbx : $(1)%.dbi
	$$(info preprocessing $(1), $(2))
	$$(cpre) -DGATT_DBI_LIB $(INCLUDE_PATHS) -MD -MT $$@ -MF $(1)/$(subst .dbi,.d,$(2)) $$< -o $(1)/$(subst .dbi,.dbx,$(2))
	$$(sed) -r -i -e '/^\./d' $(1)/$(subst .dbi,.dbx,$(2))
endif
endef # lea_gattdb_rules

$(foreach hdr, $(BT_PUBLIC_HEADERS), $(eval $(call copy_header_rules,$(hdr),$(SYNERGY_INTERFACE_DIR))))
$(foreach hdr, $(PLATFORM_PUBLIC_HEADERS), $(eval $(call copy_header_rules,$(hdr),$(SYNERGY_PLATFORM_INTERFACE_DIR))))
$(foreach dbi, $(LE_DBI_FILES), $(eval $(call copy_header_rules,$(dbi),$(SYNERGY_INTERFACE_DIR))))
$(foreach dbi_file, $(LE_DBI_FILES_DIR), $(eval $(call le_gattdb_rules,$(dir $(dbi_file)),$(notdir $(dbi_file)),$(SYNERGY_LE_SERVICE_DIR))))
$(foreach hdr, $(BT_LEA_PUBLIC_HEADERS), $(eval $(call copy_header_rules,$(hdr),$(SYNERGY_INTERFACE_DIR))))
$(foreach dbi, $(LEA_DBI_FILES), $(eval $(call copy_header_rules,$(dbi),$(SYNERGY_INTERFACE_DIR))))
ifneq (,$(LEA_ENABLED))
$(foreach dbi_file, $(LEA_DBI_FILES_DIR), $(eval $(call lea_gattdb_rules,$(dir $(dbi_file)),$(notdir $(dbi_file)),$(SYNERGY_SERVICE_DIR))))
endif

# $(1): Source file
# $(2): Object file
# $(3): CFlags
# $(4): Dependencies
define build_c_rules
$(OUTPUT_DIR)/$(dir $1)/$(notdir $1).d:
$(2): $(1) $(4) $(OUTPUT_DIR)/$(dir $1)/$(notdir $1).d
	$$(info Compiling $(1))
	-$(mkdir) $(dir $(2))
	$(CC) $(3) -o $(2) -c $(1) -MD -MF$(OUTPUT_DIR)/$(dir $1)/$(notdir $1).d
-include $(OUTPUT_DIR)/$(dir $1)/$(notdir $1).d
endef # build_c_rules

define build_c_rules_corestack
$(OUTPUT_DIR)/corestack/$(notdir $1).d:
$(2): $(1) $(4) $(OUTPUT_DIR)/corestack/$(notdir $1).d
	$$(info Compiling $(1) $(2))
	-$(mkdir) $(dir $(2))
	$(CC) $(3) -o $(2) -c $(1) -MD -MF$(OUTPUT_DIR)/corestack/$(notdir $1).d
-include $(OUTPUT_DIR)/corestack/$(notdir $1).d
endef # build_c_rules_corestack

$(foreach src, $(FRAMEWORK_LIB_SOURCES), \
    $(eval obj := $(patsubst %.c,$(BUILD_DIR)/$(OUTPUT_DIR)/%.o,$(src))) \
    $(eval ALL_OBJS_FRW += $(obj)) \
    $(eval $(call build_c_rules,$(src),$(obj),$(CFLAGS_SERVICE))))

$(foreach src, $(PROFILES_LIB_SOURCES), \
    $(eval obj := $(patsubst %.c,$(BUILD_DIR)/$(OUTPUT_DIR)/%.o,$(src))) \
    $(eval ALL_OBJS_PROFILES += $(obj)) \
    $(eval $(call build_c_rules,$(src),$(obj),$(CFLAGS), $(DBI_LE_H_FILES))))

$(foreach src, $(CORESTACK_LIB_SOURCES), \
    $(eval obj := $(patsubst %.c,$(BUILD_DIR)/$(OUTPUT_DIR)/corestack/%.o,$(notdir $(src)))) \
    $(eval ALL_OBJS_CORESTACK_LIBS += $(obj)) \
    $(eval $(call build_c_rules_corestack,$(src),$(obj),$(CFLAGS))))

$(foreach src, $(LEA_LIB_SOURCES), \
    $(eval obj := $(patsubst %.c,$(BUILD_DIR)/$(OUTPUT_DIR)/%.o,$(src))) \
    $(eval ALL_OBJS_LE_AUDIO += $(obj)) \
    $(eval $(call build_c_rules,$(src),$(obj),$(CFLAGS), $(DBI_LEA_H_FILES))))

$(foreach src, $(SERVICE_LIB_SOURCES), \
    $(eval obj := $(patsubst %.c,$(BUILD_DIR)/$(OUTPUT_DIR)/%.o,$(src))) \
    $(eval ALL_OBJS_SERVICE += $(obj)) \
    $(eval $(call build_c_rules,$(src),$(obj),$(CFLAGS_SERVICE))))

install :: copy_header synergy_libs

# $1: Target directory
# $2: Archive name
# $3: List of files to add to the archive 
define create_archive
    $(eval temp_filelist=$(basename $(notdir $2))_obj_files)
    -$(mkdir) $1
    $(file > $(temp_filelist),$3)
    $(AR) ru $2 @$(temp_filelist)
    -$(DEL) $(temp_filelist)
endef

$(LIB_DIR)/libcsr_frw.a :: $(ALL_OBJS_FRW)
	$(call create_archive,${@D},$@,$^)

$(LIB_DIR)/libcsr_bt_profiles.a :: $(ALL_OBJS_PROFILES)
	$(call create_archive,${@D},$@,$^)

$(LIB_DIR)/libcsr_bt_corestack_libs.a :: $(ALL_OBJS_CORESTACK_LIBS)
	$(call create_archive,${@D},$@,$^)

$(LIB_DIR)/libcsr_bt_le_audio.a : $(ALL_OBJS_LE_AUDIO)
	$(call create_archive,${@D},$@,$^)

ifneq (,$(LEA_ENABLED))
LE_LIB_TARGET := $(LIB_DIR)/libcsr_bt_le_audio.a
else
LE_LIB_TARGET :=
endif

$(LIB_DIR)/libsynergy_service.a :: $(ALL_OBJS_SERVICE)
	$(call create_archive,${@D},$@,$^)

.PHONY: synergy_libs
synergy_libs :: $(LIB_DIR)/libcsr_frw.a $(LIB_DIR)/libcsr_bt_profiles.a $(LIB_DIR)/libcsr_bt_corestack_libs.a $(LE_LIB_TARGET) $(LIB_DIR)/libsynergy_service.a

.PHONY: clean
formatted_hdr_list:=$(subst /,\,$(strip $(enums)))
clean:
	$(foreach hdr, $(formatted_hdr_list), $(eval $(call delete_file,$(hdr))))
ifneq (,$(wildcard $(CURDIR)/$(OUTPUT_DIR)))
	rmdir /q /s "$(CURDIR)\$(OUTPUT_DIR)"
endif
ifneq (,$(wildcard $(SYNERGY_INTERFACE_DIR)))
	$(foreach hdr, $(BT_PUBLIC_HEADERS),$(eval $(call remove_output_hdrs,$(hdr))))
	$(foreach hdr, $(PLATFORM_PUBLIC_HEADERS), $(eval $(call remove_output_hdrs,$(hdr))))
	$(foreach hdr, $(BT_LEA_PUBLIC_HEADERS),$(eval $(call remove_output_hdrs,$(hdr))))
	$(foreach dbi, $(LE_DBI_FILES), $(eval $(call remove_output_hdrs,$(dbi))))
	$(foreach dbi, $(LEA_DBI_FILES),$(eval $(call remove_output_hdrs,$(dbi))))
endif

