############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2023 Qualcomm Technologies International, Ltd.
#   %%version
#
############################################################################

from csr.dev.fw.firmware_component import FirmwareComponent
from csr.dev.model import interface

 
class MediaControlClient(FirmwareComponent):
    '''
        This class reports the Media Control Profile client state.
    '''

    def __init__(self, env, core, parent=None):
        FirmwareComponent.__init__(self, env, core, parent=parent)

        try:
            self._mcp_clients = env.cu.media_control_client.local.media_control_taskdata.media_client_instance
        except AttributeError:
            raise self.NotDetected
            
    @property
    def mcp_clients(self):
        ''' Returns the media control client data instance '''
        return self._mcp_clients

    def _generate_report_body_elements(self):
        ''' Report the Media Control Profile client state '''
        grp = interface.Group("Media Control Client")
                
        client_table = interface.Table(["CID", "Client State", "Server State"])
        
        for client in self.mcp_clients:
            cid = f"0x{client.value['cid']:08x}"
            client_table.add_row([cid, client.state, client.server_state])
            
        grp.append(client_table)

        return [grp]
