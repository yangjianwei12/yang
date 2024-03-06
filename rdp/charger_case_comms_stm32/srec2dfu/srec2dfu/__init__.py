"""
Script to generate DFU bin file from srec based DFU file.

Copyright (c) Qualcomm Technologies International, Ltd.
"""

import sys
import os
from argparse import ArgumentParser
import itertools
import logging
from struct import pack
from typing import List
from pathlib import Path

# total size of S record should be less than 240
# available payload size for S3 records = 240 -3(dfu protocol header) -2 (header) -2 (size field) -8 (address) -2 (csum ) -1(line feed) = 222
# payload size in words = 111
MAX_PAYLOAD_SIZE = 108

RECORD_DELIMITER = "00"


def make_s3(address: int, payload: int, payload_count: int) -> str:
    """Create an SREC S3 record from provided payload.
    """

    # Pad out payload if it is not divisible by 4 bytes.
    while (payload_count % 4):
        payload += 'FF'
        payload_count += 1

    payload_count += 5
    rec = f'{payload_count:02X}{address:08X}{payload}'

    csum = 0
    rlen = len(rec)

    for n in range(0, rlen, 2):
        csum += int(rec[n:n+2], 16)

    csum = csum & 0xFF
    csum = csum ^ 0xFF

    rec = f'S3{rec}{csum:02X}'

    return rec


def parse_lines(all_lines_orig: List[str], size: int) -> List[str]:
    """Resize all SREC S3 records.
    """
    all_lines = []

    address = 0
    start_address = 0
    payload_count = 0
    payload = ''

    for line in all_lines_orig:
        line = line.rstrip()

        if line.startswith("S3"):

            line_address = int(line[4:12], 16)

            if address == 0:
                address = line_address
                start_address = address
            else:
                if address != line_address:
                    # This record does not follow on immediately from
                    # the previous one, so we cannot add its payload
                    # to the data we have stored. We must first output
                    # the stored data.
                    if payload_count:
                        all_lines.append(make_s3(start_address, payload, payload_count))
                        payload_count = 0
                        payload = ''

                    address = line_address
                    start_address = address

            line_len = len(line)

            for n in range(12, line_len - 2, 2):
                payload += line[n:n+2]
                payload_count += 1
                address += 1
                if payload_count == size:
                    # We have accumulated enough payload data for one
                    # S3 record, so output it.
                    all_lines.append(make_s3(start_address, payload, payload_count))
                    payload_count = 0
                    payload = ''
                    start_address = address

        else:
            # Not an S3 record, but we may have some stored S3 payload
            # data, which we will need to output before the current
            # record.
            if payload_count:
                all_lines.append(make_s3(start_address, payload, payload_count))
                payload_count = 0
                payload = ''
                address = 0

            all_lines.append(line)

    return all_lines
    
    
def convert_file(input_file: str, output_file: str, major: int, minor: int) -> int:
    """Do the real work of converting a DFU SREC file to a DFU bin file.
    """
    input_file = Path(input_file)
    if input_file.suffix != '.srec':
        logging.error("Input file is not an srec file: '%s'", input_file)
        return 1

    if not output_file:
        output_file = input_file.with_suffix('.bin')
    else:
        output_file = Path(output_file)

    if not input_file.is_file():
        logging.error("Cannot read input file '%s'", input_file)
        return 1

    # read in all the srec lines
    with input_file.open() as srec_f:
        input_lines = [ line for line in srec_f ]

    all_lines = parse_lines(input_lines, MAX_PAYLOAD_SIZE)
    total_lines = len(all_lines)

    # extra 1 byte is added in the size of each record for RECORD_DELIMITER char
    s0_size = len(all_lines[0].rstrip()) + 1
    s7_size = len(all_lines[total_lines-1].rstrip()) + 1
    s3_count = int((total_lines - 2) / 2)

    bank_a_size = 0
    bank_b_size = 0

    for line in itertools.islice(all_lines , 1, s3_count + 1):
        bank_a_size = bank_a_size + len(line.rstrip()) + 1

    for line in itertools.islice(all_lines , s3_count + 1, total_lines-1):
        bank_b_size = bank_b_size + len(line.rstrip()) + 1

    # extra 4 bytes include the length of the offset field
    total_size = 2*(s0_size+s7_size) + bank_a_size + bank_b_size + 4
    bank_b_offset = s0_size + bank_a_size + s7_size

    # Create the binary parts
    header = pack('>8sL8shhLL',
                  b"EXTUHDR1", # 8 bytes Id
                  20,          # 4 bytes length
                  b"STM32F0x", # 8 bytes device variant
                  major,  # 2 bytes major version
                  minor,  # 2 bytes minor version
                  1,           # 4 bytes # compatible versions
                  0xffffffff   # 4 bytes compatible versions
    )
    partition_header = pack('>8sLL',
                            b'SRECDATA',   # 8 bytes srec partition ID
                            total_size,    # 4 bytes length
                            bank_b_offset  # 4 bytes bank b offset
    )
    footer = pack('>8sL',
                  b'APPUPFTR',  # 8 bytes srec partition ID 
                  0             # 4 bytes length
    )

    logging.info("Writing to file:", str(output_file))

    with output_file.open('wb') as dfu_f:
        # main header
        dfu_f.write(header)

        # partition header
        dfu_f.write(partition_header)

        # partition data
        dfu_f.write(bytes(all_lines[0].rstrip(), 'ascii'))

        # line-feed char
        dfu_f.write(bytes.fromhex(RECORD_DELIMITER))

        for line in itertools.islice(all_lines , 1, s3_count + 1):
            dfu_f.write(bytes(line.rstrip(), 'ascii'))
            dfu_f.write(bytes.fromhex(RECORD_DELIMITER))

        dfu_f.write(bytes(all_lines[total_lines-1].rstrip(), 'ascii'))
        dfu_f.write(bytes.fromhex(RECORD_DELIMITER))

        dfu_f.write(bytes(all_lines[0].rstrip(), 'ascii'))
        dfu_f.write(bytes.fromhex(RECORD_DELIMITER))

        for line in itertools.islice(all_lines , s3_count + 1, total_lines-1):
            dfu_f.write(bytes(line.rstrip(), 'ascii'))
            dfu_f.write(bytes.fromhex(RECORD_DELIMITER))

        dfu_f.write(bytes(all_lines[total_lines-1].rstrip(), 'ascii'))
        dfu_f.write(bytes.fromhex(RECORD_DELIMITER))

        # footer
        dfu_f.write(footer)

    return 0


def main(argv=None) -> int:
    """Tool entry point.
    """
    parser = ArgumentParser(description='Convert srec file to custom upgade file format for dfu file')
    parser.add_argument("file", help = "Path to srec file")
    parser.add_argument("-M", '--major', default=1, type=int,
                        help="Set the major version of the software")
    parser.add_argument("-m", '--minor', default=1, type=int,
                        help="Set the minor version of the software")
    parser.add_argument("-o", '--out', default='', type=str,
                        help="Path to output file, defaults to same folder as srec file")
    args = parser.parse_args(argv)

    try:
        return convert_file(args.file, args.out, args.major, args.minor)
    except (IOError, FileNotFoundError) as e:
        logging.error("Error in converting file: %s", str(e))
        return 1


if __name__ == "__main__":
    sys.exit(main())
