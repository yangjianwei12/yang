import argparse
import time
import json
import sys
import os
import subprocess

version = '0.1.10'  # version number of the script

'''
Add the Dependencies
'''
DEPENDENT_MODULES = ['numpy']

'''
ML_ENGINE capability ID
'''
ML_ENGINE_CAP_ID = 210
'''
MLE operator message to load a model file. This message will parse the model file. The message structure is:
[OPMSG_ML_ENGINE_LOAD_MODEL, usecase_id, file_handle, access_method]
OPMSG_ML_ENGINE_LOAD_MODEL: Message ID (0x14)
usecase_id: usecase identifier, should be same as the usecase identifier in the model file
file_handle: kymera file handle after the file is downloaded.
access_method: Allows access method for the model in addition to load or unload.
               0 = direct model access            --> Directly access the model from file manager's memory space.
                                                      If this option is being used, we should compulsorily send the
                                                      OPMSG_ML_ENGINE_UNLOAD_MODEL to unload the model.
               1 = copy access and auto unload    --> Copies the content of the model file into DM and releases the file
                                                      handle. If this option is being used, we do not need to send
                                                      the OPMSG_ML_ENGINE_UNLOAD_MODEL operator message.
               2 = copy access and no auto unload --> Copies the content of the model file into DM but does not release
                                                      the file handle. If this option is being used, we should
                                                      compulsorily send the OPMSG_ML_ENGINE_UNLOAD_MODEL to unload the
                                                      model.
'''
OPMSG_ML_ENGINE_LOAD_MODEL = 0x14  # MLE Operator message to load single model file

'''
MLE operator message to unload a model file. This message can be used to remove the file from DM RAM if
access_method option was set to 0 or 2 in LOAD_MODEL operator message. The message structure is
[OPMSG_ML_ENGINE_UNLOAD_MODEL, usecase_id, file_handle]
OPMSG_ML_ENGINE_UNLOAD_MODEL: Message ID (0x16)
usecase_id: usecase identifier.
file_handle: kymera file handle of the model.
'''
OPMSG_ML_ENGINE_UNLOAD_MODEL = 0x16  # MLE Operator message to unload single model file

'''
MLE operator message to activate a model. This will allocate memory for internal use of the model.
The message structure is:
[OPMSG_ML_ENGINE_ACTIVATE_MODEL, usecase_id]
OPMSG_ML_ENGINE_ACTIVATE_MODEL: Message ID (0x15)
useasce_id: usecase identifer.
'''
OPMSG_ML_ENGINE_ACTIVATE_MODEL = 0x15  # MLE Operator message to activate model

'''
MLE operator message to set input tensor sequence. This message will inform the MLE capability about the
packing of input tensors in its input buffer. This operator message is not required if the sequence in
which the input tensors are packed in the input buffer is same as the order in the model file.If the sequence
is not the same, this operator message should be send after the "activate_model" operator message.
The message structure is:
[OPMSG_ML_ENGINE_SET_INPUT_TENSOR_SEQUENCE, usecase_id, num_tensors, tensor_id's]
OPMSG_ML_ENGINE_SET_INPUT_TENSOR_SEQUENCE: Message ID (0x2)
usecase_id: usecase identifer for which tensors are populated in the capability's input buffer
num_tensors: number of tensors in the input buffer.
tensor_id's: List of the tensor id of the tensors present in the input buffer. The order should be same
             as the order of tensors in the input.
Example: A payload of [1,3,1,0,2] means that:
         * The tensors are for usecase id: 1
         * There are 3 tensors in the input buffer of the capability
         * The sequence of the tensors in the input buffer is [tensor_id_1, tensor_id_0, tensor_id_2]
'''

'''
MLE Operator message to set data format for its input and output terminals. The message structure is
[OPMSG_ML_ENGINE_SET_DATA_FORMAT, input_format, output_format]
OPMSG_ML_ENGINE_SET_DATA_FORMAT: Message ID(0x1)
input_format: Input data format.
output_format: Output data format.
Example: A payload of [13, 13] as in the attached mle_svad.cfg.json file means that:
         * The input data format is 13: 32-bit packed data
         * The output data format is 13:32 bit packed data
'''
OPMSG_COMMON_ID_GET_STATUS = 8198

FILE_ID = 0
USECASE_ID = 0
ACCESS_METHOD = 0
BATCH_RESET_COUNT = 0

