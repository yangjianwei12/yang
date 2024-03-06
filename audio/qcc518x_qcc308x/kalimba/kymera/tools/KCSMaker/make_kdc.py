############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd.
#
############################################################################
import subprocess
import sys
from tarfile import LNKTYPE
import types
import math
import capability
import get_python_tools
from kdc import KDCFile
from kcs import KCSFile
import parseObjFile
import parseMapOut
import logging
import os
import pickle as pickle
import collections

TAG_PM    = 0x9
TAG_VAR_DM = 0xA
TAG_DM_AS_PM = 0x9A
TAG_VAR_CONST16 = 0x1A
TAG_VAR_CONST = 0x2A
TAG_VAR_DMCONST16 = 0x3A
TAG_VAR_DMCONST24 = 0x4A
TAG_RELOC = 0xB
TAG_RELOC_CONST = 0x1B
TAG_INFO  = 0xC
TAG_SECT_CONST16 = 0x5
TAG_SECT_CONST = 0x6
TAG_END   = 0

RELOC_FIELDS = 5
RELOC_PM = 0
RELOC_DM_AS_PM = 0xFFFF
# These values are from the linker script.
DM_AS_PM_RAM_START_ADDRESS  = 0x200000
PM_RAM_START_ADDRESS  = 0x4200000
GET_VIRTUAL_ADDR_BITS_MASK = 0xF200000
# DM_AS_PM addresses will keep the top bit set to help with
# differentiating between DM_AS_PM and PM
# (DM_AS_PM addresses are in the form 0x2xxxxx and PM addresses
# are in the form 0x42xxxxx. That's how Kalaccess returns them.
# They need to be transformed into offsets relative to the start
# of the virtual addresses - DM_AS_PM_RAM_START_ADDRESS for DM_AS_PM
# and PM_RAM_START_ADDRESS for PM. However, DM_AS_PM addresses are
# kept in their initial form for processing, the top bit is masked
# at the final stage, when the relocation records are compiled.)
CLEAR_VIRTUAL_ADDR_BITS_MASK = GET_VIRTUAL_ADDR_BITS_MASK ^ 0xFFFFFFF

# RegC = MBS[...]
MBS_READ = 0
# RegC = MBU[...]
MBU_READ = 1
# RegC = MHS[...]
MHS_READ = 2
# RegC = MHU[...]
MHU_READ = 3
# RegC = M[...]
M_READ = 4
# MB[...] = RegC
MB_WRITE = 5
# MH[...] = RegC
MH_WRITE = 6
# M[...] = RegC
M_WRITE = 7

SUBWORD_B_IMM_NUM_BITS = 25
ADD_SUB_B_IMM_NUM_BITS = 26

INFO_SECTION_IO  = 1;
INFO_SECTION_CRC = 2;
INFO_SECTION_IFACE_VER = 3;
# the download routines need 1 extra word per block of memory for each variable
MALLOC_OVERHEAD = 1

# Offsets in KDC for CONST sections
RELOC_CONST_SECT_OFFSET_MS = 10
RELOC_CONST_SECT_OFFSET_LS = 11
CONST_16_SECT_OFFSET_MS = 12
CONST_16_SECT_OFFSET_LS = 13
CONST_SECT_OFFSET_MS = 14
CONST_SECT_OFFSET_LS = 15

# These should be in line with linkscript
DM_DNLD_CONST16_REGION_START=0
DM_DNLD_CONST16_REGION_END=0
DM_DNLD_CONST32_REGION_START=0
DM_DNLD_CONST32_REGION_END=0

# Region names
DM1_REGION_BASE = "$dm1_region.__Base$"
DM1_REGION_LIMIT = "$dm1_region.__Limit$"
DM2_REGION_BASE = "$dm2_region.__Base$"
DM2_REGION_LIMIT = "$dm2_region.__Limit$"
DM_REGION_BASE = "$dm_region.__Base$"
DM_REGION_LIMIT = "$dm_region.__Limit$"
DM_DNLD_CONST16_REGION_BASE = "$dm_dnld_const16_region.__Base$"
DM_DNLD_CONST16_REGION_LIMIT = "$dm_dnld_const16_region.__Limit$"
DM_DNLD_CONST32_REGION_BASE = "$dm_dnld_const32_region.__Base$"
DM_DNLD_CONST32_REGION_LIMIT = "$dm_dnld_const32_region.__Limit$"
DMCONST_DNLD_REGION_BASE = "$dmconst_dnld_region.__Base$"
DMCONST_DNLD_REGION_LIMIT = "$dmconst_dnld_region.__Limit$"

# BASE_HEADER_SIZE include the 20 16-bit values for:
#   1 (no of capabilities) + 1 (maxim sections) + 2 (no of vars) +
# + 2 (no of ELF records) + 2 (ELF ID) + 6 (const sizes) +
# + 6 (PM, DM1, DM2 sizes) 
BASE_HEADER_SIZE = 20
HEADER_FIELD_32 = 2
HEADER_FIELD_16 = 1

