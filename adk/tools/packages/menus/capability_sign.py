#!/usr/bin/env python
# Copyright (c) 2021 Qualcomm Technologies International, Ltd.

# Python 2 and 3
from __future__ import print_function
import argparse
from menus.wizard.exceptions import WizardError
import os
import glob

import capsign

from menus.addon_importer import AddonUtils
from menus.headless_capability_sign import HeadlessCapabilitySign
from menus.audio_capabilities_directories import AudioCapabilityDirectories

from menus.wizard import (
    logger,
    Wizard,
    Step,
    gui
)

class Select(Step):
    def show(self):
        super(Select, self).show(title="Select dkcs capability")
        self.audio_locations = AudioCapabilityDirectories()

        self.audio_dir = self.audio_locations.get_audio_dir_from_workspace(self.cli_args.workspace)
        kymera_dir = self.audio_locations.get_kymera_dir_from_audio_dir(self.audio_dir)

        try:
            self.output_bundles_dir = self.audio_locations.get_output_bundles_dir_from_kymera_dir(kymera_dir)
            output_bundles = os.path.join(self.output_bundles_dir, '*')
            bundles = glob.glob(output_bundles)

        except Exception as e:
            msg = "No output bundles found in {}. Please build a supported capability first".format(kymera_dir)
            gui.tkMessageBox.showerror(title="No output bundles found!", message=msg)
            raise

        self.selected_bundle = gui.StringVar(self)
        for bundle in bundles:
            cap_name = os.path.basename(bundle)
            radio = gui.Radiobutton(self, text=cap_name, variable=self.selected_bundle, value=bundle)
            radio.pack(anchor=gui.W, expand=True)

            if not self.selected_bundle.get():
                radio.select()  # Select the first in the group

    def next(self):
        prebuilt_dir = self.audio_locations.get_prebuilt_dir_from_audio_dir(self.audio_dir)
        result = capsign.sign(self.selected_bundle.get(), prebuilt_dir, self.cli_args.kit)
        if result.is_error:
            gui.tkMessageBox.showerror(title="Error signing dkcs file", message=result.err)
            raise WizardError()


class Finished(Step):
    def show(self):
        super(Finished, self).show(title="Done!")

def run(args):
    logger.log_to_file(os.path.join(os.path.dirname(args.workspace), 'capability_sign.log'))

    if args.config_filename:
        wizard = HeadlessCapabilitySign(args)
    else:
        wizard = Wizard(args, "Prebuilt capability sign", [Select, Finished])
    
    wizard.run()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-k", "--kit", required=True)
    parser.add_argument("-w", "--workspace", required=True)
    parser.add_argument("-c", "--config_filename")

    args, _ = parser.parse_known_args()
    run(args)


if __name__ == "__main__":
    main()
