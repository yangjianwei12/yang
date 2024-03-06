############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2023 Qualcomm Technologies International, Ltd.
#   %%version
#
############################################################################

from csr.dev.fw.firmware_component import FirmwareComponent
from csr.dev.model.base_component import Reportable
from csr.dev.model import interface
from .structs import LeAudioContext, LeAudioCisDirection

class LeUnicastManager(FirmwareComponent):
    ''' This class reports the LE Unicast Manager '''

    def __init__(self, env, core, parent=None):
        FirmwareComponent.__init__(self, env, core, parent=parent)
        try:
            self._le_um = env.vars["le_unicast_context"]
        except KeyError:
            raise self.NotDetected

    @property
    def le_um(self):
        return self._le_um

    def _generate_cis_data_table(self):
        header_row = ["CIS ID", "Direction", "Pending Data Cfm", "CIS Handle", "CIS State"]
        # CIS Delegation is applicable only for multidevice application
        if hasattr(self.le_um.cis[0], "is_cis_delegated"):
            header_row.append("Is Delegated")
        tbl = interface.Table(header_row)
        for cis in self.le_um.cis:
            data_row = []
            data_row.append(cis.cis_id)
            data_row.append(LeAudioCisDirection(self._core, cis.dir))
            data_row.append(bool_to_yes_no(cis.pending_data_cfm.value))
            data_row.append(cis.cis_handle)
            data_row.append(trim_string(cis.state.value_string, "le_um_cis_state_"))
            if hasattr(cis, "is_cis_delegated"):
                data_row.append(bool_to_yes_no(cis.is_cis_delegated.value))
            tbl.add_row(data_row)
        return tbl
    
    def _generate_ase_data_table(self):
        tbl = interface.Table(["ASE ID", "Direction", "Codec Version", "ASE State", "Audio Context", "CIS ID"])
        for ase in self.le_um.ase:
            state = trim_string(ase.state.value_string, "le_um_ase_state_")
            direction = {0x01: "Sink", 0x02: "Source"}.get(ase.direction.value, "NA")
            cis_id = None
            if ase.cis_data.value:
                cis_id = ase.cis_data.deref.cis_id
            tbl.add_row([ase.ase_id, direction, ase.codec_version, state, LeAudioContext(self._core, ase.audio_context), cis_id])
        return tbl

    def _generate_report_body_elements(self):
        grp = interface.Group("LE Unicast Manager")
        with self.le_um.footprint_prefetched():
            ase_tbl = self._generate_ase_data_table()
            cis_tbl = self._generate_cis_data_table()
            grp.append(ase_tbl)
            grp.append(cis_tbl)
        return [grp]

def trim_string(string, prefix):
    return string.replace(prefix, "")

def bool_to_yes_no(value):
    return "Yes" if value else "No"