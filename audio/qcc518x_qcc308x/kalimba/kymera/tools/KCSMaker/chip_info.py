############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2015 - 2021 Qualcomm Technologies International, Ltd.
#
############################################################################
from kcs import KCSFile

heap_names_enum_name = "heap_names"
dm_as_pm_heap_name = "HEAP_NVRAM"
extra_heap_name = "HEAP_EXTRA"
invalid_heap_name = "HEAP_INVALID"
    
class chip_info(object):
    """
    A class representing a chip.
    This class encapsulates all information relevant to KCSMaker
    """
    CRESCENDO = "crescendo"
    STRE      = "stre"
    STREPLUS  = "streplus"
    MAOR      = "maor"
    MAORGEN1  = "maorgen1"
    MAOR2     = "maor2"
    
    # List of currently supported chips by KCSMaker
    _supported_chips_list     = [CRESCENDO, STRE, STREPLUS, MAOR, MAORGEN1, MAOR2]
    _pm_octet_addressing_list = [CRESCENDO, STRE, STREPLUS, MAOR, MAORGEN1, MAOR2]
    _dm_octet_addressing_list = [CRESCENDO, STRE, STREPLUS, MAOR, MAORGEN1, MAOR2]
    _kal_arch4_list           = [CRESCENDO, STRE, STREPLUS, MAOR, MAORGEN1, MAOR2]
    _minim_list               = [CRESCENDO, STRE, STREPLUS, MAOR, MAORGEN1, MAOR2]

    _kld_list                 = [CRESCENDO, STRE, STREPLUS, MAOR, MAORGEN1, MAOR2]
    _mark_code_section_type   = [CRESCENDO, STRE, STREPLUS, MAOR, MAORGEN1, MAOR2]
    
    # Get this from http://wiki/ChipVersionIDs
    _dsp_id_dict = {CRESCENDO: 0x46, STRE: 0x49, STREPLUS: 0x4B, MAOR: 0x50, MAORGEN1: 0x50, MAOR2: 0x50}

    _default_kcs_type = {CRESCENDO: KCSFile.STREAM_TYPE,
                         STRE: KCSFile.STREAM_TYPE,
                         STREPLUS: KCSFile.STREAM_TYPE,
                         MAOR: KCSFile.STREAM_TYPE,
                         MAORGEN1: KCSFile.STREAM_TYPE,
                         MAOR2: KCSFile.STREAM_TYPE}

    # Conversion table for CRT libraries
    _crt_lib_name = {CRESCENDO : "csra68100_audio", STRE : "QCC512x_audio", STREPLUS : "QCC514x_audio", MAOR : "QCC516x_audio", MAORGEN1 : "QCC516x_audio", MAOR2 : "QCC517x_audio"}

    def _set_dm_as_pm_support(self, elf_name):
        # Set support for DM_AS_PM
        # This is True for builds that have CHIP_HAS_NVRAM_ACCESS_TO_DM defined
        self._supports_dm_as_pm = False
        
        try:
            import kalelfreader_lib_wrappers
        except ImportError:
            from kal_python_tools import kalelfreader_lib_wrappers
        
        ker = kalelfreader_lib_wrappers.Ker()
        ker.open_file(elf_name)
        enums = ker.get_enums()
        
        if heap_names_enum_name in enums:
            heap_names_enum = enums[heap_names_enum_name]
            try:
                dm_as_pm_heap = heap_names_enum[dm_as_pm_heap_name]
                invalid_heap = heap_names_enum[invalid_heap_name]
                
                # This check is done just in case the enum value will be
                # defined for platforms that do not support the feature,
                # but the code would be simplified if it's defined.
                if dm_as_pm_heap < invalid_heap:
                    self._supports_dm_as_pm = True
            except KeyError:
                # HEAP_NVRAM is not a member of the heap_names enum
                # This means that the feature is not available
                pass
            
        ker.close_file()
        
    def _set_dm_banks_can_be_powered_off_support(self, elf_name):
        # Set support for powering off DM banks
        # This is True for builds that have DM_BANKS_CAN_BE_POWERED_OFF defined
        self._supports_dm_banks_powered_off = False
        
        try:
            import kalelfreader_lib_wrappers
        except ImportError:
            from kal_python_tools import kalelfreader_lib_wrappers
        
        ker = kalelfreader_lib_wrappers.Ker()
        ker.open_file(elf_name)
        enums = ker.get_enums()
        
        if heap_names_enum_name in enums:
            heap_names_enum = enums[heap_names_enum_name]
            try:
                extra_heap = heap_names_enum[extra_heap_name]
                invalid_heap = heap_names_enum[invalid_heap_name]
                
                # This check is done just in case the enum value will be
                # defined for platforms that do not support the feature,
                # but the code would be simplified if it's defined.
                if extra_heap < invalid_heap:
                    self._supports_dm_banks_powered_off = True
            except KeyError:
                # HEAP_EXTRA is not a member of the heap_names enum
                # This means that the feature is not available
                pass
            
        ker.close_file()
            
    def __init__(self, firmware_build_config, elf_name):
        chip_name = firmware_build_config.split('_')[0]
        if chip_name.lower() == chip_info.CRESCENDO:
            self.chip_name = chip_info.CRESCENDO
        elif chip_name.lower() == chip_info.STRE:
            self.chip_name = chip_info.STRE
        elif chip_name.lower() == chip_info.STREPLUS:
            self.chip_name = chip_info.STREPLUS
        elif chip_name.lower() == chip_info.MAOR:
            if "v10" in firmware_build_config:
                self.chip_name = chip_info.MAOR
            else:
                self.chip_name = chip_info.MAOR2
        elif chip_name.lower() == chip_info.MAORGEN1:
            self.chip_name = chip_info.MAORGEN1
        else:
            raise Exception("Unsupported chip type: " + chip_name)
        
        self._set_dm_as_pm_support(elf_name)
        self._set_dm_banks_can_be_powered_off_support(elf_name)

    def marks_code_section_type(self):
        if self.chip_name in chip_info._mark_code_section_type:
            return True
        else:
            return False

    def supports_minim(self):
        if self.chip_name in chip_info._minim_list:
            return True
        else:
            return False

    def get_crt_lib_name(self):
        return chip_info._crt_lib_name[self.chip_name]

    def get_dsp_id(self):
        return chip_info._dsp_id_dict[self.chip_name]

    def get_string(self):
        return self.chip_name

    def get_linkscript_template_name(self, dm_as_pm):
        if self.supports_dm_as_pm() and dm_as_pm == True:
            return "linkscript_" + self.chip_name + "_all_dm_as_pm"
        return "linkscript_" + self.chip_name

    def get_KAL_ARCH(self):
        if self.chip_name in chip_info._kal_arch4_list:
            return "KAL_ARCH4"
        else:
            raise Exception("Unsupported chip type")

    def is_supported(self):
        if self.chip_name in chip_info._supported_chips_list:
            return True
        else:
            return False

    def supports_dm_as_pm(self):
        return self._supports_dm_as_pm
    
    def supports_dm_banks_powered_off(self):
        return self._supports_dm_banks_powered_off
        
    def does_pm_octet_addressing(self):
        if self.chip_name in chip_info._pm_octet_addressing_list:
            return True
        else:
            return False

    def does_dm_octet_addressing(self):
        if self.chip_name in chip_info._dm_octet_addressing_list:
            return True
        else:
            return False

    def uses_kld(self):
        if self.chip_name in chip_info._kld_list:
            return True
        else:
            return False

    def get_default_kcs_type(self):
        return chip_info._default_kcs_type[self.chip_name]