'''
Based on the MODEL_SIZE_THRESHOLD we choose memory type and access method.
For models which has size less than 300Kb we choose main heap and copy access.
For models more than 300Kb we choose extra heap and direct access.
'''
MODEL_SIZE_THRESHOLD = 300 * 1024

# starting status of VAD
VAD_STATUS = 0
ML_EXAMPLE_SVAD_UNSOLICITED_MESSAGE_ID = 101
UNSOLICITED_MESSAGE_BATCH_NUM_OFFSET = 4
UNSOLICITED_MESSAGE_VAD_STATUS_OFFSET = 5

TIMEOUT_IN_SEC = 30

ACCESS_METHOD_KEY = 'access_method'
MEMORY_TYPE_KEY = 'memory_type'


def exception_handler(func):
    def inner_function(*args, **kwargs):
        try:
            func(*args, **kwargs)
            print('{} completed'.format(func.__name__))
            return True
        except RuntimeError:
            print('{} failed. Check the audio firmware logs for more details.'.format(func.__name__))
            return False
    return inner_function


def cb_vad_status(data):
    """
    Callback to handle unsolicited messages from ml_example_svad
    capability.

    If the message comes from the capability, print the batch number and status.

    Args:
        data (list[int]): accmd data received
    """
    global VAD_STATUS
    from kse.kymera.kymera.generic.accmd import ACCMD_CMD_ID_MESSAGE_FROM_OPERATOR_REQ
    cmd_id, _, payload = kymera._accmd.receive(data)

    if cmd_id == ACCMD_CMD_ID_MESSAGE_FROM_OPERATOR_REQ:
        if payload[1:3] == [0, ML_EXAMPLE_SVAD_UNSOLICITED_MESSAGE_ID]:
            batch_num = payload[UNSOLICITED_MESSAGE_BATCH_NUM_OFFSET]
            vad_status = payload[UNSOLICITED_MESSAGE_VAD_STATUS_OFFSET]
            if vad_status != VAD_STATUS:
                print('Batch -> %d, VAD Status -> %d' % (batch_num, vad_status))
                VAD_STATUS = vad_status

@exception_handler
def download_files(graph_file):
    """
    Download the MLE model file and populate
    the file id, usecase_id and access_method options

    Args:
        graph_file: KSE Graph file
    """
    global FILE_ID, USECASE_ID, BATCH_RESET_COUNT, ACCESS_METHOD
    with open(graph_file) as f:
        kse_graph = json.load(f)
    item = kse_graph['file_mgr_file'][0]
    # Check if the config files contains the values of access_method
    # and memory_type to be used
    if ACCESS_METHOD_KEY in item.keys() and MEMORY_TYPE_KEY in item.keys():
        memory_type = item[MEMORY_TYPE_KEY]
        ACCESS_METHOD = item[ACCESS_METHOD_KEY]
    else:
        # either is not present, decide based on a threshold
        if os.stat(item['filename']).st_size > MODEL_SIZE_THRESHOLD:
            memory_type = 4
            ACCESS_METHOD = 0
        else:
            memory_type = 1
            ACCESS_METHOD = 1
    FILE_ID = hydra_file_manager.download(item['filename'], memory_type=memory_type, auto_remove=1)
    USECASE_ID = item['usecase_id']
    BATCH_RESET_COUNT = item['batch_reset_count']
    print('downloaded file_manager file:{}'.format(item['filename']))


def check_depedencies():
    """
    It checks for dependent modules, installs them if not present.
    """
    for i in range(len(DEPENDENT_MODULES)):
        subprocess.check_call([sys.executable, '-m', 'pip', 'install', DEPENDENT_MODULES[i]])


def generate_input_data(args):
    """
    Check for input pickle file and make it as raw file, if pickle file not present use raw file as input.
    """
    with open(args.graph_file) as f:
        kse_graph = json.load(f)
    output_bin_file = kse_graph['stream'][0]['kwargs']['filename']
    key = 'input_pickle_file'
    input_pickle_file = ""
    if key in kse_graph.keys():
        check_depedencies()
        input_pickle_file = kse_graph['input_pickle_file']
        if os.path.isfile(input_pickle_file):
            from pickle_to_raw import convert_pickle_to_raw
            key = 'num_input_batches'
            if key in kse_graph.keys():
                batch_size = kse_graph['num_input_batches']
                convert_pickle_to_raw(input_pickle_file, output_bin_file, batch_size)
            else:
                convert_pickle_to_raw(input_pickle_file, output_bin_file)


