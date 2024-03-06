############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2015 - 2017 Qualcomm Technologies International, Ltd.
#
############################################################################
from kcs import KCSFile
from kdc import KDCFile
import chip_info

# Minimum number of words that cap_download_mgr expects to
# have in processing buffer before processing. Used this
# number to append zeros at the end of file
MINIM_WORDS_IN_BUFFER = 20
INFO_FIELD_SIZE_32 = 2

class make_kcs:
    def __init__(self, chip, build_id):
        self.chip = chip
        self.build_id = build_id
        self.kdc_offset = []
        self.kdc_num_caps = []
        self.kdc_cap_id = []
        self.kcs = KCSFile(self.chip, self.build_id)

    def addKDC(self, kdcFile):
        self.kcs.addKDC(kdcFile)

    def addKCSHeader(self):
        # Can't get this information from C, preprocessor gets rid of it
        cv = self.chip.get_dsp_id()

        self.kcs.append(cv, "Chip name: " + self.chip.get_string())
        self.kcs.append(self.build_id & 0xFFFF, "Build ID: %d" % self.build_id)
        self.kcs.append((self.build_id >> 16) & 0xFFFF)
        self.kcs.append(self.kcs.getNumKDC(), "Number of KDCs: %d" % self.kcs.getNumKDC())

        # offset to 1st kdc is the 4 fields before offset plus 3 for each KDC
        offset_1st_kdc = 4 + 3 * self.kcs.getNumKDC()
        # plus 1 field for each capability inside each KDC
        for kdc in self.kcs.getKDCs():
            offset_1st_kdc = offset_1st_kdc + kdc.getNumCapabilities()
        # plus the length field
        offset_1st_kdc = offset_1st_kdc + 2
        offset = offset_1st_kdc
        length = 0
        i = 0
        for kdc in self.kcs.getKDCs():
            # KDCs must always start at 32-bit boundaries on random-access type KCS
            if self.chip.get_default_kcs_type() != KCSFile.STREAM_TYPE:
                if (((offset * 2) % 4) != 0):
                    offset = offset + 1
            self.kcs.append((offset>>16) & 0xFFFF, "|- KDC[%d] Offset: %d" % (i,offset))
            self.kcs.append(offset & 0xFFFF)
            offset = offset + len(kdc)
            length = length + len(kdc)
            self.kcs.append(kdc.getNumCapabilities(), "|- KDC[%d] Number capabilities: %d" % (i, kdc.getNumCapabilities()))
            j = 0
            for cap in kdc.getCapabilities():
                self.kcs.append(cap.getCapabilityId(), "|- - Cap[%d] ID: %d" % (j, cap.getCapabilityId()))
            i = i + 1
        self.kcs.append((length >> 16) & 0xFFFF, "Length (KCS): %d" % length)
        self.kcs.append(length & 0xFFFF)
        # Only add padding for random access KCS
        if self.chip.get_default_kcs_type() == KCSFile.RANDOM_ACCESS_TYPE:
            if (((len(self.kcs) * 2) % 4) != 0):
                self.kcs.append(0, "Zero-padding to ensure KDC word alignment")

    def addPaddingAndAlign(self, quantity):
        kdcPadding = KDCFile(dict())
        # Use always an even number of padding to ensure KCS 32-bit alignment
        length = 0
        for kdc in self.kcs.getKDCs():
            length = length + len(kdc)
        length = length + len(self.kcs)
        if length % 2 != 0:
            quantity = quantity + 1
        kdcPadding.addPadding(quantity)
        # Use a dummy KDC to add padding
        self.kcs.addKDC(kdcPadding)
        
    def generateKCS(self):
        self.addKCSHeader()
        if self.chip.supports_dm_banks_powered_off():
            if self.chip.supports_dm_as_pm():
                # Append zeros to ensure working buffer always have a minimum amount
                # The KCS files for the platforms that support DM_AS_PM and
                # the extra heap (powering off of some banks) have 3 extra
                # 32-bit fields in the info section (the size info for DM_AS_PM, 
                # DM1 EXTRA and DM2 EXTRA), therefore the minimum words in buffer
                # needs to be increased (a word in this context is 16-bit).
                self.addPaddingAndAlign(MINIM_WORDS_IN_BUFFER + 4 * INFO_FIELD_SIZE_32)
            else:
                # Append zeros to ensure working buffer always have a minimum amount
                # The KCS files for the platforms that support the extra heap 
                # (powering off of some banks) have 2 extra 32-bit fields in the 
                # info section (the size info for DM1 EXTRA and DM2 EXTRA), therefore 
                # the minimum words in buffer needs to be increased (a word in this 
                # context is 16-bit).
                self.addPaddingAndAlign(MINIM_WORDS_IN_BUFFER + 2 * INFO_FIELD_SIZE_32)
        else:
            if self.chip.supports_dm_as_pm():
                # Append zeros to ensure working buffer always have a minimum amount
                # The KCS files for the platforms that support DM_AS_PM have one extra
                # 32-bit field in the info section (the size info for DM_AS_PM), 
                # therefore the minimum words in buffer needs to be increased 
                # (a word in this context is 16-bit).
                self.addPaddingAndAlign(MINIM_WORDS_IN_BUFFER + 2 * INFO_FIELD_SIZE_32)
            else:
                self.addPaddingAndAlign(MINIM_WORDS_IN_BUFFER)
            
        return self.kcs