############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2023 Qualcomm Technologies International, Ltd.
#   %%version
#
############################################################################

from csr.dev.fw.firmware_component import FirmwareComponent
from csr.dev.model import interface
from csr.dev.fw.bd_addr import BdAddr
 
class GattConnect(FirmwareComponent):
    '''
        This class reports the gatt_connect module state.
    '''

    def __init__(self, env, core, parent=None):
        FirmwareComponent.__init__(self, env, core, parent=parent)

        try:
            self._gatt_connect_list = env.cu.gatt_connect_list.local.connections
        except AttributeError:
            raise self.NotDetected
            
    @property
    def gatt_connect_list(self):
        ''' Returns the gatt_connect_list data instance '''
        return self._gatt_connect_list

    def _generate_report_body_elements(self):
        ''' Report the GATT Connect state '''
        grp = interface.Group("GATT Connect")
        
        connection_table = interface.Table(["CID", "BD Address"])
        for connection in self.gatt_connect_list:
            cid = f"0x{connection.value['cid']:08x}"
            tpaddr = connection['tpaddr']
            addr = BdAddr.from_struct(tpaddr.taddr.addr)
            connection_table.add_row([cid, addr])
            
        grp.append(connection_table)

        return [grp]