class make_kdc:
    def __init__(self, dnldElfFile, romElfFile, chip, objDumpPath, oFiles, cap_list, os_type, map_out, all_dm_as_pm = False, data_in_extra_heap = False):
        self.dnldElfFile = dnldElfFile
        self.romElfFile = romElfFile
        self.chip = chip
        self.objDumpPath = objDumpPath
        self.oFiles = oFiles
        self.cap_list = cap_list
        self.mapOut = map_out
        self.file_id = 0
        self.pm_size = 0
        self.dm_as_pm_size = 0
        self.dm1_size = 0
        self.dm2_size = 0
        self.os_type = os_type
        self.all_dm_as_pm = all_dm_as_pm
        self.data_in_extra_heap = data_in_extra_heap
        self.add_dm_as_pm_sections = False
        logging.info("Initialising make_kdc object...")

    def minim_is_prefix_instruction(self, instr, is_prefixed):
        if (is_prefixed):
            return ((instr & 0xf000) == 0xf000) or ((instr & 0xfff0) == 0xcfd0)
        else:
            return (instr & 0xf000) == 0xf000

    def isPrefix(self,insn):
        "Check if the instruction is a prefix instruction."
        # KalArch 4 has PreFix instruction that can have [20:0] as prefix
        # Other KalArchs have [7:0] prefix.
        return ((insn >> 21)<<5) == 0xFD00

    def isDMasPMaddr(self, addr):
        """ DM as PM addresses have the 26th bit set, whereas PM addresses
        do not."""
        if addr & DM_AS_PM_RAM_START_ADDRESS == DM_AS_PM_RAM_START_ADDRESS:
            return True
        return False
        
    def isSpecialPrefix(self,insn):
        "A special prefix is a 'P' string"
        return insn == "P"

    def getErrorFunction(self, rom):
        errorFunc = rom.sym.labelfind("$error$").addr
        # Assume that if it's a chip that supports MiniM, the error function will be linked in MiniM
        if self.chip.supports_minim():
            errorFunc |= 1
        return errorFunc

    def getInstructionSet(self, name, modLut, progLut):
        """ Given a function name, finds out which module it belongs to and returns the instruction set it has been linked with"""
        address = progLut[name]
        maxDiff = -1
        type = None
        for mod in modLut:
            if address >= modLut[mod][0] and (((address - modLut[mod][0]) < maxDiff) or maxDiff == -1):
                type = modLut[mod][1]
                maxDiff = address - modLut[mod][0]
        if type == None:
            raise Exception("Error. No module found that contains function: %s" % name)
        return type

    def buildVariableLut(self, dnld, rom, progLinks, modLut, varLinks, varSizes, extraVarSizes, mappings, dmprogLinks):
        """Simple function that goes through the variables in the download elf file
        makes a lookup table from name to 'index'.

        Where index is the order they will be created in, note this starts at 1."""

        class Symb:
            def __init__(self, nameSizeAddr, type, name = None, size = None, addr = None):
                if addr == None:
                    self.name = nameSizeAddr[0]
                    self.size = nameSizeAddr[1][0]
                    self.addr = nameSizeAddr[1][1]
                else:
                    self.name = name
                    self.size = size
                    self.addr = int(addr)
                self.type = type
                self.blck = None
                self.id   = None

            def getId(self):
                if self.blck is not None:
                    raise Exception("Symbol %s has been asked for its ID and it has a block" % self.name)
                if self.id is None:
                    raise Exception("Symbol %s has no ID" % self.name)
                return self.id

            def getIdAndOff(self, off):
                if self.blck is not None:
                    return (self.blck.getId(), self.addr - self.blck.addr + off)
                return (self.getId(), off)

            def isInBlock(self, other):
                base = other.addr - self.addr
                return (base >= 0) and (base < self.size)

            def __cmp__(self, other):
                """Sort the list by start address, with block variables coming
                before the children variables, so they can be detected."""
                if self.addr == other.addr:
                    if self.size == other.size:
                        return 0
                    if self.size > other.size:
                        return -1
                    return 1
                if self.addr <  other.addr:
                    return -1
                return 1

            def __lt__(self, other):
                if self.addr == other.addr:
                    if self.size > other.size:
                        return True
                elif self.addr < other.addr:
                    return True
                return False

            def __eq__(self, other):
                if isinstance(other, self.__class__):
                    return self.__dict__ == other.__dict__
                else:
                    return False

            def __ne__(self, other):
                return not self.__eq__(other)

            def __str__(self):
                return "SYMBOL_START\nname: " + str(self.name) + ", size: " + str(self.size) + ", addr: " + str(hex(self.addr)) + ", blck: " + str(self.blck) + ", id: " + str(self.id) + ", type: " + str(self.type) +"\nSYMBOL_END\n"

            def __repr__(self):
                return self.__str__()

        def getAddrFromAnonymousVariable(baseAddr, pos, dnld, instrSet, off, dm_as_pm_addr):
            """ Get address for variable by reading immediate in instruction """
            pmAddrs = sorted(dnld.sym.static_pm.keys())
            static_pm = [dnld.sym.static_pm[p] for p in pmAddrs]
            static_pm_minim = []
            # Get the Minim version of static PM for later
            for inst in static_pm:
                static_pm_minim.append(inst & 0xFFFF)
                static_pm_minim.append((inst & 0xFFFF0000) >> 16)
            if instrSet == "MAXIM":
                target = baseAddr//self.PC_INCR + (pos//self.PC_INCR)
                if dm_as_pm_addr is False:
                    target += self.dm_as_pm_size 
                if self.isPrefix(static_pm[target]):
                    if (static_pm[target+1] & 0xFF000000) == 0xF5000000:
                        # Subword access with 11-bit immediate + prefix
                        immediate = (static_pm[target] & 0x1FFFFF) << 11
                        immediate = immediate | (static_pm[target+1] & 0x7FF)
                    else:
                        # Normal instruction with 16-bit immediate
                        immediate = (static_pm[target] & 0xFFFF) << 16
                        immediate = immediate | (static_pm[target+1] & 0xFFFF)
                else:
                    immediate = static_pm[target] & 0xFFFF
            else:
                if (self.PC_INCR != 4):
                    raise Exception("Error, " + instrSet + " instruction set expecting byte addressable memory")
                # MiniM instructions are 16-bit, pos already in 16-bit if it's MiniM
                target = baseAddr // 2 + pos
                if dm_as_pm_addr is False:
                    target += self.dm_as_pm_size * self.PC_INCR // 2
                logging.debug("Fetching Minim instruction (anonymous)")
                prefixes = []
                is_prefix = False
                done = False
                while (done == False):
                    instr = static_pm_minim[target]
                    is_prefix = self.minim_is_prefix_instruction(instr, is_prefix)
                    if (is_prefix == True):
                        prefixes.append(instr)
                        target += 1
                    else:
                        done = True
                logging.debug("Minim instruction (anonymous): " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                if ((instr & 0xC000) == 0x4000):
                    logging.debug("MovAddB instruction (anonymous)")
                    # This is a prefixed move or add register instruction (MovAddB)
                    # Only register move is supported (Null + offset)
                    # This instruction just supports two prefix which are expected
                    if len(prefixes) != 2:
                        raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                    # Get immediate bits in instruction
                    immediate = instr & 0x3FF
                    immediate |= ((instr & 0x3800) >> 1)
                    # Get immediate bits in prefix
                    immediate |= ((prefixes[1] & 0x7F0) >> 4) << 13
                    # Get immediate bits in pre-prefix
                    immediate |= (prefixes[0] & 0xFFF) << 20
                elif ((instr & 0xC000) == 0x8000):
                    logging.debug("Subword instruction (anonymous): " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                    # This is a prefixed memory access instruction (Subword)
                    # Two prefixes expected as the linker places variables above 17-bit address space and one prefix
                    # supports just 13 bit addresses
                    if len(prefixes) != 2:
                        raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                    # Get immediate bits in instruction
                    immediate = instr & 0x1FF
                    rw_data_sel = instr & 0xE00
                    rw_data_sel = rw_data_sel >> 9
                    immediate |= ((instr & 0x3000) >> 3)
                    # Get immediate bits in prefix
                    immediate |= ((prefixes[1] & 0x300) >> 8) << 11
                    # Get immediate bits in pre-prefix
                    immediate |= (prefixes[0] & 0xFFF) << 13
                    # In KAL_ARCH4, literals in subword are compressed depening on the value of rw_data_sel
                    # In order to return the right address we need to uncompress it
                    if self.chip.does_dm_octet_addressing():
                        if immediate & (1 << (SUBWORD_B_IMM_NUM_BITS-1)):
                            immediate = immediate | ((0xFFFFFFFF << SUBWORD_B_IMM_NUM_BITS) & 0xFFFFFFFF)
                        if (rw_data_sel == M_WRITE) or (rw_data_sel == M_READ):
                            logging.debug("32 bit packed address, unpacking")
                            immediate = immediate * 4
                        elif (rw_data_sel == MHS_READ) or (rw_data_sel == MHU_READ) or (rw_data_sel == MH_WRITE):
                            logging.debug("16 bit packed address, unpacking")
                            immediate = immediate * 2
                        else:
                            logging.debug("unpacked address")
                        # Limit to 32-bit immediate after a possible address "unpacking"
                        immediate = immediate & 0xFFFFFFFF
                elif ((instr & 0xE000) == 0x2000):
                    logging.debug("Add/Sub (Type B) instruction (anonymous): " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                    # This is a prefixed Addition/Subtraction instruction
                    # Two prefixes expected
                    if len(prefixes) != 2:
                        raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                    # Get immediate bits in instruction
                    immediate = instr & 0x3FF
                    immediate |= ((instr & 0x1800) >> 1)
                    # Get immediate bits in prefix
                    immediate |= ((prefixes[1] & 0x300) >> 8) << 12
                    # Get immediate bits in pre-prefix
                    immediate |= (prefixes[0] & 0xFFF) << 14
                    if immediate & (1 << (ADD_SUB_B_IMM_NUM_BITS - 1)):
                        immediate = immediate | ((0xFFFFFFFF << ADD_SUB_B_IMM_NUM_BITS) & 0xFFFFFFFF)
                elif ((instr & 0xE000) == 0xC000):
                    logging.debug("Insert32 instruction (anonymous)")
                    # This is a prefixed Insert32 instruction
                    # This instruction only supports three prefixes which are expected
                    if len(prefixes) != 3:
                        raise Exception(
                            "Unsupported instruction type. Wrong number of prefixes: %d" %
                            len(prefixes))
                    # Get immediate bits in pre-pre-prefix
                    immediate = prefixes[0] & 0xFFF
                    immediate = ((immediate << 20) & 0xFFF00000)
                    # Get immediate bits in pre-prefix
                    immediate |= ((prefixes[1] & 0xFFF) << 8)
                    # Get immediate bits in prefix
                    immediate |= prefixes[2] & 0xFF
                elif ((instr & 0xF0F0) == 0xE050):
                    # This is a Push Constant instruction
                    logging.debug("Push Constant instruction (anonymous): " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                    # Two prefixes expected
                    if len(prefixes) != 2:
                        raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                    # Get immediate bits in instruction
                    immediate = instr & 0x000F
                    immediate |= (instr & 0x0F00) >> 4
                    # Get immediate bits in prefix
                    immediate |= (prefixes[1] & 0x0FFF) << 8
                    # Get immediate bits in pre-prefix
                    immediate |= (prefixes[0] & 0x0FFF) << 20
                else:
                    # Unsupported instruction type
                    raise Exception("Unsupported instruction type (anonymous): %s" % hex(instr))
            # Adjust address read from the opcode with the offset from the relocation record
            logging.debug("Address from opcode: " + hex(immediate))
            immediate = immediate - off
            logging.debug("Address adjusted: " + hex(immediate))
            return immediate

        def getAddrFromAnonymousVariableVarLut(varLut, varLink, lnk, dnld):
            address = None

            for v in varLut:
                logging.debug("We have: " + str(v))
                if v.name == varLink:
                    logging.debug("Address of {v} is {addr}:".format(v=v.name,addr=hex(v.addr)))
                    logging.debug("lnk pos:" + str(lnk.pos))
                    logging.debug("lnk off:" + str(lnk.off))
                    address = dnld.sym.static_dm[v.addr + lnk.pos] - lnk.off

            if address is None:
                logging.info("Warning, {} not found in varLut".format(varLink))

            return address

        def getRegions(regNameStart, regNameEnd, dnld):
            """ Get the value for the regions given in regNameStart and regNameEnd """
            REGION_ADDR_START = [x for x in list(dnld.sym.variables.items()) if x[0] == regNameStart]
            if len(REGION_ADDR_START) != 0:
                REGION_ADDR_END = [x for x in list(dnld.sym.variables.items()) if x[0] == regNameEnd]
                REGION_ADDR_START = REGION_ADDR_START[0][1][1]
                REGION_ADDR_END = REGION_ADDR_END[0][1][1]
            else:
                # The region label can also be found on the constants section of the ELF
                # This is the case when using ld instead of klink
                REGION_ADDR_START = [x for x in list(dnld.sym.constants.items()) if x[0] == regNameStart]
                if len(REGION_ADDR_START) != 0:
                    REGION_ADDR_END = [x for x in list(dnld.sym.constants.items()) if x[0] == regNameEnd]
                    REGION_ADDR_START = REGION_ADDR_START[0][1]
                    REGION_ADDR_END = REGION_ADDR_END[0][1]
                else:
                    return None, None
            if REGION_ADDR_START < REGION_ADDR_END:
                return REGION_ADDR_START, REGION_ADDR_END
            else:
                return None, None

        def getVarType(address):
            if DM_DNLD_CONST16_REGION_START != None and address >= DM_DNLD_CONST16_REGION_START and address <= DM_DNLD_CONST16_REGION_END:
                return "CONST16"
            elif DM_DNLD_CONST32_REGION_START != None and address >= DM_DNLD_CONST32_REGION_START and address <= DM_DNLD_CONST32_REGION_END:
                return "CONST"
            else:
                return "NOT_CONST"

        def filterVar(varname, ignore_prefixes, ignore_suffixes, mappings):
            origvarname = ""
            if varname in mappings:
                origvarname = varname
                varname = mappings[varname]
            for prefix in ignore_prefixes:
                if varname.startswith(prefix):
                    logging.debug("ignoring variable {v} ({o}): Starts with {p}".format(v=varname, o=origvarname, p=prefix))
                    return False
            for suffix in ignore_suffixes:
                if varname.endswith(suffix):
                    logging.debug("ignoring variable {v} ({o}): Ends with {s}".format(v=varname, o=origvarname, s=suffix))
                    return False
            return True

        def getFolders(path):
            # Consider using memoization
            folders = []
            while 1:
                path, folder = os.path.split(path)
                if folder != "":
                    folders.append(folder)
                else:
                    if path != "":
                        folders.append(path)
                    return folders

        global DM_DNLD_CONST16_REGION_START
        global DM_DNLD_CONST32_REGION_START

        ignore_prefixes = ["L_", "A_"]
        ignore_suffixes = ["__Base", "__Limit"]
        varFilt = lambda n: (filterVar(n[0],ignore_prefixes, ignore_suffixes, mappings))
        checksumFilt = lambda x: (x[0] == "__devtools_image_checksum")

        DM_DNLD_CONST32_REGION_START, DM_DNLD_CONST32_REGION_END = getRegions("$DM_DNLD_CONST32_REGION.__Base", "$DM_DNLD_CONST32_REGION.__Limit", dnld)
        DM_DNLD_CONST16_REGION_START, DM_DNLD_CONST16_REGION_END = getRegions("$DM_DNLD_CONST16_REGION.__Base", "$DM_DNLD_CONST16_REGION.__Limit", dnld)
        DMCONST16_DNLD_REGION_START, DMCONST16_DNLD_REGION_END = getRegions("$DMCONST16_DNLD_REGION.__Base", "$DMCONST16_DNLD_REGION.__Limit", dnld)
        DMCONST24_DNLD_REGION_START, DMCONST24_DNLD_REGION_END = getRegions("$DMCONST24_DNLD_REGION.__Base", "$DMCONST24_DNLD_REGION.__Limit", dnld)

        if DM_DNLD_CONST16_REGION_START != None:
            const16Filt = lambda n: n[1][1] >= DM_DNLD_CONST16_REGION_START and n[1][1] <= DM_DNLD_CONST16_REGION_END
        else:
            const16Filt = lambda n: False

        if DM_DNLD_CONST32_REGION_START != None:
            constFilt = lambda n: n[1][1] >= DM_DNLD_CONST32_REGION_START and n[1][1] <= DM_DNLD_CONST32_REGION_END
        else:
            constFilt = lambda n: False

        if DMCONST16_DNLD_REGION_START != None:
            DMConst16Filt = lambda n: n[1][1] >= DMCONST16_DNLD_REGION_START and n[1][1] <= DMCONST16_DNLD_REGION_END
        else:
            DMConst16Filt = lambda n: False

        if DMCONST24_DNLD_REGION_START != None:
            DMConst24Filt = lambda n: n[1][1] >= DMCONST24_DNLD_REGION_START and n[1][1] <= DMCONST24_DNLD_REGION_END
        else:
            DMConst24Filt = lambda n: False

        notConstItems = lambda n: not const16Filt(n) and not constFilt(n) and not DMConst16Filt(n) and not DMConst24Filt(n)

        varItems = list(filter(varFilt, list(dnld.sym.variables.items())))

        # At this point encrypted varItems might not have size information.
        # See if size information can be populated from kobjdump -h
        pos = 0
        for v in varItems:
            logging.debug(v)
            if v[0] in varSizes:
                logging.debug("FOUND!")
                # It should be an encrypted variable
                # Check size is effectively 0
                if v[1][0] != 0 and v[0] not in extraVarSizes:
                    raise Exception("Error. {} size 0 was expected".format(v))
                # rebuild the tuples
                logging.debug("new tuple:" + str(( v[1][0], (varSizes[v[0]], v[1][1], v[1][2], v[1][3], v[1][4]))))
                logging.debug("varItemsBefore:" + str(varItems))
                varItems[pos] = ( v[0], (varSizes[v[0]], v[1][1], v[1][2], v[1][3], v[1][4]))
                logging.debug("varItemsAfter:" + str(varItems))
            else:
                logging.debug("NOT FOUND!")
            pos = pos + 1
        logging.debug("varItems:" + str(varItems))


        file_id_addr = list(filter(checksumFilt, varItems))
        if len(file_id_addr) != 1:
            raise Exception("couldn't find one instance of __devtools_image_checksum in ELF file")
        # For some reason, __devtools_image_checksum length is set to 1 even in dm octet addressable chips
        # so we need to manually patch it to 4 on those chips
        if self.chip.does_dm_octet_addressing():
            for i in range(len(varItems)):
                if varItems[i][0] == "__devtools_image_checksum":
                    param1 = varItems[i][1][1]
                    param2 = varItems[i][1][2]
                    varItems[i] = ("__devtools_image_checksum", (4, param1, param2))

        file_id_addr = file_id_addr[0][1][1]
        self.file_id = dnld.sym.static_dm[file_id_addr]

        const16Items = list(filter( const16Filt, varItems ))
        constItems = list(filter( constFilt, varItems ))
        DMConst16Items = list(filter ( DMConst16Filt, varItems ))
        DMConst24Items = list(filter ( DMConst24Filt, varItems ))
        notConstItems =  list(filter( notConstItems, varItems ))

        # get the list of variables and remove the ones we don't care about
        varLut = [Symb(x, "NOT_CONST") for x in notConstItems]
        const16Lut = [Symb(x, "CONST16") for x in const16Items]
        constLut = [Symb(x, "CONST") for x in constItems]
        DMConst16Lut = [Symb(x, "DMCONST16") for x in DMConst16Items]
        DMConst24Lut = [Symb(x, "DMCONST24") for x in DMConst24Items]

        varLut.extend(const16Lut)
        varLut.extend(constLut)
        varLut.extend(DMConst16Lut)
        varLut.extend(DMConst24Lut)

        varLut.sort()
        # now find duplicates -> blocks
        prev = None
        for var in varLut:
            if prev is not None and prev.isInBlock(var):
                var.blck = prev
            else:
                prev = var

        # Add non-global variables referenced in prog links
        for prg in progLinks:
            logging.debug("dbg prg: " + str(prg))
            for lnk in [x for x in progLinks[prg] if x.type == x.TYPE_SE]:
                # An anonymous variable will have the object file name prefixed to its name separated by a '%' char
                if lnk.name.find("%") != -1:
                    # Non-global variables start with L_ or A_ in gcc compilers, but names might be encrypted,
                    # So check against the mapping table first
                    lnk_name_to_check = lnk.name[lnk.name.find("%")+1:]
                    if lnk_name_to_check in mappings:
                        lnk_name_to_check = mappings[lnk_name_to_check]
                    if lnk_name_to_check.startswith("L_") or lnk_name_to_check.startswith("A_") or lnk_name_to_check.startswith("rwgroup") or lnk_name_to_check.startswith("rogroup"):
                        logging.debug("dbg lnk: " + str(lnk))
                        # First see if we have size information for this variable in varSizes
                        if lnk.name in varSizes:
                            anonymous_size = varSizes[lnk.name]
                        else:
                            # We get the size of non-global variable through the object file header section
                            anonymous_size = parseObjFile.getAnonymousVariableSize(lnk.name.split("%")[0], lnk.name.split("%")[1], self.objDumpPath)
                        anonymous_addr =  getAddrFromAnonymousVariable(modLut[prg][0], lnk.pos, dnld, modLut[prg][1], lnk.off, False)
                        newSym = Symb([], getVarType(anonymous_addr), lnk.name, anonymous_size, anonymous_addr)
                        logging.debug("new Sym is: " + str(newSym))
                        if newSym not in varLut:
                            varLut.extend([newSym])
                            logging.debug("Added")
                        else:
                            logging.debug("NOT Added (Already in varLut)")
                            
        # Add non-global variables referenced in dmprog links
        for prg in dmprogLinks:
            logging.debug("dbg dmprg: " + str(prg))
            for lnk in [x for x in dmprogLinks[prg] if x.type == x.TYPE_SE]:
                # An anonymous variable will have the object file name prefixed to its name separated by a '%' char
                if lnk.name.find("%") != -1:
                    # Non-global variables start with L_ or A_ in gcc compilers, but names might be encrypted,
                    # So check against the mapping table first
                    lnk_name_to_check = lnk.name[lnk.name.find("%")+1:]
                    if lnk_name_to_check in mappings:
                        lnk_name_to_check = mappings[lnk_name_to_check]
                    if lnk_name_to_check.startswith("L_") or lnk_name_to_check.startswith("A_") or lnk_name_to_check.startswith("rwgroup") or lnk_name_to_check.startswith("rogroup"):
                        logging.debug("dbg lnk: " + str(lnk))
                        # First see if we have size information for this variable in varSizes
                        if lnk.name in varSizes:
                            anonymous_size = varSizes[lnk.name]
                        else:
                            # We get the size of non-global variable through the object file header section
                            anonymous_size = parseObjFile.getAnonymousVariableSize(lnk.name.split("%")[0], lnk.name.split("%")[1], self.objDumpPath)
                        anonymous_addr =  getAddrFromAnonymousVariable(modLut[prg][0]&CLEAR_VIRTUAL_ADDR_BITS_MASK, lnk.pos, dnld, modLut[prg][1], lnk.off, True)
                        newSym = Symb([], getVarType(anonymous_addr), lnk.name, anonymous_size, anonymous_addr)
                        logging.debug("new Sym is: " + str(newSym))
                        if newSym not in varLut:
                            varLut.extend([newSym])
                            logging.debug("Added")
                        else:
                            logging.debug("NOT Added (Already in varLut)")

        ignore_varLinks_libs = "kcc"
        prevVarLutSize = -1
        while len(varLut) != prevVarLutSize:
            logging.debug("varLut current size: {}".format(len(varLut)))
            logging.debug("varLut previous size: {}".format(prevVarLutSize))
            prevVarLutSize = len(varLut)
            # Add non-global variables referenced in varLinks
            for varLink in varLinks:
                logging.debug("[var] dbg varLink: " + str(varLink))
                folders = getFolders(varLink.split("%")[0])
                if not ignore_varLinks_libs in folders:
                    for lnk in [x for x in varLinks[varLink] if x.type == x.TYPE_SE]:
                        # An anonymous variable will have the object file name prefixed to its name separated by a '%' char
                        if lnk.name.find("%") != -1:
                            # Non-global variables start with L_ or A_ in gcc compilers, but names might be encrypted,
                            # So check against the mapping table first
                            lnk_name_to_check = lnk.name[lnk.name.find("%")+1:]
                            if lnk_name_to_check in mappings:
                                lnk_name_to_check = mappings[lnk_name_to_check]
                            if lnk_name_to_check.startswith("L_") or lnk_name_to_check.startswith("A_") or lnk_name_to_check.startswith("rwgroup") or lnk_name_to_check.startswith("rogroup"):
                                logging.debug("[var] dbg lnk: " + str(lnk))
                                # First see if we have size information for this variable in varSizes
                                if lnk.name in varSizes:
                                    anonymous_size = varSizes[lnk.name]
                                else:
                                    # We get the size of non-global variable through the object file header section
                                    anonymous_size = parseObjFile.getAnonymousVariableSize(lnk.name.split("%")[0], lnk.name.split("%")[1], self.objDumpPath)

                                anonymous_addr = getAddrFromAnonymousVariableVarLut(varLut, varLink, lnk, dnld)
                                if anonymous_addr is None:
                                    continue

                                newSym = Symb([], getVarType(anonymous_addr), lnk.name, anonymous_size, anonymous_addr)
                                logging.debug("[var] new Sym is: " + str(newSym))
                                if newSym not in varLut:
                                    varLut.extend([newSym])
                                    logging.debug("[var] Added")
                                else:
                                    logging.debug("[var] NOT Added (Already in varLut)")
                else:
                    logging.debug("[var] dbg varLink: " + str(varLink) + " ignored")
        # Do we still have any variable with zero size?
        zeroSizeSym = [x for x in varLut if x.size == 0 and x.blck == None]
        varLut = [x for x in varLut if x.size != 0 or x.blck != None]
        logging.debug("variables still with ZERO size: {}".format(zeroSizeSym))

        # Try to find any variables we might have missed using the map_out.txt
        mapOutVars = parseMapOut.getVariables(self.mapOut, self.getAddr(dnld, DM2_REGION_BASE))
        mapOutVars = [Symb(mapOutVars[x], mapOutVars[x][2], x, mapOutVars[x][1], mapOutVars[x][0]) for x in mapOutVars]

        for var in mapOutVars:
            # Try to find missing size information
            for var2 in zeroSizeSym:
                if var.addr == var2.addr:
                    if var.name != var2.name:
                        logging.info("map_out.txt and zeroSizeSym variable names don't match(1): {v1} AND {v2}".format(v1=var,v2=var2))
                    # Now check wether the variable we are adding already exists in varLut
                    varFound = False
                    for v in varLut:
                        if v.addr == var.addr:
                            logging.debug("Variable {v1} from mapOutVars already exists in varLut: {v2}, importing the size information: {s}".format(v1=var, v2=v, s=var.size))
                            v.size = var.size
                            varFound = True
                            break
                    if not varFound:
                        varLut.extend([var])
                        logging.debug("Added to varLut a variable that was missing size information from map_out.txt, (probably coming from C runtime libraries):{}".format(var))

            # Try to find missing variables in varLut
            varFound = False
            for var2 in varLut:
                if var.addr == var2.addr:
                    if var.name != var2.name:
                        logging.info("map_out.txt and varLut variable names don't match(2): {v1} AND {v2}".format(v1=var, v2=var2))
                    if var.size != var2.size:
                        raise Exception("map_out.txt and varLut variable sizes don't match(2): {v1} AND {v2}".format(v1=var, v2=var2))
                    varFound = True
                    break
            if not varFound:
                # missing variable, add it to varLut
                varLut.extend([var])
                logging.debug("Added to varLut a variable that was missing from map_out.txt, (probably coming from C runtime libraries):{}".format(var))

        # sort again
        varLut.sort()
        logging.debug("varLut now is: {}".format(str(varLut)))
        # assign the IDs
        count = 1
        count_blck = 0
        prev_var = None
        base_dm2 = self.getAddr(dnld, DM2_REGION_BASE)
        limit_dm2 = self.getAddr(dnld, DM2_REGION_LIMIT)
        processing_dm2 = False
        for var in varLut:
            if var.blck is not None:
                count_blck = count_blck + 1
                continue
            var.id = count
            logging.debug(var.name + " has been assigned ID: " + str(var.id))
            # Check variables are consecutive in memory. If there's been padding added by the linker
            # Adjust variable sizes accordingly
            if prev_var != None:
                if var.addr < limit_dm2:
                    if (var.addr >= base_dm2) and (not processing_dm2):
                        processing_dm2 = True
                    elif var.addr != (prev_var.addr + prev_var.size):
                        logging.debug(str(prev_var) + " reports size " + str(prev_var.size) + ", but next variable is: " + str(var))
                        padding = var.addr - (prev_var.addr + prev_var.size)
                        if padding > 3 or padding < 0:
                            raise Exception("Error. Needed padding is out of range: {}".format(padding))
                        logging.debug("Adding {p} bytes of padding to {v}".format(p=padding,v=prev_var.name))
                        prev_var.size = prev_var.size + padding
            prev_var = var
            count += 1
        # When generating a dict, we might not have the same amount of items if the key is duplicated
        # Make sure this hasn't happened
        dic_varLut = dict( [(v.name, v) for v in varLut] )
        if len(dic_varLut) != (count + count_blck -1):
            raise Exception("Error, varLut doesn't contain the expected amount of variables (%d vs %d)" % (len(dic_varLut), count + count_blck - 1))
        return dic_varLut

    def _get_label(self, dnld, kld_match_string, klink_match_string):
        # This function can be called with two different type of objects, add support here for both
        try:
            import kalaccess
            import kalelfreader_lib_wrappers
        except ImportError:
            from kal_python_tools import kalaccess
            from kal_python_tools import kalelfreader_lib_wrappers
        if dnld.__class__ == kalaccess.Kalaccess:
            if self.chip.uses_kld():
                return dnld.sym.anysymbolfind(kld_match_string + "$").address_or_value
            else:
                return dnld.sym.anysymbolfind(klink_match_string + "$").address_or_value
        elif dnld.__class__ == kalelfreader_lib_wrappers.Ker:
            if self.chip.uses_kld():
                val  = [l for l in dnld.get_all_symbols() if l[0].endswith(kld_match_string)]
            else:
                val  = [l for l in dnld.get_all_symbols() if l[0].endswith(klink_match_string)]
            if len(val) != 1:
                raise Exception("Expecting 1 label but found: %s" % val)
            return val[0][3]

    def getPMBase(self, dnld):
        kld_base_match_string = "MEM_MAP_CODE_START"
        klink_base_match_string ="__Base"
        # Disable top bit set by using ABSOLUTE(.) in the linkscript
        return 0x7FFFFFFF & self._get_label(dnld, kld_base_match_string, klink_base_match_string)

    def getPMLimit(self, dnld):
        kld_limit_match_string = "MEM_MAP_CODE_END"
        klink_limit_match_string ="__Limit"
        # Disable top bit set by using ABSOLUTE(.) in the linkscript
        return 0x7FFFFFFF & self._get_label(dnld, kld_limit_match_string, klink_limit_match_string)
    
    def getDMasPMLimit(self, dnld):
        kld_limit_match_string = "MEM_MAP_DM_CODE_END"
        # Disable top bit set by using ABSOLUTE(.) in the linkscript
        base = 0x7FFFFFFF & self._get_label(dnld, kld_limit_match_string, None)

        return base

    def buildProgrammeLut(self, dnld):
        """Simple function that goes through programme labels in the elf file and
        builds a list of name to address."""

        progLut = {}

        base = self.getPMBase(dnld)

        for (l, addr) in dnld.sym.labels:
            if l.endswith("__Limit"):
                continue
            if l.endswith("__Base"):
                continue
            if self.chip.supports_dm_as_pm():
                if addr & GET_VIRTUAL_ADDR_BITS_MASK == DM_AS_PM_RAM_START_ADDRESS:
                    # If it's a DM_AS_PM address, leave the 26th bit
                    # set to tell it apart from a PM address later on.
                    progLut[l] = addr & 0xFFFFFF
                else:
                    progLut[l] = addr & CLEAR_VIRTUAL_ADDR_BITS_MASK
            else:
                progLut[l] = addr - base

        return progLut

    def getDMasPMsize(self, dnld):
        """This returns the size of the DM as PM section, in words."""
        
        dm_as_pm_size_words = 0
        base = self.getPMBase(dnld)
        limit = self.getDMasPMLimit(dnld)
        
        dm_as_pm_size_words = limit - base
        dm_as_pm_size_words = int(math.ceil(dm_as_pm_size_words / 4.0))
        
        return dm_as_pm_size_words

    def buildModuleLut(self, func, ker, mappings):
        """Read the elf file and get the list of modules."""

        moduleLut = {}
        dm_as_pm_funcs = []

        base = self.getPMBase(ker)
        limit = self.getPMLimit(ker)
        mod_list = parseMapOut.getModules(self.mapOut, mappings)

        for funct in mod_list:
            logging.debug(funct.get_name())
            logging.debug(hex(funct.get_low_pc()))
            logging.debug(hex(funct.get_high_pc()))
            logging.debug(hex(base))
            logging.debug(hex(limit))
            # These cases filters out the functions that are not linked in
            if (funct.get_high_pc() > limit) or (funct.get_low_pc() == funct.get_high_pc()):
                logging.debug("Function not linked in")
                continue
            # Function information stored in func comes from object files and names might have changed, check there are
            # not any inconsistencies
            if (funct.get_name() == "$M.kdc_start__minim"):
                logging.debug(funct.get_name() + " module found!")
            # Make sure that all modules ind mod_list have an entry in func
            found = False
            for f in func:
                if f == funct.get_name():
                    if found == True:
                        raise Exception("Error. " + str(funct) +  " appears twice. All functions must have unique names (including static)")
                    found = True
                    logging.debug(f + " found in func: " + funct.get_name())
            if not found:
                raise Exception("Error. " + funct.get_name() +  " NOT found")
            else:
                if self.chip.supports_dm_as_pm():
                    if funct.get_low_pc() & GET_VIRTUAL_ADDR_BITS_MASK == DM_AS_PM_RAM_START_ADDRESS:
                        # DM_AS_PM code sections are in the format 0x2xxxxx
                        # The 26th bit is used to differentiate between DM as PM
                        # addresses and PM.
                        dm_as_pm_funcs.append(funct.get_name())
                        low_pc = funct.get_low_pc() & 0xFFFFFF
                    else:
                        # The top bits are masked for PM addresses
                        low_pc = funct.get_low_pc() & CLEAR_VIRTUAL_ADDR_BITS_MASK
                else:
                    low_pc = funct.get_low_pc() - base
                func[funct.get_name()].insert(0, low_pc)
                moduleLut[funct.get_name()] = func[funct.get_name()]
                logging.debug("moduleLut:" + str(moduleLut))
                logging.debug("dm_as_pm_funcs: " + str(dm_as_pm_funcs))
        return (moduleLut, dm_as_pm_funcs)

    def fixupCodeOffsets(self, progLinks, modLut, dnld, pm_section = False):
        """Symbol references are generated by processing object files. However when
        the object files are created the linker has not run so all references are
        assumed to be absolute. When the linker runs (assuming --relax switch), some
        of these prefixes may be removed. As such we need to fix up the reference
        lists.

        Note we only care about the destination value, the target address is
        retrieved from the elf file, i.e. after the linker has run and
        absolute/relative addressing has been taken into account."""

        pmAddrs = sorted(dnld.sym.static_pm.keys())
        static_pm = [dnld.sym.static_pm[p] for p in pmAddrs]
        static_pm_minim = []
        # Get the Minim version of static PM for later
        for inst in static_pm:
            static_pm_minim.append(inst & 0xFFFF)
            static_pm_minim.append((inst & 0xFFFF0000) >> 16)
        #print "progLinks" + str(progLinks)
        #print "modLut" + str(modLut)
        #print map(hex,static_pm)
        for prog in progLinks:
            baseAddr = modLut[prog][0]
            baseAddr = baseAddr & CLEAR_VIRTUAL_ADDR_BITS_MASK
            instrSet = modLut[prog][1]
            logging.debug("Processing module: " + str(prog) + str(modLut[prog]))
            # TODO this string could be dmprogLinks for DM as PM
            logging.debug("proglinks[" + str(prog) + "]:" + str(progLinks[prog]))
            mismatch = 0
            if instrSet == "MAXIM":
                for ref in progLinks[prog]:
                    logging.debug(ref)
                    ref.updatePos(ref.pos - mismatch)
                    logging.debug(ref)
                    target = baseAddr//self.PC_INCR + (ref.pos//self.PC_INCR)
                    if pm_section:
                        target += self.dm_as_pm_size

                    # if this instruction does not have a prefix, its the next
                    # instruction that goes wrong
                    if not self.isPrefix(static_pm[target]):
                        mismatch += (1*self.PC_INCR)
            else:
                if (self.PC_INCR != 4):
                    raise Exception("Error, " + instrSet + " instruction set expecting byte addressable memory")
                target = baseAddr // 2 # MiniM instructions are 16-bit
                if pm_section:
                    target += self.dm_as_pm_size * 2
                logging.debug(list(map(hex,static_pm_minim)))
                # Process Minim links
                # First count how many instructions away is this link
                instrCount = 0
                cur_offset = 0
                offset_in_minim = 0
                is_prefix = False
                if len(modLut[prog]) != 3:
                    raise Exception("Error. Module {} doesn't contain disassembly for MINIM section".format(prog))
                for ref in progLinks[prog]:
                    pos_opcode = ref.pos // self.PC_INCR
                    while (cur_offset < pos_opcode):
                        # When current opcode is not a prefix, add 1 instruction
                        # These instructions are extracted from object files and thus are MaxiM
                        if not self.isSpecialPrefix(modLut[prog][2][cur_offset]):
                            instrCount = instrCount + 1
                        cur_offset = cur_offset + 1
                    # Now we now how many instructions we have to traverse before patching the reference
                    # in MiniM
                    logging.debug("Minim instruction to patch inside current module: " + str(instrCount))
                    while (instrCount > 0):
                        is_prefix = self.minim_is_prefix_instruction(static_pm_minim[target], is_prefix)
                        if (is_prefix == False):
                            logging.debug("Opcode " + str(hex(static_pm_minim[target])) + " is NOT a prefix")
                            instrCount = instrCount - 1
                        else:
                            logging.debug("Opcode " + str(hex(static_pm_minim[target])) + " is a prefix")
                        target += 1
                        logging.debug("target: " + str(target))
                        offset_in_minim += 1
                        logging.debug("offset in minim: " + str(offset_in_minim))
                    logging.debug(ref)
                    # Update link with the number of MiniM opcodes traversed
                    ref.updatePos(offset_in_minim)
                    logging.debug(ref)

    def getAddr(self, dnld, region):
        try:
            import kal_sym
        except ImportError:
            from kal_python_tools import kal_sym
        if self.chip.uses_kld():
            try:
                constfind_list = dnld.sym.constfind(region)
            except kal_sym.SymbolSearchError:
                raise
            if len(constfind_list) > 1:
                raise Exception("Error. constant %s found more than once" % region)
            if len(constfind_list) == 1:
                return constfind_list.pop()[1]
            else:
                raise kal_sym.SymbolSearchError(0,0,0)
        else:
            try:
                varfound = dnld.sym.varfind(region).addr
            except kal_sym.SymbolSearchError:
                raise
            return varfound

    def processVariableLinks(self, varLinks, varLut, progLut, dnld, rom, modLut, kdc_idx):
        """Function to take the two sets of information we have on variables, size
        and initialisation values from the elf file and link information from the o
        files, and merge them into a single source.

        Note, the relocation information includes references to symbols in the
        ROM. Hence the elf file for the ROM application needs to be provided as well
        to ensure nothing is missed. However, linking does not need to be changed
        for this as these addresses will not be moving and so the linker work is
        acceptable."""
        try:
            import kal_sym
        except ImportError:
            from kal_python_tools import kal_sym


        def dmLookup(addr, pref):
            for (base, limit, flag) in pref:
                if addr >= base and addr < limit:
                    return flag
            raise Exception("Address is invalid: 0x%06X" % addr)

        # now get the regions to work out what the required preference was
        DM_PREF = []
        try:
            BASE  = self.getAddr(dnld, DM1_REGION_BASE)
        except kal_sym.SymbolSearchError:
            pass
        else:
            LIMIT = self.getAddr(dnld, DM1_REGION_LIMIT)
            #FLAG  = dnld.sym.constfind("$malloc.dm_preference.DM1$").value
            # We can't extract this information from elf as it's # defined and the preprocessor removes the symbol
            FLAG  = 1
            DM_PREF.append( (BASE, LIMIT, FLAG) )
        try:
            BASE  = self.getAddr(dnld, DM2_REGION_BASE)
        except kal_sym.SymbolSearchError:
            pass
        else:
            LIMIT = self.getAddr(dnld, DM2_REGION_LIMIT)
            #FLAG  = dnld.sym.constfind("$malloc.dm_preference.DM2$").value
            # We can't extract this information from elf as it's # defined and the preprocessor removes the symbol
            FLAG = 2
            DM_PREF.append( (BASE, LIMIT, FLAG) )
        try:
            BASE  = self.getAddr(dnld, DM_REGION_BASE)
        except kal_sym.SymbolSearchError:
            pass
        else:
            LIMIT = self.getAddr(dnld, DM_REGION_LIMIT)
            #FLAG  = dnld.sym.constfind("$malloc.dm_preference.NO_PREFERNCE$").value
            # We can't extract this information from elf as it's # defined and the preprocessor removes the symbol
            FLAG = 3
            DM_PREF.append( (BASE, LIMIT, FLAG) )
        try:
            BASE  = self.getAddr(dnld, DM_DNLD_CONST16_REGION_BASE)
        except kal_sym.SymbolSearchError:
            pass
        else:
            LIMIT = self.getAddr(dnld, DM_DNLD_CONST16_REGION_LIMIT)
            #FLAG  = dnld.sym.constfind("$malloc.dm_preference.NO_PREFERNCE$").value
            # We can't extract this information from elf as it's # defined and the preprocessor removes the symbol
            FLAG = 3
            DM_PREF.append( (BASE, LIMIT, FLAG) )
        try:
            BASE  = self.getAddr(dnld, DM_DNLD_CONST32_REGION_BASE)
        except kal_sym.SymbolSearchError:
            pass
        else:
            LIMIT = self.getAddr(dnld, DM_DNLD_CONST32_REGION_LIMIT)
            #FLAG  = dnld.sym.constfind("$malloc.dm_preference.NO_PREFERNCE$").value
            # We can't extract this information from elf as it's # defined and the preprocessor removes the symbol
            FLAG = 3
            DM_PREF.append( (BASE, LIMIT, FLAG) )
        try:
            BASE  = self.getAddr(dnld, DMCONST_DNLD_REGION_BASE)
        except kal_sym.SymbolSearchError:
            pass
        else:
            LIMIT = self.getAddr(dnld, DMCONST_DNLD_REGION_LIMIT)
            #FLAG  = dnld.sym.constfind("$malloc.dm_preference.NO_PREFERNCE$").value
            # We can't extract this information from elf as it's # defined and the preprocessor removes the symbol
            FLAG = 3
            DM_PREF.append( (BASE, LIMIT, FLAG) )

        class Variable:
            def __init__(self, var, dm_pref, static_data, patch):
                self.var         = var
                self.dm_pref     = dm_pref
                self.static_data = static_data
                self.patch       = patch

            def __cmp__(self, other):
                if self.var.id == other.var.id:
                    return 0
                if self.var.id <  other.var.id:
                    return -1
                return 1

            def __lt__(self, other):
                if self.var.id < other.var.id:
                    return True
                return False

            def __str__(self):
                return "VARIABLE_START\nvar: " + str(self.var) + ", dm_pref: " + str(self.dm_pref) + ", static_data: " + str(self.static_data) + ", patch: " + str(self.patch) + "\nVARIABLE_END\n"

            def __repr__(self):
                return self.__str__()

        errorFunc = self.getErrorFunction(rom)

        variables = []

        logging.debug("DM_PREF: " + str(DM_PREF))

        for (name, v) in list(varLut.items()):
            logging.debug(name)
            logging.debug(v)
            # we don't care about variables that are within blocks, we only include
            # the block
            if v.blck is not None:
                continue

            dm     = dmLookup(v.addr, DM_PREF)
            # Make static hold either words or octets depending on architecture
            if self.chip.does_dm_octet_addressing():
                unalignment = v.addr % 4
                # We can only read from word aligned addresses
                static_32 = [dnld.sym.static_dm.get(a, 0) for a in range(v.addr & ~0x3, v.addr + v.size + unalignment, 4)]
                static_bytes = v.size * [None]
                word_32 = static_32[unalignment//4]
                word_32 = word_32 >> 8*unalignment
                bytes_left = 4 - unalignment
                for i in range(v.size):
                    static_bytes[i] = word_32 & 0xFF
                    word_32 = word_32 >> 8
                    bytes_left = bytes_left - 1
                    if bytes_left == 0 and i < (v.size - 1):
                        word_32 = static_32[(i + 1 + unalignment)//4]
                        bytes_left = 4
                static = static_bytes
            else:
                static = [dnld.sym.static_dm.get(a, 0) for a in range(v.addr, v.addr+v.size)]

            links  = varLinks.get(v.name, [])
            logging.debug("static: " + str(list(map(hex,static))))
            # now go through the list of links
            patch = []
            for lnk in links:
                logging.debug("progLut:" + str(progLut))
                logging.debug("varLut:" + str(varLut))
                logging.debug("lnk: " + str(lnk))
                # varLut contains object file name prefix for non-global symbols
                if lnk.name in varLut:
                    logging.debug(str(lnk.name) + " found in varLut!")
                    logging.debug(varLut[lnk.name])
                    # Variable in DM has a reference in DM, normal case
                    # OR Variable in DM has a reference in CONST (descriptor) AND it's a streamed KCS (We patch descriptors)
                    if ((v.type == "NOT_CONST" or v.type.startswith("DMCONST")) and (varLut[lnk.name].type == "NOT_CONST" or varLut[lnk.name].type.startswith("DMCONST"))) or \
                    ((v.type == "NOT_CONST" or v.type.startswith("DMCONST")) and (varLut[lnk.name].type.startswith("CONST")) and (self.chip.get_default_kcs_type() == KCSFile.STREAM_TYPE)):
                        logging.debug("adding patch")
                        #print lnk.pos
                        # patch static DM
                        static[lnk.pos] = 0
                        # Assume 32 bit pointers when generating KCS for a dm_octet_addressing platform
                        if self.chip.does_dm_octet_addressing():
                            static[lnk.pos+1] = 0
                            static[lnk.pos+2] = 0
                            static[lnk.pos+3] = 0
                        # build the patch entry
                        id, off = varLut[lnk.name].getIdAndOff( lnk.off )
                        
                        patch.append((id, lnk.pos, off))
                    # Variable in DM has a reference in CONST, it's a descriptor and it's a random access KCS, don't patch it
                    elif (v.type == "NOT_CONST" or v.type.startswith("DMCONST")) and (varLut[lnk.name].type.startswith("CONST")):
                        logging.debug(str(lnk.name) + " is a " + varLut[lnk.name].type + ", ignoring this patch entry. Assuming " + str(v.name) + " is a descriptor and patch base_id instead")
                        # Patch base_id with current KDC index
                        static[2] = kdc_idx
                        logging.debug("Also patch descriptor with var_id " + str(lnk.name) + "(0x%06X)" % varLut[lnk.name].id)
                        # Patch var_id so we can find relocation section when table gets loaded
                        static[3] = varLut[lnk.name].id
                        logging.debug("base_id and var_id patched, new static: " +  str(list(map(hex,static))))
                    elif (v.type.startswith("CONST") and (varLut[lnk.name].type == "NOT_CONST" or varLut[lnk.name].type == "DMCONST")):
                        logging.debug("Relocation of variable " + v.name + ", which is a " + v.type + " with variable " + varLut[lnk.name].name + ", which is a " + varLut[lnk.name].type)
                        # patch static DM
                        static[lnk.pos] = 0
                        if self.chip.does_dm_octet_addressing():
                            static[lnk.pos+1] = 0
                            static[lnk.pos+2] = 0
                            static[lnk.pos+3] = 0
                        # Assume 32 bit pointers when generating KCS for a dm_octet_addressing platform
                        # build the patch entry
                        id, off = varLut[lnk.name].getIdAndOff( lnk.off )
                        
                        patch.append((id, lnk.pos, off))
                    # Other cases currently unsupported
                    else:
                        raise Exception("Error. Unsupported relocation for variable " + v.name + ", which is a " + v.type + " with variable " + varLut[lnk.name].name + ", which is a " + varLut[lnk.name].type)
                # progLut doesn't contain object file name prefix for non-global symbols, so use
                # a global name that removes the prefix
                proglut_name = lnk.name
                if proglut_name.find("%") != -1:
                    proglut_name = proglut_name[proglut_name.find("%")+1:]

                if proglut_name in progLut:
                    logging.debug(str(lnk.name) + " found in progLut!, adding patch")
                    # patch static DM
                    if self.chip.does_dm_octet_addressing():
                        static[lnk.pos]   = (errorFunc) & 0xFF
                        static[lnk.pos+1] = (errorFunc>>8) & 0xFF
                        static[lnk.pos+2] = (errorFunc>>16) & 0xFF
                        static[lnk.pos+3] = (errorFunc>>24) & 0xFF
                    else:
                        static[lnk.pos] = errorFunc
                    type = self.getInstructionSet(proglut_name, modLut, progLut)
                    off = progLut[proglut_name] + lnk.off
                    is_target_dm_as_pm_addr = self.isDMasPMaddr(off)
                    off = off & CLEAR_VIRTUAL_ADDR_BITS_MASK
                    if type == "MINIM":
                        off = (off // 2) | 0x80000000
                    else:
                        off = off // self.PC_INCR
                    if self.chip.supports_dm_as_pm():
                        if is_target_dm_as_pm_addr:
                            patch.append((RELOC_DM_AS_PM, lnk.pos, off))
                        else:
                            
                            patch.append((RELOC_PM, lnk.pos, off))
                    else:
                        patch.append((0, lnk.pos, off))
            # now sort the entry
            patch.sort()

            # check the static data, we can lose trailing zeros, which may mean we
            # lose the lot
            lastZero = None
            for i in range( len(static)-1, -1, -1):
                if static[i] != 0:
                    break
                lastZero = i
            static = static[:lastZero]

            variables.append(Variable(v, dm, static, patch))

        # finally sort the variables by id
        variables.sort()

        return variables

    def processProgrammeLinks(self, progLinks, varLut, progLut, modLut, dnld, rom, pm_section = True):
        """Generate a list of variable and code patching in the programme.

        It checks the addresses are valid: they lie within the base and limit range
        for the section and they are contiguous."""

        # get the program addresses and check they are sane
        pmAddrs = sorted(dnld.sym.static_pm.keys())
        if pmAddrs[0] < self.getPMBase(dnld):
            errStr  = "Program appears to be linked below start of region:\n  Base: "
            errStr += "0x%06X, address: 0x%06X" % (self.getPMBase(dnld), pmAddrs[0])
            raise Exception(errStr)
        if pmAddrs[-1] >= self.getPMLimit(dnld):
            errStr  = "Program appears to be linked beyond end of region:\n  Limit: "
            errStr += "0x%06X, address: 0x%06X" % (self.getPMLimit(dnld), pmAddrs[-1])
            raise Exception(errStr)
        
        if pm_section:
            # Check for gaps in pmAddr
            for (p, n) in zip(pmAddrs[:-1], pmAddrs[1:]):
                if (p+(1*self.PC_INCR)) != n:
                    # There is an expected gap between DM as PM and PM
                    if (p & GET_VIRTUAL_ADDR_BITS_MASK == DM_AS_PM_RAM_START_ADDRESS and \
                            n & GET_VIRTUAL_ADDR_BITS_MASK == PM_RAM_START_ADDRESS) is False:
                        raise Exception("Program not contiguous, gap between: 0x%06X & 0x%06X" % (p, n))
        else:
            pmAddrs = pmAddrs[:self.dm_as_pm_size]
        
        errorFunc = self.getErrorFunction(rom)

        patch = []
        
        logging.debug("static pm: " + str(dnld.sym.static_pm))
        static_pm = [dnld.sym.static_pm[p] for p in pmAddrs]
        logging.debug("progLinks:" + str(progLinks))
        for prg in progLinks:
            for lnk in [x for x in progLinks[prg] if x.type == x.TYPE_SE]:
                logging.debug("--------------------------------------------")

                if lnk.name in varLut:
                    logging.debug(str(lnk.name) + " found in varLut!, adding patch(2)")
                    if modLut[prg][1] == "MAXIM":
                        addr = ((modLut[prg][0] & CLEAR_VIRTUAL_ADDR_BITS_MASK) + lnk.pos) // self.PC_INCR
                        if self.chip.supports_dm_as_pm() and pm_section:
                            addr += self.dm_as_pm_size
                        # patch the static data
                        if self.isPrefix(static_pm[addr]):
                            if ((static_pm[addr+1] & 0xFF000000) == 0xF5000000):
                                # SubWord(B) Instruction
                                static_pm[addr] = static_pm[addr] & 0xFFE00000
                                static_pm[addr+1] = static_pm[addr+1] & 0xFFFFF800
                            else:
                                static_pm[addr] = static_pm[addr] & 0xFFFF0000
                                static_pm[addr+1] = static_pm[addr+1] & 0xFFFF0000
                        else:
                            if ((static_pm[addr] & 0xFF000000) == 0xF5000000):
                                raise Exception("Currently unsupported instruction: %s" % str(hex(static_pm[addr])))
                            static_pm[addr] = static_pm[addr] & 0xFFFF0000
                        # build the patch entry
                        id, off = varLut[lnk.name].getIdAndOff( lnk.off )
                        if self.chip.supports_dm_as_pm():
                            if pm_section:
                                addr -= self.dm_as_pm_size
                                
                            patch.append( (id, addr, off) )
                        else:
                            patch.append( (id, addr, off) )
                        logging.debug("patch:" + str(patch))
                    else:
                        logging.debug("Fetching Minim instruction")
                        prefixes = []
                        is_prefix = False
                        # Get base minim address
                        addr = (modLut[prg][0] & CLEAR_VIRTUAL_ADDR_BITS_MASK) // 2
                        # Get the instruction minim address
                        addr += lnk.pos
                        if self.chip.supports_dm_as_pm() and pm_section:
                            addr += self.dm_as_pm_size * self.PC_INCR // 2
                        done = False
                        while (done == False):
                            # Get the maxim address and offset
                            addrMax = addr // 2
                            offMax = addr % 2
                            instr = static_pm[addrMax]
                            if (offMax == 0):
                                instr = instr & 0x0000FFFF
                            else:
                                instr = (instr & 0xFFFF0000) >> 16
                            is_prefix = self.minim_is_prefix_instruction(instr,is_prefix)
                            if (is_prefix == True):
                                prefixes.append(instr)
                                addr +=1
                            else:
                                done = True

                        logging.debug("Minim instruction: " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                        # Sanity check we must have at least one prefix
                        if ((instr & 0xC000) == 0x8000):
                            logging.debug("Subword instruction")
                            # This is a prefixed memory access instruction (Subword)
                            # Two prefixes expected as the linker places variables above 17-bit address space and one prefix
                            # supports just 13 bit addresses
                            if len(prefixes) != 2:
                                raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                            # Zero immediate fields in instruction
                            instr = instr & 0xCE00
                            # Zero immediate fields in prefix
                            prefixes[1] = prefixes[1] & 0xFCFF
                            # Zero immediate fields in pre-prefix
                            prefixes[0] = prefixes[0] & 0xF000
                        elif ((instr & 0xC000) == 0x4000):
                            logging.debug("MovAddB instruction")
                            # This is a prefixed move or add register instruction (MovAddB)
                            # Only register move is supported (Null + offset)
                            # This instruction just supports two prefix which are expected
                            if len(prefixes) != 2:
                                raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                            # Zero immediate fields in instruction
                            instr = instr & 0xC400
                            # Zero immediate fields in prefix
                            prefixes[1] = prefixes[1] & 0xF80F
                            # Zero immediate fields in pre-prefix
                            prefixes[0] = prefixes[0] & 0xF000
                        elif ((instr & 0xE000) == 0x2000):
                            logging.debug("Add/Sub (Type B) instruction: " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                            # This is a prefixed Addition/Subtraction instruction
                            # Two prefixes expected
                            if len(prefixes) != 2:
                                raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                            # Zero immediate fields in instruction
                            instr = instr & 0xE400
                            # Zero immediate fields in prefix
                            prefixes[1] = prefixes[1] & 0xFCFF
                            # Zero immediate fields in pre-prefix
                            prefixes[0] = prefixes[0] & 0xF000
                        elif ((instr & 0xE000) == 0xC000):
                           # This is an Insert32 instruction
                           logging.debug("Insert32 instruction: " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                           # Three prefixes expected
                           if len(prefixes) != 3:
                                raise Exception("Insert32 instruction. 3 prefixes expected, but %d found" % len(prefixes))
                           logging.debug("(Type B - 3 prefixes)")
                           prefixes[2] = prefixes[2] & 0xFF00
                           prefixes[1] = prefixes[1] & 0xF000
                           prefixes[0] = prefixes[0] & 0xF000
                        elif ((instr & 0xF0F0) == 0xE050):
                            # This is a Push Constant instruction
                            logging.debug("Push Constant instruction: " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                            # Two prefixes expected
                            if len(prefixes) != 2:
                                raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                            # Zero immediate fields in instruction
                            instr = instr & 0xF0F0
                            prefixes[1] = prefixes[1] & 0xF000
                            prefixes[0] = prefixes[0] & 0xF000
                        else:
                            # Unsupported instruction type
                            raise Exception("Unsupported instruction type: %s" % hex(instr))
                        # Generate an array with the final result
                        patchedInstr = prefixes
                        patchedInstr.append(instr)
                        # Patch PM
                        addr = (modLut[prg][0] & CLEAR_VIRTUAL_ADDR_BITS_MASK) // 2
                        # Get the instruction MiniM address
                        addr += lnk.pos
                        # Save for later
                        addrSaved = addr
                        if self.chip.supports_dm_as_pm() and pm_section:
                            addr += self.dm_as_pm_size * self.PC_INCR // 2
                        logging.debug("Minim instruction patched: " + str(list(map(hex,patchedInstr))))
                        for newinstr in patchedInstr:
                            addrMax = addr // 2
                            offMax = addr % 2
                            if (offMax == 0):
                                static_pm[addrMax] &= 0xFFFF0000
                                static_pm[addrMax] |= (newinstr & 0x0000FFFF)
                            else:
                                static_pm[addrMax] &= 0x0000FFFF
                                static_pm[addrMax] |= (newinstr << 16)
                            addr += 1
                        # build the patch entry
                        id, off = varLut[lnk.name].getIdAndOff( lnk.off )
                        
                        # Set top bit to mark the patch destination address as MiniM
                        patch.append( (id, addrSaved | 0x80000000, off) )
                        logging.debug("patch:" + str(patch))

                # progLut doesn't contain object file name prefix for non-global symbols, so use
                # a global name that removes the prefix
                proglut_name = lnk.name
                if proglut_name.find("%") != -1:
                    proglut_name = proglut_name[proglut_name.find("%")+1:]

                if proglut_name in progLut:
                    logging.debug(str(lnk.name) + " found in progLut!, adding patch(2)")
                    if modLut[prg][1] == "MAXIM":
                        addr = ((modLut[prg][0] & CLEAR_VIRTUAL_ADDR_BITS_MASK) + lnk.pos) // self.PC_INCR
                        if self.chip.supports_dm_as_pm() and pm_section:
                            addr += self.dm_as_pm_size
                        # patch static data - all download addresses use a prefix
                        if ((static_pm[addr+1] & 0xFF000000) == 0xF5000000):
                            raise Exception("Currently unsupported instruction: %s" % str(hex(static_pm[addr+1])))
                        static_pm[addr+0] = (static_pm[addr+0] & 0xFFFF0000) + (errorFunc >>    16)
                        static_pm[addr+1] = (static_pm[addr+1] & 0xFFFF0000) + (errorFunc & 0xFFFF)
                        off = progLut[proglut_name] + lnk.off
                        is_target_dm_as_pm_addr = self.isDMasPMaddr(off)
                        off = off & CLEAR_VIRTUAL_ADDR_BITS_MASK
                        # Work out whether the lnk is in Minim or Maxim address space
                        type = self.getInstructionSet(proglut_name, modLut, progLut)
                        if type == "MINIM":
                            off = (off // 2) | 0x80000000
                        else:
                            off = off // self.PC_INCR
                        if self.chip.supports_dm_as_pm():
                            if pm_section:
                                addr -= self.dm_as_pm_size
                            if is_target_dm_as_pm_addr:
                                patch.append( (RELOC_DM_AS_PM, addr, off) )
                            else:
                                patch.append( (RELOC_PM, addr, off) )
                        else:
                            patch.append( (0, addr, off) )
                        logging.debug("patch:" + str(patch))
                    else:
                        logging.debug("Fetching Minim instruction")
                        logging.debug("Error function located at:" + str(hex(errorFunc)))
                        prefixes = []
                        is_prefix = False
                        # Get base minim address
                        addr = (modLut[prg][0] & CLEAR_VIRTUAL_ADDR_BITS_MASK) // 2
                        # Get the instruction minim address
                        addr += lnk.pos
                        if self.chip.supports_dm_as_pm() and pm_section:
                            addr += self.dm_as_pm_size * self.PC_INCR // 2
                        done = False
                        while (done == False):
                            # Get the maxim address and offset
                            addrMax = addr // 2
                            offMax = addr % 2
                            instr = static_pm[addrMax]
                            if (offMax == 0):
                                instr = instr & 0x0000FFFF
                            else:
                                instr = (instr & 0xFFFF0000) >> 16
                            is_prefix = self.minim_is_prefix_instruction(instr,is_prefix)
                            if (is_prefix == True):
                                prefixes.append(instr)
                                addr +=1
                            else:
                                done = True

                        logging.debug("Minim instruction: " + str(list(map(hex,prefixes))) + " " + str(hex(instr)))
                        # Only MovAddB, Push Constant and Add/Sub (Type B) instructions are allowed when referencing download code
                        if ((instr & 0xC000) == 0x4000):
                            logging.debug("MovAddB instruction")
                            # This is a prefixed move or add register instruction (MovAddB)
                            # Only register move is supported (Null + offset)
                            # This instruction must contain two prefixes when dealing with a PM address
                            if len(prefixes) != 2:
                                raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                            # Check Mv/+ field matches 0 which means "register move"
                            if ((instr & 0x400) != 0):
                                raise Exception("Unsupported instruction type. Wrong Mv/+ field: %d" % (instr & 0x400)>>10)
                            # Zero immediate fields in instruction
                            instr = instr & 0xC400
                            # Add error function
                            instr = instr | (errorFunc & 0x3FF) | ((errorFunc & 0x1C00) << 1)
                            # Zero immediate fields in prefix
                            prefixes[1] = prefixes[1] & 0xF80F
                            # Add error function
                            prefixes[1] = prefixes[1] | (((errorFunc >> 13) & 0x7F) << 4)
                            prefixes[0] = 0xF000 | ((errorFunc >> 20) & 0xFFF)
                        elif ((instr & 0xF0F0) == 0xE050):
                            logging.debug("Push Constant instruction")
                            # This is a prefixed Push Constant instruction
                            # This instruction must contain two prefixes when dealing with a PM address
                            if len(prefixes) != 2:
                                raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                            # Zero immediate fields in instruction
                            instr = instr & 0xF0F0
                            # Add error function
                            instr = instr | (errorFunc & 0xF) | ((errorFunc & 0xF0) << 4)
                            # Zero immediate fields in prefix
                            prefixes[1] = prefixes[1] & 0xF000
                            # Add error function
                            prefixes[1] = prefixes[1] | ((errorFunc >> 8) & 0xFFF)
                            prefixes[0] = 0xF000 | ((errorFunc >> 20) & 0xFFF)
                        elif ((instr & 0xE000) == 0x2000):
                            logging.debug("Add/Sub (Type B)")
                            # This is a prefixed Add/Sub (Type B) instruction
                            # This instruction must contain THREE prefixes when dealing with a PM address
                            if len(prefixes) != 3:
                                raise Exception("Unsupported instruction type. Wrong number of prefixes: %d" % len(prefixes))
                            # Zero immediate fields in instruction
                            instr = instr & 0xE400
                            # Add error function
                            instr = instr | (errorFunc & 0x3FF) | ((errorFunc & 0xC00) << 1)
                            # Zero immediate fields in prefix
                            prefixes[2] = prefixes[2] & 0xFCFF
                            # Add error function
                            prefixes[2] = prefixes[2] | (((errorFunc >> 12) & 0x3) << 8)
                            prefixes[1] = 0xF000 | ((errorFunc >> 14) & 0xFFF)
                            prefixes[0] = 0xF000 | ((errorFunc >> 26) & 0xFFF)
                        else:
                            # Unsupported instruction type
                            raise Exception("Unsupported instruction type: %s" % hex(instr))
                        # Generate an array with the final result
                        patchedInstr = prefixes
                        patchedInstr.append(instr)
                        # Patch PM
                        addr = (modLut[prg][0] & CLEAR_VIRTUAL_ADDR_BITS_MASK) // 2
                        # Get the instruction MiniM address
                        addr += lnk.pos
                        # Save for later
                        addrSaved = addr
                        if self.chip.supports_dm_as_pm() and pm_section:
                            addr += self.dm_as_pm_size * self.PC_INCR // 2
                        logging.debug("Minim instruction patched: " + str(list(map(hex,patchedInstr))))
                        for newinstr in patchedInstr:
                            addrMax = addr // 2
                            offMax = addr % 2
                            if (offMax == 0):
                                static_pm[addrMax] &= 0xFFFF0000
                                static_pm[addrMax] |= (newinstr & 0x0000FFFF)
                            else:
                                static_pm[addrMax] &= 0x0000FFFF
                                static_pm[addrMax] |= (newinstr << 16)
                            addr += 1
                        # build the patch entry
                        off = progLut[proglut_name] + lnk.off
                        is_target_dm_as_pm_addr = self.isDMasPMaddr(off)
                        off = off & CLEAR_VIRTUAL_ADDR_BITS_MASK
                        # Work out whether the lnk is in Minim or Maxim address space
                        type = self.getInstructionSet(proglut_name, modLut, progLut)
                        if type == "MINIM":
                            off = (off // 2) | 0x80000000
                        else:
                            off = off // self.PC_INCR
                        # Set top bit to mark the patch destination address as MiniM
                        if self.chip.supports_dm_as_pm():
                            # check offset here if it's in dm_as_pm
                            # addrSaved is PM
                            if is_target_dm_as_pm_addr:
                                patch.append( (RELOC_DM_AS_PM, addrSaved | 0x80000000, off) )
                            else:
                                patch.append( (RELOC_PM, addrSaved | 0x80000000, off) )
                        else:
                            patch.append( (0, addrSaved | 0x80000000, off) )
                        logging.debug("patch:" + str(patch))

        patch.sort()
        
        if self.add_dm_as_pm_sections and pm_section:
            if self.dm_as_pm_size != 0:
                # The DM as PM section is at the beginning, therefore the first
                # addresses are not included
                return (static_pm[(self.dm_as_pm_size):], patch)
            else:
                return (static_pm, patch)
        else:
            return (static_pm, patch)

    def getMaximAddresses(self, modLut, dnld):
        """For targets that support MiniM, most of the code will be in MiniM, but we need to differentiate between the two
        while programming PM so that we can fix up immediates in MiniM branches but ignore the ones in MaxiM"""
        instrSetMaxim = []
                
        # Since MaxiM addresses will be 32-bit aligned, compress the offset dividing by 4
        startAddr = [(modLut[x][1],modLut[x][0] // 4) for x in modLut]
        # Sort by start address
        startAddr.sort(key = lambda x: x[1])
        processingMaxim = False
        for type, addr in startAddr:
            if (type == "MAXIM") and (processingMaxim == False):
                savedMaximStartAddr = addr
                processingMaxim = True
            elif (type =="MINIM") and (processingMaxim == True):
                # If it's a MiniM address but we had a start address for Maxim
                # save end address
                instrSetMaxim.append((savedMaximStartAddr, addr - 1))
                processingMaxim = False
        # If we were processing a Maxim start address but we didn't find a MiniM start address afterwards
        if (processingMaxim == True):
            # Get last address in elf
            static_pm = sorted(dnld.sym.static_pm.keys())
            # Remove initial PM offset and compress address
            instrSetMaxim.append((savedMaximStartAddr, (static_pm[-1] - static_pm[0])//4 ))
        return instrSetMaxim

    def addInfoSection(self, kdc, instrSetMaxim, ELFRecords, num_vars, dm_as_pm_instrSetMaxim):
        """Function to start the kdc file off. See design document CS-323058-DD for details"""
        #size is num_capabilities, num_maxim_sections & CRC field + 12 fields per capability + 2 fields per maxim section
        num_ELF_records = len(ELFRecords)
        numcap = kdc.getNumCapabilities()
        if self.chip.supports_dm_banks_powered_off():
            if self.chip.supports_dm_as_pm():
                # There are 4 extra size information field: DM1 extra heap, DM2 extra heap, DM_AS_PM size 
                # (all 32-bit) and DM as PM no of maxim sections (16-bit).
                size    = BASE_HEADER_SIZE + 3 * HEADER_FIELD_32 + numcap * 2 + \
                                len(instrSetMaxim) * 2 + num_ELF_records * 6 + \
                                len(dm_as_pm_instrSetMaxim) * 2 + HEADER_FIELD_16
            else:
                # Two extra 32-bit fields for the sizes of DM1 and DM2 extra heaps
                size    = BASE_HEADER_SIZE + 2 * HEADER_FIELD_32 + numcap * 2 + \
                                len(instrSetMaxim) * 2 + num_ELF_records * 6
        else:
            if self.chip.supports_dm_as_pm():
                # Two extra fields for DM as PM size info and DM as PM no of maxim sections
                size    = BASE_HEADER_SIZE + HEADER_FIELD_32 + numcap * 2 + len(instrSetMaxim) * 2 + \
                                num_ELF_records * 6 + len(dm_as_pm_instrSetMaxim) * 2 + HEADER_FIELD_16
            else:
                size    = BASE_HEADER_SIZE + numcap * 2 + len(instrSetMaxim) * 2 + num_ELF_records * 6
        
        size_ms = size >> 16
        size_ls = size & 0xFFFF
        dm1_extra_heap_size = 0
        dm2_extra_heap_size = 0
        
        kdc.append( (TAG_INFO << 8) + size_ms, "INFO TAG" )
        kdc.append( size_ls )
        kdc.append(numcap, "Number of capabilities: %d" % numcap)
        kdc.append(len(instrSetMaxim), "Number of maxim sections: %d" % len(instrSetMaxim))
        kdc.append(num_vars & 0xFFFF, "Number of variables: %d" % num_vars)
        kdc.append((num_vars >> 16) & 0xFFFF )
        kdc.append(num_ELF_records & 0xFFFF, "Number of ELF records: %d" % num_ELF_records)
        kdc.append((num_ELF_records >> 16) & 0xFFFF )
        kdc.append(self.file_id & 0xFFFF, "ELF FILE ID: 0x%0X" % self.file_id)
        kdc.append((self.file_id >> 16) & 0xFFFF)
        # The following entries for RELOC CONST, CONST16 and CONST are overwritten
        # in addDmReloc(),addCONST16() and addCONST().
        kdc.append(0 , "RELOC CONST Section offset from start of KDC: 0")
        kdc.append(0)
        kdc.append(0 , "CONST16 Section offset from start of KDC: 0")
        kdc.append(0)
        kdc.append(0 , "CONST Section offset from start of KDC: 0")
        kdc.append(0)
        if self.chip.supports_dm_as_pm():
            kdc.append(len(dm_as_pm_instrSetMaxim), "Number of DM_AS_PM maxim sections: %d" % len(dm_as_pm_instrSetMaxim))
        if self.chip.supports_dm_as_pm():
            kdc.supports_dm_as_pm = True
            if self.all_dm_as_pm:
                self.dm_as_pm_size = self.pm_size
                self.pm_size = 0
        if self.chip.supports_dm_banks_powered_off():
            kdc.supports_dm_banks_powered_off = True
            if self.data_in_extra_heap:
                dm1_extra_heap_size = self.dm1_size
                dm2_extra_heap_size = self.dm2_size
                self.dm1_size = 0
                self.dm2_size = 0
        kdc.append(self.pm_size & 0xFFFF, "PM allocate (in 32-bit words): %d - 0x%0X" % (self.pm_size, self.pm_size))
        kdc.append((self.pm_size >> 16) & 0xFFFF)
        kdc.PM_size = self.pm_size * 4
        if self.chip.supports_dm_as_pm():
            kdc.append(self.dm_as_pm_size & 0xFFFF, "DM as PM allocate (in 32-bit words): %d - 0x%0X" % (self.dm_as_pm_size, self.dm_as_pm_size))
            kdc.append((self.dm_as_pm_size >> 16) & 0xFFFF)
            kdc.DM_as_PM_size = self.dm_as_pm_size * 4
        kdc.append(self.dm1_size & 0xFFFF, "DM1 allocate (in bytes): %d - 0x%0X" % (self.dm1_size, self.dm1_size))
        kdc.append((self.dm1_size >> 16) & 0xFFFF)
        kdc.DM1_size = self.dm1_size
        if self.chip.supports_dm_banks_powered_off():
            kdc.append(dm1_extra_heap_size & 0xFFFF, "DM1 extra heap allocate (in bytes): %d - 0x%0X" % (dm1_extra_heap_size, dm1_extra_heap_size))
            kdc.append((dm1_extra_heap_size >> 16) & 0xFFFF)
            if self.dm1_size == 0:
                kdc.DM1_extra_size = dm1_extra_heap_size
        kdc.append(self.dm2_size & 0xFFFF, "DM2 allocate (in bytes): %d - 0x%0X" % (self.dm2_size, self.dm2_size))
        kdc.append((self.dm2_size >> 16) & 0xFFFF)
        kdc.DM2_size = self.dm2_size
        if self.chip.supports_dm_banks_powered_off():
            kdc.append(dm2_extra_heap_size & 0xFFFF, "DM2 extra heap allocate (in bytes): %d - 0x%0X" % (dm2_extra_heap_size, dm2_extra_heap_size))
            kdc.append((dm2_extra_heap_size >> 16) & 0xFFFF)
            kdc.DM2_extra_size = dm2_extra_heap_size
        for cap in kdc.getCapabilities():
            kdc.append(cap.getCapabilityId(), "Cap ID: %d" % cap.getCapabilityId())
            kdc.append(cap.getSymCapData(), "Symbol for CAPABILITY_DATA structure: %d" % cap.getSymCapData())
        for startAddr, endAddr in instrSetMaxim:
            if self.chip.does_pm_octet_addressing():
                kdc.append( startAddr &  0xFFFF, "Start Addr (x 4): %d - 0x%0X" % (startAddr * 4, startAddr * 4) )
                kdc.append( endAddr   &  0xFFFF, "End Addr (x 4 + 3): %d - 0x%0X" % (endAddr * 4 - 1, endAddr * 4 + 3) )
            else:
                raise Exception("Chips with no PM octet addressing aren't supported yet")
        if self.chip.supports_dm_as_pm():
            for startAddr, endAddr in dm_as_pm_instrSetMaxim:
                if self.chip.does_pm_octet_addressing():
                    kdc.append( startAddr &  0xFFFF, "DM as PM Start Addr (x 4): %d - 0x%0X" % (startAddr * 4, startAddr * 4) )
                    kdc.append( endAddr   &  0xFFFF, "DM as PM End Addr (x 4 + 3): %d - 0x%0X" % (endAddr * 4 - 1, endAddr * 4 + 3) )
                else:
                    raise Exception("Chips with no PM octet addressing aren't supported yet")
        for record in ELFRecords:
            kdc.append(record[0] & 0xFFFF, "Mem type: %s. Offset: %d - 0x%0X" % ((record[0] & 0xC0000000) >> 30, record[0] & 0x3FFFFFFF, record[0] & 0x3FFFFFFF))
            kdc.append((record[0] >> 16) & 0xFFFF)
            kdc.append(record[1] & 0xFFFF, "Length: 0x%0X" % record[1])
            kdc.append((record[1] >> 16) & 0xFFFF)
            kdc.append(record[2] & 0xFFFF, "File address: 0x%0X" % record[2])
            kdc.append((record[2]>> 16) & 0xFFFF)


    def addPmSection(self, kdc, static_pm, tag):
        """Function to read the static PM information from the elf file and write it
        into the kdc file."""

        # Work out total size
        size    = len(static_pm) * 2
        # Split size in MS and LS
        size_ms = (size >> 16) & 0xFF
        size_ls =  size        & 0xFFFF

        section_string = ""
        if tag == TAG_PM:
            section_string = "PM section"
        else:
            section_string = "DM as PM section"
        # Write TAG for PM section + MS part of size
        kdc.append( (tag) << 8 + size_ms, section_string)
        # Write rest of size
        kdc.append(                 size_ls, "Size: %d - 0x%0X" % (size, size))
        for pm in static_pm:
            # Write 16 bits of a 32 bit PM word
            kdc.append( pm &  0xFFFF )
            # Write the 16 bits left
            kdc.append( pm >>     16 )
        return

    def addVarSections(self, kdc, variables):
        """Function to add the variable sections, one per variable."""

        for v in variables:
            # build the entry
            # Only include the offset from start of CONSTx section
            sectSize = 2
            if (v.var.type == "CONST16"):
                kdc.append( (TAG_VAR_CONST16 << 8) + ((sectSize >> 16) & 0xFF), "%s CONST16 Variable" % v.var.name )
                kdc.append( sectSize & 0xFFFF )
                if self.chip.does_dm_octet_addressing():
                    # DM_DNLD_CONST16_REGION_START could not be word aligned, but
                    # the linker will append necessary zeroes so that first variable
                    # starts word aligned
                    constOff16 = v.var.addr - ((DM_DNLD_CONST16_REGION_START+3)//4)*4
                else:
                    constOff16 = v.var.addr - DM_DNLD_CONST16_REGION_START
                if constOff16 < 0:
                    raise Exception("Error. Invalid constant variable address or DM_DNLD_CONST16_REGION_START value")
                kdc.append( constOff16 & 0xFFFF, "Offset from start of CONST16: %d - 0x%0X" % (constOff16, constOff16))
                kdc.append( (constOff16 >> 16) & 0xFFFF )
            elif (v.var.type == "CONST"):
                kdc.append( (TAG_VAR_CONST << 8) + ((sectSize >> 16) & 0xFF), "%s CONST Variable" % v.var.name )
                kdc.append( sectSize & 0xFFFF )
                if self.chip.does_dm_octet_addressing():
                    # DM_DNLD_CONST_REGION_START could not be word aligned, but
                    # the linker will append necessary zeroes so that first variable
                    # starts word aligned
                    constOff = v.var.addr - ((DM_DNLD_CONST32_REGION_START+3)//4)*4
                else:
                    constOff = v.var.addr - DM_DNLD_CONST32_REGION_START
                if constOff < 0:
                    raise Exception("Error. Invalid constant variable address or DM_DNLD_CONST_REGION_START value")
                kdc.append( constOff & 0xFFFF, "Offset from start of CONST: %d - 0x%0X" % (constOff, constOff))
                kdc.append( (constOff >> 16) & 0xFFFF )
            else:
                # section size is: length(2), preference and static
                if self.chip.does_dm_octet_addressing():
                    sectSize = 2 + 1 + (len(v.static_data) + 1) // 2
                else:
                    sectSize = 2 + 1 + (len(v.static_data)*3+1)//2
                if (v.var.type == "DMCONST16"):
                    kdc.append( (TAG_VAR_DMCONST16 << 8) + ((sectSize >> 16) & 0xFF), "%s DMCONST16 Variable" % v.var.name )
                elif (v.var.type == "DMCONST24"):
                    kdc.append( (TAG_VAR_DMCONST24 << 8) + ((sectSize >> 16) & 0xFF), "%s DMCONST24 Variable" % v.var.name )
                else:
                    kdc.append( (TAG_VAR_DM << 8) + ((sectSize >> 16) & 0xFF), "%s DM Variable" % v.var.name )
                kdc.append( sectSize & 0xFFFF )
                kdc.append( v.var.size & 0xFFFF, "Variable size: %d - 0x%0X" % (v.var.size, v.var.size))
                kdc.append( (v.var.size >> 16) & 0xFFFF )
                kdc.append( v.dm_pref )

                # Add variables static data to KDC
                if self.chip.does_dm_octet_addressing():
                    for i in range(len(v.static_data)):
                        if i & 1:
                            kdc.append( (v.static_data[i-1] & 0xFF) + ((v.static_data[i] << 8) & 0xFF00))
                    if len(v.static_data) & 1:
                        kdc.append( (v.static_data[-1] & 0xFF))
                else:
                    for i in range(len(v.static_data)):
                        if i & 1:
                            kdc.append( ((v.static_data[i-1] & 0xFF) << 8) + ((v.static_data[i] >> 16) & 0xFF) )
                            kdc.append( v.static_data[i] & 0xFFFF )
                        else:
                            kdc.append( (v.static_data[i] >> 8 ) & 0xFFFF )

                    if len(v.static_data) & 1:
                        kdc.append( (v.static_data[-1] & 0xFF) << 8 )

    def addPmReloc(self, kdc, program, varLut, tag):
        """Add the PM (or DM_AS_PM) relocation section to the KDC file."""

        # is there actually anything to do
        if len(program) == 0:
            return

        relocation_string = ""
        if tag == RELOC_PM:
            relocation_string = "PM relocation section"
        else:
            relocation_string = "DM as PM relocation section"
        # start the section
        
        size    = 1 + len(program) * 5
        size_ms = size >> 16
        size_ls = size & 0xFFFF

        # add the section header
        kdc.append( (TAG_RELOC << 8) + size_ms, relocation_string )
        kdc.append(                    size_ls )

        # say that it targets PM or DM_AS_PM
        kdc.append( tag )

        # now add the individual sections
        for (sym, offd, offt) in program:
            if sym == RELOC_PM:
                kdc.append( sym, "Target symbol     : PM" )
            elif sym == RELOC_DM_AS_PM:
                kdc.append( sym, "Target symbol     : DM as PM" )
            else:
                kdc.append( sym, "Target symbol     : %s" % sym)
            # Is it a MiniM offset?
            if (offd & 0x80000000):
                kdc.append( (offd >> 16) & 0xFFFF, "Destination offset: %4d (0x%06X) (MiniM)" % (offd & 0x7FFFFFFF, offd & 0x7FFFFFFF))
                kdc.append( offd & 0xFFFF)
            else:
                kdc.append( (offd >> 16) & 0xFFFF, "Destination offset: %4d (0x%06X) (Maxim)" % (offd, offd))
                kdc.append( offd & 0xFFFF)
            if sym == RELOC_PM or sym == RELOC_DM_AS_PM:
                if (offt & 0x80000000):
                    kdc.append( (offt >> 16) & 0xFFFF, "Target offset: %4d (0x%06X) (MiniM)" % (offt & 0x7FFFFFFF, offt & 0x7FFFFFFF))
                    kdc.append( offt & 0xFFFF)
                else:
                    kdc.append( (offt >> 16) & 0xFFFF, "Target offset: %4d (0x%06X) (Maxim)" % (offt, offt))
                    kdc.append( offt & 0xFFFF)
            else:
                kdc.append( (offt >> 16) & 0xFFFF, "Target offset     : %4d (0x%06X)" % (offt, offt))
                kdc.append( offt & 0xFFFF)
        return

    def addDmReloc(self, kdc, variables):
        """Add the DM relocation sections to the kdc files."""

        for var in variables:
            if len(var.patch) == 0:
                continue
            # calculate the size of this section, include the destination variable ID
            
            size    = 1 + 5 * len(var.patch)
            size_ms = size >> 16
            size_ls = size & 0xFFFF
            if (var.var.type.startswith("DMCONST") or var.var.type == "NOT_CONST"):
                kdc.append( (TAG_RELOC << 8) + size_ms, "%s relocation section (DM)" % var.var.name )
            else:
                if kdc[RELOC_CONST_SECT_OFFSET_LS][0] == 0 and kdc[RELOC_CONST_SECT_OFFSET_MS][0] == 0:
                    if self.chip.get_default_kcs_type() == KCSFile.RANDOM_ACCESS_TYPE:
                        # Add an end section here to stop parser trying to parse CONSTs relocation sections
                        self.addEndSection(kdc)
                        self.ensurePadding32(kdc)
                    relocConstStartAddr = len(kdc) * 2
                    kdc[RELOC_CONST_SECT_OFFSET_MS] = ((relocConstStartAddr >> 16) & 0xFFFF, "RELOC CONST Section offset from start of KDC: 0x%0X" % relocConstStartAddr)
                    kdc[RELOC_CONST_SECT_OFFSET_LS] = (relocConstStartAddr & 0xFFFF, "")
                kdc.append( (TAG_RELOC_CONST << 8) + size_ms, "%s relocation section (CONST)" % var.var.name )
            kdc.append(                    size_ls, "Size: %d - 0x%0X" % (size, size)      )
            kdc.append(                     var.var.getId()                                )
            
            for p in var.patch:
                if p[0] == RELOC_PM or p[0] == RELOC_DM_AS_PM:
                    if p[0] == RELOC_PM:
                        kdc.append(p[0], "target: PM")
                    else:
                        kdc.append(p[0], "target: DM as PM")
                    kdc.append((p[1] >> 16) & 0xFFFF, "Destination offset: %4d (0x%06X)" % (p[1], p[1]))
                    kdc.append(p[1] & 0xFFFF)
                    if (p[2] & 0x80000000):
                        kdc.append((p[2] >> 16) & 0xFFFF, "Target offset: %4d (0x%06X) (MiniM)" % (p[2] & 0x7FFFFFFF, p[2] & 0x7FFFFFFF ) )
                        kdc.append(p[2] & 0xFFFF)
                    else:
                        kdc.append((p[2] >> 16) & 0xFFFF, "Target offset: %4d (0x%06X) (Maxim)" % (p[2], p[2]) )
                        kdc.append(p[2] & 0xFFFF)
                else:
                    kdc.append(p[0], "target: %s" % var.var.name)
                    kdc.append((p[1] >> 16) & 0xFFFF, "Destination offset: %4d (0x%06X)" % (p[1], p[1]))
                    kdc.append(p[1] & 0xFFFF)
                    kdc.append((p[2] >> 16) & 0xFFFF, "Target offset: %4d (0x%06X)" % (p[2], p[2]) )
                    kdc.append(p[2] & 0xFFFF)
        return

    def addEndSection(self, kdc):
        "Add the end tag"
        kdc.append( 0, "End" )
        return

    def addCONST16(self, kdc, variables):
        """Function to add the CONST16 data"""
        const16SectStartAddr = len(kdc)
        kdc.append( 0, "CONST16 SECTION START")
        kdc.append( 0 )
        if self.chip.get_default_kcs_type() == KCSFile.STREAM_TYPE:
            # No use for this when streaming a KCS
            const16StartAddr = 0
        else:
            const16StartAddr = len(kdc) * 2
        kdc[CONST_16_SECT_OFFSET_MS] = ((const16StartAddr >> 16) & 0xFFFF, "CONST16 Section offset from start of KDC: 0x%0X" % const16StartAddr)
        kdc[CONST_16_SECT_OFFSET_LS] = (const16StartAddr & 0xFFFF, "")
        sectSize = 0
        if self.chip.does_dm_octet_addressing():
            for v in variables:
                for i in range(len(v.static_data)):
                    if i & 1:
                        kdc.append( (v.static_data[i-1] & 0xFF) + ((v.static_data[i] << 8) & 0xFF00))
                # Append zeros until variable size is reached
                for j in range(i, v.var.size - 1):
                    if i & 1:
                        kdc.append(0)
                sectSize = sectSize + (v.var.size // 2)
        else:
            for v in variables:
                for i in range(len(v.static_data)):
                    kdc.append(v.static_data[i] & 0xFFFF)
                    sectSize = sectSize + 1
                # Append zeros until variable size is reached
                for j in range(i, v.var.size - 1):
                    kdc.append(0)
                    sectSize = sectSize + 1
        # Now we know the size of the const16 section, so patch it
        kdc[const16SectStartAddr] = ((TAG_SECT_CONST16 << 8) + ((sectSize >> 16) & 0xFF), "CONST16 SECTION START (Size: %d - 0x%0X)" % (sectSize, sectSize))
        kdc[const16SectStartAddr + 1] = (( sectSize & 0xFFFF ), "")
        kdc.CONST16_size = sectSize

    def addCONST(self, kdc, variables):
        """Function to add the CONST data"""
        constSectStartAddr = len(kdc)
        kdc.append( 0, "CONST SECTION START")
        kdc.append( 0 )
        if self.chip.get_default_kcs_type() == KCSFile.STREAM_TYPE:
            # No use for this when streaming a KCS
            constStartAddr = 0
        else:
            constStartAddr = len(kdc) * 2
        kdc[CONST_SECT_OFFSET_MS] = ((constStartAddr >> 16) & 0xFFFF, "CONST Section offset from start of KDC: 0x%0X" % constStartAddr)
        kdc[CONST_SECT_OFFSET_LS] = (constStartAddr & 0xFFFF, "")
        sectSize=0
        if self.chip.does_dm_octet_addressing():
            for v in variables:
                if len(v.static_data) != 0:
                    for i in range(len(v.static_data)):
                        if (i & 3) == 3:
                            kdc.append( (v.static_data[i-3] & 0xFF) + ((v.static_data[i-2] << 8) & 0xFF00))
                            kdc.append( (v.static_data[i-1] & 0xFF) + ((v.static_data[i] << 8) & 0xFF00))
                    # Flush any octets left
                    if (i & 3) != 3:
                        static_left = 4*[0]
                        remaining = (i + 1) % 4
                        for k in range(remaining):
                            static_left[k] = v.static_data[i-remaining+k+1]
                        kdc.append( (static_left[0] & 0xFF) + ((static_left[1] << 8) & 0xFF00))
                        kdc.append( (static_left[2] & 0xFF) + ((static_left[3] << 8) & 0xFF00))
                # Append zeros until variable size is reached, start from the next multiple
                # of 4 after len(static_data), as the region between len(static_data) and round_up(len(static_data),4)
                # will have been padded with zeroes in the previous loop
                for j in range(((len(v.static_data)+3)//4)*4, v.var.size):
                    if j & 1:
                        kdc.append(0x0)
                sectSize = sectSize + (v.var.size // 2)
        else:
            for v in variables:
                for i in range(len(v.static_data)):
                        kdc.append( (v.static_data[i] >> 16) & 0xFF )
                        kdc.append( v.static_data[i] & 0xFFFF )
                # Append zeros until variable size is reached
                for j in range(len(v.static_data), v.var.size):
                    kdc.append(0)
                    kdc.append(0)
                sectSize = sectSize + v.var.size * 2
        # Now we know the size of the const section, so patch it
        kdc[constSectStartAddr] = ((TAG_SECT_CONST << 8) + ((sectSize >> 16) & 0xFF), "CONST SECTION START (Size: %d - 0x%0X)" % (sectSize, sectSize))
        kdc[constSectStartAddr + 1] = (( sectSize & 0xFFFF ), "")
        kdc.CONST_size = sectSize

    def ensurePadding32(self, kdc):
        """Ensure we finish at a 32-bit boundary"""
        if (((len(kdc) * 2) % 4) != 0):
            kdc.append(0, "Add padding to ensure 32-bit word alignment")

    def parseELF(self, ker, dnld, len_static_pm):
        DOWNLOAD_ELF_SECTION_PM = 0
        DOWNLOAD_ELF_SECTION_DM1 = 1
        DOWNLOAD_ELF_SECTION_DM2 = 2
        DOWNLOAD_ELF_SECTION_DM_AS_PM = 3
        total_pm_size = 0

        records = []
        section_hdrs = ker.get_elf_section_headers()
        pm_section_keys = [x for x in section_hdrs if (section_hdrs[x].type == ker.KerElfSectionHeaderInfo.ELF_SECTION_TYPE_MAXIM) or (section_hdrs[x].type == ker.KerElfSectionHeaderInfo.ELF_SECTION_TYPE_MINIM)]
        if len(pm_section_keys) == 0:
            raise Exception("Error. Couldn't find any PM section in ELF file")
        # Sort pm_section_keys by section start address
        pm_section_keys = sorted(pm_section_keys, key=lambda x: section_hdrs[x].address)
        
        # Add PM section to the records
        prev_end_addr = 0
        for pm_sect in pm_section_keys:
            # If the end address of the previous section doesn't match the start address of the current one
            # add padding
            if prev_end_addr != 0:
                # Make sure this is not at the boundary between DM as PM and PM
                if (prev_end_addr & GET_VIRTUAL_ADDR_BITS_MASK == DM_AS_PM_RAM_START_ADDRESS and \
                        section_hdrs[pm_sect].address & GET_VIRTUAL_ADDR_BITS_MASK == PM_RAM_START_ADDRESS) is False:
                    total_pm_size = total_pm_size + (section_hdrs[pm_sect].address - prev_end_addr)
                    logging.debug("Current section ({cur}) start addres is: {start}. Previous section end address is: {end}".format(cur=pm_sect, start=hex(section_hdrs[pm_sect].address), end=hex(prev_end_addr)))
                else:
                    # Only the first PM section after DM as PM gets here
                    # There might be the need to add some padding after DM as PM
                    # total_pm_size should be equal to self.dm_as_pm_size (transformed in octets)
                    if self.dm_as_pm_size * self.PC_INCR != total_pm_size:
                        # The long version:
                        # total_pm_size = total_pm_size + (self.dm_as_pm_size * self.PC_INCR - total_pm_size)
                        total_pm_size = self.dm_as_pm_size * self.PC_INCR
                        logging.debug("Current section ({cur}) start addres is: {start}. Previous section end address is: {end}".format(cur=pm_sect, start=hex(section_hdrs[pm_sect].address), end=hex(prev_end_addr)))
            total_pm_size = total_pm_size + section_hdrs[pm_sect].num_bytes

            if section_hdrs[pm_sect].address & GET_VIRTUAL_ADDR_BITS_MASK == DM_AS_PM_RAM_START_ADDRESS:
                pm_base = DM_AS_PM_RAM_START_ADDRESS | 0x80000000
                section_title = DOWNLOAD_ELF_SECTION_DM_AS_PM
            else:
                pm_base = PM_RAM_START_ADDRESS | 0x80000000
                section_title = DOWNLOAD_ELF_SECTION_PM
            offset = section_hdrs[pm_sect].address - pm_base
            # Save previous end address
            prev_end_addr = section_hdrs[pm_sect].address + section_hdrs[pm_sect].num_bytes
            # Top 2 bits from offset used to indicate it's a code ELF section
            records.append(((section_title << 30) | (offset & 0x3FFFFFFF), section_hdrs[pm_sect].num_bytes, section_hdrs[pm_sect].address))

        # End address must always be word aligned
        if self.chip.does_pm_octet_addressing():
            total_pm_size = int(math.ceil(float(total_pm_size) / 4.0))

        # Sanity check against static_pm given by kalaccess
        if total_pm_size != len_static_pm:
            raise Exception("Error. Mismatch between PM sizes given by kalaccess and kalelfreader_lib_wrappers")

        # Process DM1 & DM2 sections
        base_dm1 = self.getAddr(dnld, DM1_REGION_BASE)
        limit_dm1 = self.getAddr(dnld, DM1_REGION_LIMIT)
        base_dm2 = self.getAddr(dnld, DM2_REGION_BASE)
        limit_dm2 = self.getAddr(dnld, DM2_REGION_LIMIT)
        dm_section_keys = [x for x in section_hdrs if section_hdrs[x].type == ker.KerElfSectionHeaderInfo.ELF_SECTION_TYPE_DATA]
        dm_section_keys = sorted(dm_section_keys, key=lambda x: section_hdrs[x].address)
        for dm_sect in dm_section_keys:
            if (section_hdrs[dm_sect].address >= base_dm1) and (section_hdrs[dm_sect].address < limit_dm1):
                self.dm1_size = self.dm1_size + section_hdrs[dm_sect].num_bytes
                # Top 2 bits from offset are used to indicate it's a DM1 ELF section
                offset = section_hdrs[dm_sect].address - base_dm1
                records.append(((DOWNLOAD_ELF_SECTION_DM1 << 30) | (offset & 0x3FFFFFFF), section_hdrs[dm_sect].num_bytes, section_hdrs[dm_sect].address))
            elif (section_hdrs[dm_sect].address >= base_dm2) and (section_hdrs[dm_sect].address < limit_dm2):
                self.dm2_size = self.dm2_size + section_hdrs[dm_sect].num_bytes
                # Top 2 bits from offset are used to indicate it's a DM2 ELF section
                offset = section_hdrs[dm_sect].address - base_dm2
                records.append(((DOWNLOAD_ELF_SECTION_DM2 << 30) | (offset & 0x3FFFFFFF), section_hdrs[dm_sect].num_bytes, section_hdrs[dm_sect].address))
        return records

    def adjustDMSizes(self, variables, records):
        total_dm1_var_size = 0
        total_dm2_var_size = 0
        dm1_vars = [x for x in variables if x.dm_pref == 1]
        dm2_vars = [x for x in variables if x.dm_pref == 2]
        for v in dm1_vars:
            total_dm1_var_size = total_dm1_var_size + v.var.size
        for v in dm2_vars:
            total_dm2_var_size = total_dm2_var_size + v.var.size

        records_dm1 = [r for r in records if ((r[0] & 0xC0000000) >> 30) == 1]
        records_dm2 = [r for r in records if ((r[0] & 0xC0000000) >> 30) == 2]

        # Each record for a particular DM section provided by the ELF may need as a maximum an extra 3 bytes to keep word alignment
        # across sections. Calculate that theoretical maximum padding
        records_dm1_max_padding = 3 * len(records_dm1)
        records_dm2_max_padding = 3 * len(records_dm2)
        max_size_dm1 = self.dm1_size + records_dm1_max_padding
        max_size_dm2 = self.dm2_size + records_dm2_max_padding

        logging.debug("DM1 Size reported by ELF sections: {}".format(self.dm1_size))
        logging.debug("DM2 Size reported by ELF sections: {}".format(self.dm2_size))
        logging.debug("Max DM1 size reported by ELF sections: {}".format(max_size_dm1))
        logging.debug("Min DM2 size reported by ELF sections: {}".format(max_size_dm2))
        logging.debug("DM1 Size reported by variables: {}".format(total_dm1_var_size))
        logging.debug("DM2 Size reported by variables: {}".format(total_dm2_var_size))

        if total_dm1_var_size > self.dm1_size:
            if total_dm1_var_size <= max_size_dm1:
                logging.info("DM1 Size adjusted from {s1} to {s2}".format(s1=self.dm1_size,s2=total_dm1_var_size))
                self.dm1_size = total_dm1_var_size
            else:
                raise Exception("Error. DM1 Maximum size allowed from ELF sections would be {s1} but variables need {s2}".format(s1=max_size_dm1,s2=total_dm1_var_size))

        if total_dm2_var_size > self.dm2_size:
            if total_dm2_var_size <= max_size_dm2:
                logging.info("DM2 Size adjusted from {s1} to {s2}".format(s1=self.dm2_size,s2=total_dm2_var_size))
                self.dm2_size = total_dm2_var_size
            else:
                raise Exception("Error. DM2 Maximum size allowed from ELF sections would be {s1} but variables need {s2}".format(s1=max_size_dm2,s2=total_dm2_var_size))

    def generateKDC(self, kdc_idx, rom):
        import getopt

        sys.path.append(get_python_tools.python_tools_version().get_python_tools_path(self.os_type))

        try:
            import kalaccess
            import kalelfreader_lib_wrappers
        except ImportError:
            from kal_python_tools import kalaccess
            from kal_python_tools import kalelfreader_lib_wrappers

        kal = kalaccess.Kalaccess()
        ker = kalelfreader_lib_wrappers.Ker()

        supports_dm_as_pm = self.chip.supports_dm_as_pm()
        
        if self.dnldElfFile is None:
            raise Exception("Download elf file not provided")
        if self.romElfFile is None:
            raise Exception("ROM elf file not provided")
        if self.objDumpPath is None:
            raise Exception("kobjdump path not provided")
        if self.chip is None:
            raise Exception("chip not provided")

        if self.chip.does_pm_octet_addressing():
            self.PC_INCR = 4
        else:
            self.PC_INCR = 1

        # get the elf files
        dnld = kalaccess.Kalaccess()
        dnld.sym.load(self.dnldElfFile)
        ker.open_file(self.dnldElfFile)

        # extract the O files if we haven't already been given them
        if len(self.oFiles) == 0:
            raise Exception("No object files provided, nothing to do.")

        # Prebuilt library reloc files are expected to be found at "kymera/lib_release/<config>".
        # Private library reloc files are expected to be found at
        # "kymera/tools/KCSMaker/out/<config>/capabilities/<private_library_name>/debugbin/".
        mappings = {}
        mappings_to_export = {}
        func_import = {}
        extraVarSizes = {}
        for lib_name in self.oFiles:
            reloc_file = "{}.reloc".format(os.path.splitext(lib_name)[0])
            if os.path.isfile(reloc_file):
                # Import any exported relocation data if we are dealing with a prebuilt library
                if os.path.dirname(reloc_file).find("lib_release") != -1:
                    with open(reloc_file,"rb") as input:
                        func_import[lib_name] = pickle.load(input)
                        reloc_map = pickle.load(input)
                        reloc_extravarsizes = pickle.load(input)
                        # Sanity check. Make sure we don't have duplicated symbols with different values across libraries
                        for k in reloc_map.keys():
                            if k in mappings:
                                if mappings[k] != reloc_map[k]:
                                    raise Exception("Error. Mappings symbol {s} in {f} duplicated with different values. Try rebuilding to generate new scrambled symbols".format(s=k,f=reloc_file))
                        for k in reloc_extravarsizes.keys():
                            if k in extraVarSizes:
                                if extraVarSizes[k] != reloc_extravarsizes[k]:
                                    raise Exception("Error. extraVarSizes symbol {s} in {f} duplicated with different values. Try rebuilding to generate new scrambled symbols".format(s=k,f=reloc_file))
                        mappings.update(reloc_map)
                        extraVarSizes.update(reloc_extravarsizes)

        # parse the O files
        parseObjFile.getMappings(self.oFiles, self.objDumpPath, mappings, mappings_to_export)
        logging.debug("mappings:" + str(mappings))
        logging.debug("mappings_to_export:" + str(mappings_to_export))

        # dmprogLinks is empty if the chip does not support running code from DM and
        # the flag DM_AS_PM is True
        if supports_dm_as_pm == True:
            if self.all_dm_as_pm == False:
                self.add_dm_as_pm_sections = True
            else:
                self.add_dm_as_pm_sections = False
        else:
            self.add_dm_as_pm_sections = False
        
        (progLinks, varLinks, const, func, func_export, varSizes, dmprogLinks) = parseObjFile.getVariables( self.oFiles, self.objDumpPath, mappings, func_import, self, self.add_dm_as_pm_sections)

        # Populate a dictionary of candidate variables to export
        # Initially take all variables found in mappings (encrypted)
        # Use the unencrypted name
        if extraVarSizes == {}:
            for name in mappings:
                if name in varSizes and name not in extraVarSizes:
                    logging.debug("Adding {} to extraVarSizes".format(name))
                    extraVarSizes[mappings[name]] = varSizes[name]

        # From here onwards we don't need the full unscrambled name. Only the prefixes ("$", "L_", etc.)
        mappings = mappings_to_export

        logging.debug("varSizes:" + str(varSizes))
        logging.debug("extraVarSizes:" + str(extraVarSizes))
        logging.debug("func:" + str(func))
        logging.debug("func_export:" + str(func_export))
        for lib_name in func_export:
            with open("{}.reloc".format(os.path.splitext(lib_name)[0]),"wb") as output:
                pickle.dump(func_export[lib_name], output, -1)
                pickle.dump(mappings, output, -1)
                input = open("{}.symbols".format(lib_name),"rb")
                publicSymbols = [x.strip() for x in input.readlines()]
                input.close()
                # Only export variable name and size information for those variables that are
                # part of the public symbols list
                extraVarSizes = {k: v for k, v in extraVarSizes.items() if k in publicSymbols}
                logging.debug("extraVarSizesToExport" + str(extraVarSizes))
                pickle.dump(extraVarSizes, output, -1)

        # Combine varSizes and extraVarSizes. extraVarSizes contain variable size information
        # using the unencrypted name. Some variables that have gone through kalscramble but are present
        # in the lib.symbols file might have lost the size information in the ELF so add them to
        # the existing varSizes dictionary for further use
        for k in extraVarSizes:
            varSizes[k] = extraVarSizes[k]
        # now add the constant symbols to the variables as we don't really have
        # constants in a download
        varLinks.update(const)

        # get the module list, combine information from functions to record whether it's MiniM or MaxiM code
        (modLut, dm_as_pm_funcs) = self.buildModuleLut( func, ker, mappings )
        logging.debug("modLut0:" + str(modLut))
        # modLut is a dictionary
        # modLut:{'func_name': [start_address, INSTR_SET, [32_bit_opcode_1, ...]], ...}
        # INSTR_SET is a string. Either "MAXIM" or "MINIM"
        # [32_bit_opcode_1, ...] are the opcodes (32 bit per position in array) of the function func_name

        if self.add_dm_as_pm_sections == True:
            # do the same for DM as PM
            logging.debug("dmprogLinks before garbage collection:" + str(dmprogLinks))
            # the list of symbols we get from the object files may include garbage
            # collected symbols, so strip out anything not in the elf file
            garbage = [x for x in dmprogLinks if x not in modLut]
            for sym in garbage:
                logging.debug("removed: " + str(sym))
                dmprogLinks.pop(sym)
    
            logging.debug("dmprogLinks before fixup:" + str(dmprogLinks))
            self.fixupCodeOffsets(dmprogLinks, modLut, dnld)
            logging.debug("dmprogLinks after fixup:" + str(dmprogLinks))
    
            self.dm_as_pm_size = self.getDMasPMsize(dnld)
            logging.debug("dm_as_pm_size_words: " + str(self.dm_as_pm_size))
            
        logging.debug("progLinks before garbage collection:" + str(progLinks))
        # the list of symbols we get from the object files may include garbage
        # collected symbols, so strip out anything not in the elf file
        garbage = [x for x in progLinks if x not in modLut]
        for sym in garbage:
            logging.debug("removed: " + str(sym))
            progLinks.pop(sym)

        logging.debug("progLinks before fixup:" + str(progLinks))
        self.fixupCodeOffsets(progLinks, modLut, dnld, True)
        logging.debug("progLinks after fixup:" + str(progLinks))

        logging.debug("varLinks before adjusting:" + str(varLinks))
        # get the symbols
        # dmprogLinks is an empty dictionary if dm_as_pm sections should not be added
        varLut  = self.buildVariableLut(dnld, rom, progLinks, modLut, varLinks, varSizes, extraVarSizes, mappings, dmprogLinks)
        # varLut is a dictionary
        # varLut:{'var_name': Symb, ...}
        # See definition of Symb class above
        progLut = self.buildProgrammeLut(dnld)
        # progLut is a dictionary
        # progLut:{'func_name': start_address, ...}

        garbage = [x for x in varLinks if x not in varLut]
        for sym in garbage:
            varLinks.pop(sym)

        # we need to handle addresses/references that have moved because of relaxing
        logging.debug("modLut:" + str(modLut))
        logging.debug("varLut:" + str(varLut))
        logging.debug("varLinks after filtering:" + str(varLinks))

        logging.debug("progLut:" + str(progLut))


        variables = self.processVariableLinks(varLinks, varLut, progLut, dnld, rom, modLut, kdc_idx)
        # variables is an array of Variables
        # see Variable class definition above
        # each variable contains a Symbol (see Class defined above) a dm preference field, an array of static initialisation
        # data and an array of patches (relocation information)

        # Split variables in three arrays (DM, CONST16 and CONST)
        DMVars = [x for x in variables if x.var.type == "NOT_CONST" or x.var.type == "DMCONST16" or x.var.type == "DMCONST24"]
        logging.debug("DMVars:" + str(DMVars))
        Const16 = [x for x in variables if x.var.type == "CONST16"]
        logging.debug("Const16:" + str(Const16))
        Const = [x for x in variables if x.var.type == "CONST"]
        logging.debug("Const:" + str(Const))


        (static_pm, program) = self.processProgrammeLinks(progLinks, varLut, progLut, modLut, dnld, rom)
        # static_pm is an array of 32-bit opcodes that will be written in KDC
        # program is an array of tuples
        # each tuple is a patch
        # a patch is made of :
        # - Target symbol (0: PM, otherwise variable ID)
        # - Destination offset
        # - Target offset

        total_len = len(static_pm)
        if self.add_dm_as_pm_sections:
            (dmstatic_pm, dmprogram) = self.processProgrammeLinks(dmprogLinks, varLut, progLut, modLut, dnld, rom, False)
            logging.debug("dmprogram: " + str(dmprogram))
            # dmstatic_pm is a dictionary, not a list as static_pm
            logging.debug("dmstatic_pm: " + str(dmstatic_pm))
            
            dm_as_pm_size_static = len(dmstatic_pm)
            if dm_as_pm_size_static != self.dm_as_pm_size:
                raise Exception("Error. Calculated DM as PM size does not match the linker value")
            total_len += self.dm_as_pm_size

        modLutDMasPM = {}
        for func in dm_as_pm_funcs:
            modLutDMasPM[func] = modLut[func]
            del modLut[func]
        logging.debug("modLutDMasPM: " + str(modLutDMasPM))
        # We have to tell cap_download_mgr what instruction set we are programming, so that it knows
        # when to patch relative branch immediates in MiniM
        instrSetMaxim = self.getMaximAddresses(modLut, dnld)
        dm_as_pm_instrSetMaxim = self.getMaximAddresses(modLutDMasPM, dnld)

        logging.debug("instrSetMaxim: " + str(instrSetMaxim))

        logging.debug("program: " + str(program))
        
        logging.debug("LOOK:" + str(variables))
        kdc = KDCFile(varLut)

        for cap in self.cap_list:
            logging.debug(cap.getSymCapDataName())
            for v in variables:
                logging.debug("->" + str(v.var.name))
                # Ignore $_ when doing this comparison
                if cap.getSymCapDataName() == v.var.name.strip("$_"):
                    if cap.getSymCapData() == 0:
                        cap.setSymCapData(v.var.id);
                    else:
                        raise Exception("Error. cap_data symbol repeated")
            kdc.addCapability(cap)

        ELFRecords = self.parseELF(ker, dnld, total_len)
        self.pm_size = len(static_pm)
        
        logging.debug("static_pm: " + str(static_pm))

        self.adjustDMSizes(variables, ELFRecords)

        self.addInfoSection(kdc, instrSetMaxim, ELFRecords, len(variables), dm_as_pm_instrSetMaxim)

        self.addPmSection(kdc, static_pm, TAG_PM)
        if self.add_dm_as_pm_sections and self.dm_as_pm_size != 0:
            self.addPmSection(kdc, dmstatic_pm, TAG_DM_AS_PM)

        self.addVarSections(kdc, variables)

        if self.chip.get_default_kcs_type() == KCSFile.STREAM_TYPE:
            # streamed KCS need to place constants before relocation sections
            self.addCONST16(kdc, Const16)
            self.addCONST(kdc, Const)
            self.addPmReloc(kdc, program, varLut, RELOC_PM)
            if self.add_dm_as_pm_sections == True:
                self.addPmReloc(kdc, dmprogram, varLut, RELOC_DM_AS_PM)
            self.addDmReloc(kdc, variables)
            self.addEndSection(kdc)
        else:
            self.addPmReloc(kdc, program, varLut, RELOC_PM)
            if self.add_dm_as_pm_sections == True:
                self.addPmReloc(kdc, dmprogram, varLut, RELOC_DM_AS_PM)
            self.addDmReloc(kdc, variables)
            # add the end section
            self.addEndSection(kdc)
            self.ensurePadding32(kdc)
            # After the end section of KDC, write constant data
            self.addCONST16(kdc, Const16)
            self.ensurePadding32(kdc)
            self.addCONST(kdc, Const)
            self.ensurePadding32(kdc)
        return kdc

