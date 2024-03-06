############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2023 Qualcomm Technologies International, Ltd.
#   %%version
#
############################################################################

from csr.dev.fw.firmware_component import FirmwareComponent
from csr.dev.model import interface

 
class CallControlClient(FirmwareComponent):
    '''
        This class reports the Call Control Profile client state.
    '''

    def __init__(self, env, core, parent=None):
        FirmwareComponent.__init__(self, env, core, parent=parent)

        try:
            self._ccp_clients = env.cu.call_control_client.local.call_control_taskdata.call_client_instance
        except AttributeError:
            raise self.NotDetected
            
    @property
    def ccp_clients(self):
        ''' Returns the call control client data instance '''
        return self._ccp_clients

    def _generate_report_body_elements(self):
        ''' Report the Call Control Profile client state '''
        grp = interface.Group("Call Control Client")
                
        client_table = interface.Table(["CID", "Client State", "Call 0 State", "Call 1 State"])
        
        for client in self.ccp_clients:
            cid = f"0x{client.value['cid']:08x}"
            call_0_state = client.call_state[0].state
            call_1_state = client.call_state[1].state
            client_table.add_row([cid, client.state, call_0_state, call_1_state])
            
        grp.append(client_table)

        return [grp]
