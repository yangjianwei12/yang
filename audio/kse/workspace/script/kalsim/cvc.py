#
# Copyright (c) 2017 Qualcomm Technologies International, Ltd.
# All rights reserved.
# Qualcomm Technologies International, Ltd. Confidential and Proprietary.
#
# pylint: skip-file
# @PydevCodeAnalysisIgnore

import argparse
from kats.framework.library.file_util import load

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Download capability startup kalsim shell script')
    parser.add_argument('dkcs_file', type=str, help='Input bundle file (dkcs file)')
    parser.add_argument('graph_file', type=str, help='Graph configuration file (cfg.json file)')
    args = parser.parse_args()

    print('downloading file %s' % (args.dkcs_file))
    id = hydra_cap_download.download(args.dkcs_file)
    print('Bundle downloaded. Id is %s' % (id))

    print ('loading cVc graph %s' % (args.graph_file))
    data = load(args.graph_file)
    print ('cVc process started ...')

    graph.play(data)
    hydra_cap_download.remove(id)
    
    print ('cVc finished, bundle closed!')
