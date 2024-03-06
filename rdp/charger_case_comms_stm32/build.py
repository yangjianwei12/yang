#
# \copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.\n
#            All Rights Reserved.\n
#            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#\file
#\brief      Charger Case build script
#

import re
import os
import zlib
import argparse
from shutil import copyfile
from shutil import rmtree
from datetime import datetime

global cwd
cwd = os.getcwd()
make_dir =  os.path.join(cwd, "make");
build_dir = os.path.join(make_dir, "build");

# All the build variants.
# field 0: name
# field 1: bootloader name
# field 2: long name(s)
all_variants = {
    'ST2': {'long_name': ['qcc5171_rdp_1.0.1_case']},
    'MON': {'long_name': ['monaco_rdp_1.0.1_case']}
}

all_banks = {
    'a'   : {'memory_map': 'mem_bank_a.ld'},
    'b'   : {'memory_map': 'mem_bank_b.ld'},
    'boot': {'memory_map': 'mem_boot.ld'}
}

def fail():

    print('Build unsuccessful')
    exit(1)


def clean():
    global cwd

    os.chdir(make_dir)
    if os.system("make clean"):
        fail()


def build_variant(variant, bank, c_defines):
    global cwd

    os.chdir(make_dir)

    make_string = 'make VARIANT={} BANK={} C_DEFS="{}" all'.format(variant, bank, c_defines)

    print(make_string);
    if os.system(make_string):
        fail()

    os.chdir(cwd)


def byte_swap_32(x):
    return ((x & 0x000000FF) << 24) + \
        ((x & 0x0000FF00) << 8) + \
        ((x & 0x00FF0000) >> 8) + \
        ((x & 0xFF000000) >> 24)


def s0(csum_a, csum_b, variant):

    # Address.
    r = '0000'

    # Payload.
    r += '{0:0{1}X}'.format(byte_swap_32(csum_a), 8)
    r += '{0:0{1}X}'.format(byte_swap_32(csum_b), 8)

    vlen = len(variant)
    for n in range(7):
        if n < vlen:
            r += variant[n].encode('utf-8').hex()
        else:
            r += '00'
    r += '00'

    # Length.
    rlen = len(r)
    r = '{0:0{1}X}'.format(int(rlen / 2) + 1, 2) +  r

    # Checksum.
    csum = 0
    for n in range(0, rlen + 2, 2):
        csum += int(r[n:n+2], 16)
    csum = ((csum ^ 0xFF) & 0xFF)
    r += '{0:0{1}X}'.format(csum, 2)

    return 'S0' + r


def bit_swap(data):
    result = 0;

    for n in range(32):
        result = result << 1

        if (data & 0x1):
            result = result | 0x1

        data = data >> 1;

    return result & 0xFFFFFFFF


def calculate_crc32(all_records):
    crc32 = 0

    for record in all_records:
        if record.startswith("S3"):
            payload = record.rstrip()[12:-2]
            plen = len(payload)

            # Pad out payload if it is not divisible by 4 bytes.
            while plen % 8:
                payload += 'FF'
                plen += 2

            for n in range(0, plen, 8):
                crc32 = zlib.crc32(bit_swap(int(payload[n:n+8], 16)).to_bytes(4, 'big'), crc32)

    crc32 = bit_swap(crc32) ^ 0xFFFFFFFF

    return crc32


def build_dfu(variant):
    global cwd

    fout = open(os.path.join(cwd, variant) + "_dfu.srec", "w")

    fa = open(os.path.join(build_dir, variant , "A", "charger-comms-stm32f0.srec"))
    alla = fa.readlines()

    fb = open(os.path.join(build_dir, variant , "B", "charger-comms-stm32f0.srec"))
    allb = fb.readlines()

    # Work out CRC32 for Image A.
    crc32a = calculate_crc32(alla)

    # Work out CRC32 for Image B.
    crc32b = calculate_crc32(allb)

    # Write the S0 record.
    fout.write(s0(crc32a, crc32b, variant) + "\n")

    # Write the S3 records for Image A.
    for record in alla:
        if record.startswith("S3"):
            fout.write(record)

    # Write the S3 records for Image B.
    for record in allb:
        if record.startswith("S3"):
            fout.write(record)

    # Write the S7 record.
    fout.write("S70500000000FA\n")

    fa.close()
    fb.close()
    fout.close()


def build_image(variant):
    global cwd

    fout = open(os.path.join(cwd, variant) + ".srec", "w")

    # Write a minimal S0 record.
    fout.write("S0050000434275\n")

    # Write all the S3 records from the bootloader.
    fboot = open(os.path.join(build_dir, "BOOT", "charger-comms-stm32f0.srec"))
    allboot = fboot.readlines()
    for record in allboot:
        if record.startswith("S3"):
            fout.write(record)

    # Write all the S3 records from Image A.
    fa = open(os.path.join(build_dir, variant , "A", "charger-comms-stm32f0.srec"))
    alla = fa.readlines()
    for record in alla:
        if record.startswith("S3"):
            fout.write(record)

    # Write the S7 record.
    fout.write("S70500000000FA\n")

    fa.close()
    fboot.close()
    fout.close()

