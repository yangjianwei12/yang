#!/usr/bin/env python
# Copyright (c) 2022 Qualcomm Technologies International, Ltd.

# Python 2 and 3
from __future__ import print_function
import os
import logging
import json

import capsign

from menus.wizard import logger
from menus.audio_capabilities_directories import AudioCapabilityDirectories

from os.path import exists
     
class HeadlessCapabilitySign():
    def __init__(self, args):
    	self.log = logging.getLogger('wizard.{}'.format(type(self).__name__))
        self.toolkit = args.kit
        self.config_filename = args.config_filename
        self.workspace = args.workspace
          
    def read_config_file(self):
        
        try:
            with open(self.config_filename) as json_file:
                data = json.load(json_file)
        except Exception:
            self.log.error("Cannot open or read from config file!")
            raise
        
        return data.get('capability_project') 
        
    def run(self):
        capabilities = self.read_config_file()
        audio_locations = AudioCapabilityDirectories()
              
        if capabilities:
            self.log.info("File read - Capabilities probject is {}".format(capabilities))

            audio_dir = audio_locations.get_audio_dir_from_workspace(self.workspace)
            kymera_dir = audio_locations.get_kymera_dir_from_audio_dir(audio_dir)
            
            output_bundles_dir = audio_locations.get_output_bundles_dir_from_kymera_dir(kymera_dir)
            selected_bundle = os.path.abspath(os.path.join(output_bundles_dir, os.path.splitext(capabilities)[0]) )

            if exists(selected_bundle):
                self.log.info("audio_dir is {}".format(audio_dir))
                self.log.info("output_bundles_dir is {}".format(output_bundles_dir))
                self.log.info("selected_bundle is {}".format(selected_bundle))

                prebuilt_dir = audio_locations.get_prebuilt_dir_from_audio_dir(audio_dir)
                self.log.info("prebuilt_dir is {}".format(prebuilt_dir))
                
                result = capsign.sign(selected_bundle, prebuilt_dir, self.toolkit)
                if result.is_error:
                    self.log.error("Error - {}".format(result.err))
                else:
                    self.log.info("Capability sign complete!")

            else:
                self.log.error("Path to {} doesn't exist - did you run the VA Wizard?".format(self.selected_bundle))

        else:
            self.log.error("Capabilities have not been defined")