def set_kse_config_params():
    from kse.framework.library.config import set_config_param
    set_config_param(param='KALCMD_LOCK_TIMEOUT',
                     value=TIMEOUT_IN_SEC)
    set_config_param(param='UUT_MESSAGE_RECEIVE_TIMEOUT',
                     value=TIMEOUT_IN_SEC)


def print_kpi_info():
    """
    Print KPI info for ML_ENGINE capability only
    """
    # Save the original reference of standard output
    original_ref = sys.stdout
    with open('log/kpi_info.log', 'w') as f:
        sys.stdout = f
        try:
            ml_profiler = acat.p0.get('ml_profiler')
            print('max_cycle=', ml_profiler.run_clks_max.value)
        except NameError:
            pass
        sys.stdout = original_ref


@exception_handler
def load_model(op_id):
    global FILE_ID, USECASE_ID, ACCESS_METHOD, BATCH_RESET_COUNT
    # Load the MLE Model File
    kymera.opmgr_operator_message(op_id,
                                  [OPMSG_ML_ENGINE_LOAD_MODEL,
                                   USECASE_ID,
                                   BATCH_RESET_COUNT,
                                   FILE_ID,
                                   ACCESS_METHOD])


@exception_handler
def activate_model(op_id):
    global USECASE_ID
    kymera.opmgr_operator_message(op_id,
                                  [OPMSG_ML_ENGINE_ACTIVATE_MODEL,
                                   USECASE_ID])


@exception_handler
def query_compatible_version(op_id, keai_version_expected):
    status = kymera.opmgr_operator_message(op_id, [OPMSG_COMMON_ID_GET_STATUS])
    keai_version = int(status[1])
    print('KEAI file format version expected by the firmware is {}'.format(keai_version))
    if keai_version_expected and keai_version_expected != keai_version:
        msg = 'The KEAI file format version is:' + str(keai_version_expected) + ' whereas the firmware expects:' + str(
                keai_version)
        sys.exit(msg)

def run_ml_graph(args):
    """
    Run the ML graph.

    Args:
        args (argparser.args): parsed command-line arguments
    """
    # set kse config params
    set_kse_config_params()
    # read pickle file and populate raw file
    generate_input_data(args)
    from kse.framework.library.file_util import load
    data = load(args.graph_file)
    graph.load(data)
    print('Graph loaded')

    # Register accmd callback to get VAD status.
    # Useful only for ml_example_svad capability - used only for ml_example_svad.cfg.json
    cb_handler = kymera._accmd.register_receive_callback(cb_vad_status)

    graph.create()
    print('Graph created')

    # query the firmware for the major version of the KEAI file format version it supports and compare it to
    # the version provided in the argument to the script. In case of a mis-match,exit the script with a message
    if args.keai_version and not query_compatible_version(graph.get_operator(0).get_id(), args.keai_version):
        print('Older version of the firmware does not support GET_STATUS opmsg for ML_ENGINE. Assuming that the KEAI'
              'file format version supplied is same as what the firmware expects. ')

    # download the model file required for the MLE capability
    if not download_files(args.graph_file):
        sys.exit()
    if not load_model(graph.get_operator(0).get_id()):
        sys.exit()
    if not activate_model(graph.get_operator(0).get_id()):
        sys.exit()

    graph.config()
    print('Graph configured')

    graph.connect()
    print('Graph connected')

    print('Graph starting')
    graph.start()

    while graph.check_active():
        time.sleep(2)

    print('Graph Finished!')

    # If the capability being tested is ML_ENGINE, then poke for KPI information
    if graph.get_operator(0).cap_id == ML_ENGINE_CAP_ID:
        print_kpi_info()

    # Add delay to ensure that the complete output file is generated
    time.sleep(2)
    graph.stop()
    graph.disconnect()

    if ACCESS_METHOD != 1:
        # remove the model file from DM RAM
        status = kymera.opmgr_operator_message(graph.get_operator(0).get_id(),
                                               [OPMSG_ML_ENGINE_UNLOAD_MODEL, USECASE_ID, FILE_ID])

    graph.destroy()
    print('Graph destroyed')

    # Unregister callback
    kymera._accmd.unregister_receive_callback(cb_handler)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='ML capability startup kalsim shell script, version=' + version)
    parser.add_argument('graph_file', type=str, help='KSE Graph File')
    parser.add_argument('--keai_version', type=int, help='KEAI file format version of the supplied KEAI file',
                        required=False)
    args = parser.parse_args()
    run_ml_graph(args)
