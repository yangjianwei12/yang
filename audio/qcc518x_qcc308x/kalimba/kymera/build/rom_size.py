#!/usr/bin/env python
############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2020 Qualcomm Technologies International, Ltd.
#
############################################################################
"""
Prints ROM size
"""
import sys
import os
import collections
import json
from io import StringIO 

class Capturing(list):
    def __enter__(self):
        self._stdout = sys.stdout
        sys.stdout = self._stringio = StringIO()
        return self
    def __exit__(self, *args):
        self.extend(self._stringio.getvalue().splitlines())
        del self._stringio    # free up some memory
        sys.stdout = self._stdout

def print_help():
    """
    Print script usage.
    """
    print('''
    rom_size.py - Gets elf file and presents total rom in Mbits, plus comparison with last build
               
    takes as command line arguments:
        (1) input location where to store size information
        (1) input elf files
    ''')

if __name__ == '__main__':

    # Sanity checks
    if len(sys.argv) != 3:
        print_help()
        sys.exit(1)
    size_file = os.path.join(sys.argv[1], 'sizes.json')
    elf_file = sys.argv[2]
    sys.path.append(os.path.join(sys.path[0], '..', 'tools', 'kalromuse'))

    
    import KalRomUse
    # Prepare kalromuse arguments and execute it.
    kalromuse_args = collections.OrderedDict()
    kalromuse_args = {'elf': elf_file, 'summary_json': size_file}
    KalRomUse.run(**kalromuse_args)