def generate_version_headers(version_number, version_string):
    if not os.path.exists(os.path.join(cwd, "include", "override")):
        os.makedirs(os.path.join(cwd, "include", "override"))
    fts = open(os.path.join(cwd, "include", "override", "timestamp.h"), "w")
    now = datetime.now()
    fts.write("#define DATE_STAMP \"" + now.strftime("%b %d %Y") + "\"\n")
    fts.write("#define TIME_STAMP \"" + now.strftime("%H:%M:%S") + "\"\n")
    fts.close()

    fver = open(os.path.join(cwd, "include", "override", "version.h"), "w")
    fver.write("#define SW_VERSION_NUMBER " + str(version_number) + "\n")
    fver.write("#define SW_VERSION_STRING \"" + version_string + "\"\n")
    fver.close()


def generate_defines_from_mem_map_ld(bank, mem_map_lines):
    # Strip out whitespace, comments - only leave memory map link script lines
    lines = [l.replace(" ", "") for l in mem_map_lines]
    mem_maps = [re.search(r'(.+)\(.+\):ORIGIN=(.+),LENGTH=(.+)',l) for l in lines]

    header_lines = []
    for map in mem_maps:
        if map and map.group(1) and map.group(2) and map.group(3):
            section_name = "{}_{}".format(map.group(1), bank.upper())
            origin = map.group(2)
            length = map.group(3).replace("K","*1024").replace("M","*1024*1024")

            # Generate macros for origin and length of each section
            header_lines.append("#define _{}_ORIGIN ({})\n".format(section_name, origin))
            header_lines.append("#define _{}_LENGTH ({})\n".format(section_name, length))

    return header_lines


def generate_section_headers():
    if not os.path.exists(os.path.join(cwd, "include", "override")):
        os.makedirs(os.path.join(cwd, "include", "override"))

    overall_mem = open(os.path.join(cwd, "include", "override", "mem.h"), "w")

    for bank in all_banks.keys():
        memory_map_filename = all_banks[bank]['memory_map']
        header_filename = "mem_{}.h".format(bank)

        # Include this bank header in the main memory header.
        overall_mem.write("#include \"{}\"\n".format(header_filename))

        # Read all lines from the memory map link script
        mem_map_file = open(os.path.join(make_dir, memory_map_filename))
        mem_map_lines = mem_map_file.readlines()

        # Open a header file and write defines based on the link script.
        mem_map_h = open(os.path.join(cwd, "include", "override", header_filename), "w")
        header_lines = generate_defines_from_mem_map_ld(bank, mem_map_lines);
        mem_map_h.writelines(header_lines);
        mem_map_h.close()

    overall_mem.close()

def main(argv = []):


    parser = argparse.ArgumentParser(description='Charger Case Build')
    parser.add_argument("-a", default=False, action="store_true", help = "Build image A only")
    parser.add_argument("-b", default=False, action="store_true", help = "Build image B only")
    parser.add_argument("-o", default=False, action="store_true", help = "Build boot image only")
    parser.add_argument("-s", default=0, type=int, help = "Software version number")
    parser.add_argument("-t", default='UNRELEASED', type=str, help = "Software version string")
    parser.add_argument("-v", default='all', type=str, help = "Variant")
    parser.add_argument("-d", default='', type=str, help = "C defines")
    parser.add_argument("-c", default=False, action="store_true", help = "Clean build directory before build")
    args = parser.parse_args()

    generate_section_headers()
    generate_version_headers(args.s, args.t);

    if (args.c):
        clean()

    for variant_name in all_variants.keys():
        variant = all_variants[variant_name]

        if args.v == 'all' or args.v == variant_name:
            if args.a:
                build_variant(variant_name, 'A', args.d)
            elif args.b:
                build_variant(variant_name, 'B', args.d)
            elif args.o:
                build_variant('', 'BOOT', args.d)
            else:
                build_variant('', 'BOOT', args.d)
                build_variant(variant_name, 'A', args.d)
                build_variant(variant_name, 'B', args.d)
                build_dfu(variant_name)
                build_image(variant_name)

                # Create long-named output files.
                for ln in variant['long_name']:
                    copyfile(os.path.join(cwd, variant_name + '.srec'),
                             os.path.join(cwd, ln + '.srec'))
                    copyfile(os.path.join(cwd, variant_name + '_dfu.srec'),
                             os.path.join(cwd, ln + '_dfu.srec'))

    print('')
    exit(0)

if __name__ == "__main__":
    main()
