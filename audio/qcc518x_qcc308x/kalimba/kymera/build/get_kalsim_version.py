############################################################################
# Copyright (c) 2013 - 2020 Qualcomm Technologies International, Ltd.
############################################################################
"""
Prints the path to the relevant Kalsim binary based on the operating system type
and requested target chip path.
"""

import sys
import os

SUPPORTED_CHIPS = ["crescendo_audio", "stre_audio", "streplus_audio", "maor_audio", "maor2_audio"]
SUPPORTED_PLATFORMS = ["win32", "linux", "linux2"]

if __name__ == '__main__':

    chipname = sys.argv[1]
    if chipname not in SUPPORTED_CHIPS:
        sys.stderr.write("Unknown chipname: {0}{1}".format(chipname, os.linesep))
        sys.exit(1)

    if sys.platform not in SUPPORTED_PLATFORMS:
        sys.stderr.write("Unknown platform: {0}{1}".format(sys.platform, os.linesep))
        sys.exit(1)

    if sys.platform == "win32":
        kalsim_path = "//root.pri/FILEROOT/UnixHomes/home/devtools/kalsim/22h/win32/kalsim_"+sys.argv[1]+".exe"
    else:
        if os.path.exists(r'/comm'):
            kalsim_path = "/home/devtools/kalsim/22h/posix/kalsim_"+sys.argv[1]
        else:
            kalsim_path = os.path.join(os.environ["_SCRIPT_DIR"], 'tools', 'kalsim', 'kalsim' + chipname + '_audio')

    print(kalsim_path)
