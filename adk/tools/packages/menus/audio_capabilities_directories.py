#!/usr/bin/env python
# Copyright (c) 2022 Qualcomm Technologies International, Ltd.

# Python 2 and 3
from __future__ import print_function

import os
import glob
import logging

from menus.addon_importer import AddonUtils
from menus.wizard import logger

class AudioCapabilityDirectories():

    def __init__(self):
      	self.log = logging.getLogger('wizard.{}'.format(type(self).__name__))

    def get_audio_dir_from_workspace(self,workspace):
        addon_utils = AddonUtils()
        app_project = os.path.splitext(workspace)[0] + '.x2p'
        try:
            self.chip_type = addon_utils.readAppProjectProperty("CHIP_TYPE", app_project)
        except Exception as e:        
            self.log.error("Error getting CHIP_TYPE {}").format(str(e))
            raise
        audio_dir = os.path.abspath(os.path.join(os.path.dirname(app_project), '..', '..', '..', 'audio', self.chip_type))

        return audio_dir
 
    def get_kymera_dir_from_audio_dir(self, audio_dir):
         return(os.path.join(audio_dir, 'kalimba', 'kymera'))

    def get_output_bundles_dir_from_kymera_dir(self,kymera_dir):
        try:
            output_bundles_dir = glob.glob(os.path.join(kymera_dir, 'output_bundles', '*_rom_release'))[0]
        except Exception as e:
            self.log.error("No output bundles found in {}. Please build a supported capability first".format(kymera_dir))
            raise
            
        return output_bundles_dir
     
    def get_prebuilt_dir_from_audio_dir(self, audio_dir):
        prebuilt_dir = glob.glob(os.path.join(audio_dir, 'kalimba_ROM_*', 'kymera', 'prebuilt_dkcs', '*_rom_release'))[0]
        return prebuilt_dir
        