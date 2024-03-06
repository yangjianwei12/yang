############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Check that 2 ELF files are identical by ensuring that their linker script
checksum variables are located at the same place and have the same value.
"""

import sys
import kalaccess

CHECKSUM_SYM_NAME = "$__devtools_image_checksum"

if __name__ == '__main__':

    if len(sys.argv) < 3:
        sys.stderr.write(sys.argv[0] + ": Error, Please provide two ELF files to compare\n")
        sys.exit(1)
    elf1 = sys.argv[1]
    elf2 = sys.argv[2]
    kal1 = kalaccess.Kalaccess()
    kal2 = kalaccess.Kalaccess()
    kal1.sym.load(elf1)
    kal2.sym.load(elf2)
    varfind1 = kal1.sym.varfind(CHECKSUM_SYM_NAME)
    varfind2 = kal2.sym.varfind(CHECKSUM_SYM_NAME)
    if len(varfind1) != 1 or len(varfind2) != 1:
        raise Exception("Symbol " + CHECKSUM_SYM_NAME + " NOT found")
    else:
        addr1 = varfind1[0][2]
        addr2 = varfind2[0][2]
        if addr1 != addr2:
            raise Exception("Checksum found at different addresses")
        elif kal1.sym.static_dm[addr1] == kal2.sym.static_dm[addr2]:
            print("ELFs checksums are equal")
            exit(0)
        else:
            raise Exception("Checksum is different")
