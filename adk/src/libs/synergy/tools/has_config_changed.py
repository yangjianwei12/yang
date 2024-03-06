# Copyright (c) 2022 Qualcomm Technologies International, Ltd
# has_config_changed.py - Checks and maintains a reference to the last build configuration.

import os
import sys
CURRENT_CONFIG_FILE = '.tmp_current_config'
def SetCurrentConfig(app_type):
    with open(CURRENT_CONFIG_FILE, "w+") as f:
        f.truncate()
        f.write(app_type)
        f.close()

def HasConfigChanged(new_app_type):
    result = False
    if os.path.exists(CURRENT_CONFIG_FILE):
        with open(CURRENT_CONFIG_FILE, "r") as f:
            old_app_type = f.readline()
            if old_app_type != new_app_type:
                result = True
    else:
        with open(CURRENT_CONFIG_FILE, "w+") as f:
            f.write(new_app_type)
    return result

if __name__ == '__main__':
    app_type = sys.argv[1]
    if HasConfigChanged(app_type):
        SetCurrentConfig(app_type)
        print("True")
    else:
        print("False")
