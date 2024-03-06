############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2023 Qualcomm Technologies International, Ltd.
#   %%version
#
############################################################################

from csr.dev.fw.firmware_component import FirmwareComponent
from csr.dev.model import interface

class AudioSource(FirmwareComponent):
    '''
        Decoder for the audio_router_data_t structure.
    '''

    def __init__(self, env, core, audio_source, parent=None):
        FirmwareComponent.__init__(self, env, core, parent=parent)
        self._audio_source = audio_source

    @property
    def type(self):
        ''' Returns the type of audio source - voice or audio '''
        return self._audio_source.source.type

    @property
    def source(self):
        ''' Returns the source enum value '''
        if self._audio_source.source.type.value == self.env.enums["source_type_t"]["source_type_voice"]:
            return self._audio_source.source.u.voice
        elif self._audio_source.source.type.value == self.env.enums["source_type_t"]["source_type_audio"]:
            return self._audio_source.source.u.audio

    @property
    def state(self):
        ''' Returns the source state '''
        return self._audio_source.state

    @property
    def present(self):
        ''' Returns if the audio source is present (registered) '''
        return bool(self._audio_source.present.value)

class AudioRouter(FirmwareComponent):
    '''
        This class reports the audio router state.
    '''

    def __init__(self, env, core, parent=None):
        FirmwareComponent.__init__(self, env, core, parent=parent)

        try:
            self._audio_router = env.cu.audio_router_data.local.audio_router_data_container
        except AttributeError:
            raise self.NotDetected

    @property
    def audio_router(self):
        ''' Returns the audio router data instance '''
        return self._audio_router

    def _audio_source_generator(self):
        ''' Iterates the audio source data array yielding audio source states '''
        if self.audio_router != 0:
            for audio_source in self.audio_router.data:
                yield AudioSource(self.env, self._core, audio_source, self)

    def _generate_report_body_elements(self):
        ''' Report the audio router state '''
        grp = interface.Group("Audio Router")

        overview = interface.Table(["Name", "Value"])
        overview.add_row(["Last Routed Audio Source", self.audio_router.last_routed_audio_source])
        grp.append(overview)

        tbl = interface.Table(["Type", "Source", "State"])
        with self._audio_router.footprint_prefetched():
            for audio_source in self._audio_source_generator():
                if audio_source.present:
                    tbl.add_row([audio_source.type, audio_source.source, audio_source.state])
        grp.append(tbl)
        return [grp]

    @property
    def audio_sources(self):
        ''' Returns a list of audio source states '''
        return list(self._audio_source_generator())