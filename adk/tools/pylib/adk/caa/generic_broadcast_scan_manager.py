############################################################################
# CONFIDENTIAL
#
# Copyright (c) 2020 Qualcomm Technologies International, Ltd.
#   %%version
#
############################################################################

from csr.dev.fw.firmware_component import FirmwareComponent
from csr.dev.model import interface

class GenericBroadcastScanManager(FirmwareComponent):
    ''' This class reports the GBSS server data and provides some
        reporting and notification related helper functions. '''
    
    @property
    def _call(self):
        ''' Access the fw.call object for this component/core. '''
        return self._FirmwareComponent__core.fw.call
    
    @property
    def _dm(self):
        ''' Access the dm object for this core. '''
        return self._FirmwareComponent__core.dm

    @property
    def server(self):
        ''' Returns the current server data. '''
        return self.env.var.gbss_server
    
    @property
    def scan_report(self):
        ''' Returns the current server report data. '''
        return self.server.gbss_report
    
    @property
    def scan_active_t(self):
        ''' Returns the enum dictionary. '''
        return self.env.enum.gbss_scan_active_t
    
    @property
    def scan_status_t(self):
        ''' Returns the enum dictionary. '''
        return self.env.enum.gbss_self_scan_status_t
    
    @property
    def broadcast_name(self):
        ''' Returns the broadcast name in the current server report data as a string. '''
        
        _bcast_name_cast = self.env.cast(self.scan_report.broadcast_name, "char[{}]".format(self.scan_report.broadcast_name_len))
        return "".join([chr(c) for c in _bcast_name_cast.value[:-1]])
    
    @broadcast_name.setter
    def broadcast_name(self, bcast_name):
        ''' Write the given string into on-chip memory and point the broadcast name to this.
            Also update the broadcast_name_len in the on-chip data to match.
            Note that any previous broadcast_name is freed when setting a new one. '''
        
        self.scan_report.broadcast_name_len.value = _bcast_name_len = len(bcast_name)+1
        
        self._call.free(self.scan_report.broadcast_name)
        
        _bcast_name_mem = self._call.new("uint8", _bcast_name_len)
        _bcast_name_mem.value = list(map(ord, bcast_name)) + [0]
        self.scan_report.broadcast_name.value = _bcast_name_mem.address
    
    @property
    def metadata(self):
        ''' Returns a list where each item is the metadata set for each subgroup. '''
        
        metadata_list = list()
        for sg in self.subgroups:
            metadata_cast = self.env.cast(sg.metadata, "uint8[{}]".format(sg.metadata_length))
            metadata_items = [m.value for m in metadata_cast]
            metadata_list.append(metadata_items)
        
        return metadata_list
    
    @property
    def subgroups(self):
        ''' Returns the subgroups in the current server report data as an array. '''
        return self.env.cast(self.scan_report.subgroups, "gbss_subgroups_t[{}]".format(self.scan_report.num_subgroups))
    
    @subgroups.setter
    def subgroups(self, metadata_list=[[]]):
        ''' Write the subgroups with the provided list of metadata items. Also sets num_subgroups.
            metadata_list is expected as a 2-level iterable e.g. [[a,b,c], []} 
            would configure 2 subgroups, the first one with 3 metadata items, the 2nd one with no metadata item.
            Note that subgroups and metadata are freed before writing the new sets. '''
        
        # Free any previous subgroups and metadata allocated within
        #for i in range(self.scan_report.num_subgroups.value):
        #   self._call.free(self.subgroups[i].metadata)
        for sg in self.subgroups:
            self._call.free(sg.metadata)
        self._call.free(self.subgroups)
        
        # Set num_subgroups based on number of items in metadata_list
        self.scan_report.num_subgroups.value = _subgroups_len = len(metadata_list)
        
        # Allocate subgroups and metadata within
        _subgroups_mem = self._call.new("gbss_subgroups_t", _subgroups_len)
        self.scan_report.subgroups.value = _subgroups_mem.address
        for i,metadata in enumerate(metadata_list):
            _subgroups_mem[i].metadata_length.value = len(metadata)
            _metadata_mem = self._call.new("uint8", len(metadata))
            _metadata_mem.value = list(metadata)
            _subgroups_mem[i].metadata.value = _metadata_mem.address
        self.scan_report.subgroups.value = _subgroups_mem.address
    
    
    def notify_scan_report(self):
        ''' Issue a scan report notification to the GBSS GATT client. '''
        
        self._call.GenericBroadcastScanServer_NotifyGbssScanReport()
    
    
    def scan_start(self):
        ''' Send a start scan report to the GATT client. '''
        
        self.scan_report.scan_active.value = self.scan_active_t["GBSS_SCAN_ACTIVE"]
        self.scan_report.scan_result.value = self.scan_status_t["SCANNING_NO_SOURCE"]
        
        self.notify_scan_report()
        
        
    def scan_stop(self):
        ''' Send a stop scan report to the GATT client. '''
                
        self.scan_report.scan_active.value = self.scan_active_t["GBSS_SCAN_INACTIVE"]
        self.scan_report.scan_result.value = self.scan_status_t["SCANNING_NO_SOURCE"]
        
        self.notify_scan_report()
    
    
    def scan_discovered(self, bcast_id, bcast_name="", encr=False, rssi=-95, adv_seid=15, subgroups_metadata=[[]], start=True, stop=True, verbose=True):
        ''' Send a broadcast discovered scan report to the GATT client. 
        - start True to notify a start scan before notifying the source found. (normally expected by the client)
        - stop True to notify a stop scan after notifying the source found. (normally expected by the client) '''
                
        if start:
            self.scan_start()
        
        # Scanner
        self.scan_report.scan_result.value = self.scan_status_t["SCANNING_SOURCE_FOUND"]
        # Basic parameters
        self.scan_report.encryption_required.value = encr
        self.scan_report.rssi.value = rssi
        self.scan_report.source_adv_sid.value = adv_seid
        self.scan_report.broadcast_id.value = bcast_id
        # Broadcast name
        self.broadcast_name = bcast_name
        # Subgroups
        self.subgroups = subgroups_metadata
        
        if verbose:
            self.report()
        
        self.notify_scan_report()
        
        if stop:
            self.scan_stop()
    

    def _generate_report_body_elements(self):
        ''' Report the scan report data.'''
        grp = interface.Group("Scan report")
        overview = interface.Table(['Name', 'Value'])
        overview.add_row(['Scan Active', self.scan_report.scan_active])
        overview.add_row(['Scan Result', self.scan_report.scan_result])
        overview.add_row(['Encryption Required', self.scan_report.encryption_required])
        overview.add_row(['RSSI', self.scan_report.rssi])
        overview.add_row(['Broadcast Name Length', self.scan_report.broadcast_name_len])
        overview.add_row(['Broadcast Name', self.broadcast_name])
        a = self.scan_report.source_address.addr
        t = self.scan_report.source_address.type
        overview.add_row(['Source Address', '{n}:{u}:{l} ({t})'.format(n=a.nap, u=a.uap, l=a.lap, t=t)])
        overview.add_row(['Source Adv SID', self.scan_report.source_adv_sid])
        overview.add_row(['Broadcast ID', self.scan_report.broadcast_id])
        overview.add_row(['PA Interval', self.scan_report.pa_interval])
        overview.add_row(['Number of Subgroups', self.scan_report.num_subgroups])
        subgroups = interface.Table(['Subgroup', 'Metadata'])
        for i,sg in enumerate(self.subgroups):
            subgroups.add_row([i+1, [hex(m) for m in self.metadata[i]]])
        overview.add_row(['Subgroups', subgroups])
        grp.append(overview)
        return [grp]
