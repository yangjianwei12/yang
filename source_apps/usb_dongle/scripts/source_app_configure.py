"""
Copyright (c) 2023 Qualcomm Technologies International, Ltd.

A script to exercise sending HID commands from host to USB dongle for configuring dongle behaviour
-----------------------------------------------------------------------------------------------------------------

Dependency Package Installation: pip install pywinusb

Usages:
HID commands to modify functionality to the USB source dongle.

To see help : py source_app_configure.py -h
usage: source_app_configure.py [-h] [-n NAME] [-c CODE] [--nc][-a MODE] [--bcast_adv] [--bcast_conf] [--bcast_id]

Source dongle configure

Optional arguments:
  -h, --help            show this help message and exit
  -n NAME, --name NAME  LE-Audio Broadcast source name
  -c CODE, --code CODE  LE-Audio Broadcast code with 4 to 16 (only of EVEN size) ascii char
  --nc                  To disable broadcast encryption
  -a AUDIO_MODE, --audio_mode AUDIO_MODE
                        Audio mode (0: High Quality, 1: Gaming, 2: LE-Audio Broadcast)
  -t TRANSPORT_MODE, --transport_mdoe TRANSPORT_MODE
                        Transport mode (1: BREDR, 2: LE, 3: Dual)
  --at audio_mode transport_mode, --audio_transport_mode audio_mode transport_mode
                        audio_mode (0: High Quality, 1: Gaming, 2: LE-Audio
                        Broadcast), transport_mode (1: BREDR, 2: LE, 3:Dual)
  --bcast_adv BCAST_ADV
                        Advertisment Settings for PBP and TMAP Broadcast (See BapBroadcastSrcAdvParams/PS_KEY_USB_DONGLE_BROADCAST_ADV_CONFIG for the format)
  --bcast_conf BCAST_CONF
                        BAP Audio Config Broadcast (See LE_AUDIO_CLIENT_BROADCAST_CONFIG_T/PS_KEY_BROADCAST_SRC_AUDIO_CONFIG for the format)
  --bcast_id BCAST_ID
                      six digit Broadcast ID random number generation as defined in Volume 3, Part H, Section 2 in Core specAudio Config Broadcast (See PS_KEY_BROADCAST_ID for the format)

Examples:

  To set new audio modes:
    Switch to high quality mode                         : py source_app_configure.py -a 0
    Switch to gaming mode                               : py source_app_configure.py -a 1
    Switch to broadcast mode                            : py source_app_configure.py -a 2
  To set transport mode:
    Switch to BREDR transport                           : py source_app_configure.py -t 1
    Switch to LE transport                              : py source_app_configure.py -t 2
    Switch to Dual transport                            : py source_app_configure.py -t 3
  To set audio and transport mode together:
    Switch to high quality audio, BREDR transport mode  : py source_app_configure.py --at 0 1
    Switch to gaming audio mode, LE transport mode      : py source_app_configure.py --at 1 2
  To set broadcast source parameter, use following options
    name                                                : py source_app_configure.py -n "name_for_broadcast"
    code                                                : py source_app_configure.py -c "1234561234561234"
      To disable encryption                             : py source_app_configure.py --nc
    Adv                                                 : py source_app_configure.py --bcast_adv "00 00 00 60 00 66 00 01 07 00 00 01 00 01 02 10 02 20 14 00"
                                                            Format (all bytes are mandatory)
                                                              Byte 0, 1   - Advertising type.
                                                              Byte 2, 3   - Advertising Interval Min in units 0.625 ms
                                                              Byte 4, 5   - Advertising Interval Max in units 0.625 ms
                                                              Byte 6, 7   - Primary Advertising Phy (1 - LE 1M, 3 - LE Coded)
                                                              Byte 8, 9   - Primary Advertising Channel Map (bit 0 = Channel 37, bit 1 = Channel 38 and bit 2 = Channel 39)
                                                              Byte 10     - Secondary Advertising Max Skip before sending an AUX_ADV_IND
                                                              Byte 11     - Secondary Advertising Phy (1 - LE 1M, 3 - LE Coded)
                                                              Byte 12, 13 - Advertsing set ID
                                                                              0x100 = Legacy advertising
                                                                              0x400 = Stack will assign unique value
                                                                              0 to 15 = Application decides unique value
                                                                              0x200 + (0 to 15) = More than 1 advertising set can have this value
                                                              Byte 14, 15 - Periodic Advertising Interval Min in 1.25 ms
                                                              Byte 16, 17 - Periodic Advertising Interval Max in 1.25 ms
                                                              Byte 18     - Advertising Tx Power Range: -127 to 20 dbM (default is 20dbM, for specifying -127 set it to 81)
                                                              Byte 19     - Set to 00, RFU (Reserved for Future use).
    Config                                              : py source_app_configure.py --bcast_conf "04 01 00 64 00 14 00 02 00 02 00 01 00 00 00 04 00 00 00 04 00 00 08 00 00 00 27 10 00 00 4E 20"
                                                            Format (all bytes are mandatory)
                                                              Byte 0      - Number of retransmission (Default 0x04)
                                                              Byte 1      - Max. codec frame per SDU (Default 0x01)
                                                              Byte 2, 3   - SDU size (Default 0x64)
                                                              Byte 4, 5   - Max. latency in ms (Default 20ms)
                                                              Byte 6, 7   - Phy channel for broadcast (Default BAP_LE_2M_PHY)
                                                                              0x0001 = BAP_LE_1M_PHY
                                                                              0x0002 = BAP_LE_2M_PHY
                                                                              0x0003 = BAP_LE_CODED_PHY
                                                              Byte 8, 9   - No. of BIS (Default 0x02)
                                                              Byte 10, 11 - No. of channels per BIS (Default 0x1)
                                                              Byte 12, 15 - Audio context as per CapClientContext(Default 0x04)
                                                                              0x00000000 = CAP_CLIENT_CONTEXT_TYPE_PROHIBITED
                                                                              0x00000001 = CAP_CLIENT_CONTEXT_TYPE_UNSPECIFIED
                                                                              0x00000004 = CAP_CLIENT_CONTEXT_TYPE_MEDIA
                                                                              0x00000008 = CAP_CLIENT_CONTEXT_TYPE_GAME
                                                                              0x00000010 = CAP_CLIENT_CONTEXT_TYPE_INSTRUCTIONAL
                                                                              0x00000020 = CAP_CLIENT_CONTEXT_TYPE_VOICE_ASSISTANT
                                                                              0x00000080 = CAP_CLIENT_CONTEXT_TYPE_SOUND_EFFECTS
                                                                              0x00000100 = CAP_CLIENT_CONTEXT_TYPE_NOTIFICATIONS
                                                                              0x00000200 = CAP_CLIENT_CONTEXT_TYPE_RINGTONE
                                                                              0x00000400 = CAP_CLIENT_CONTEXT_TYPE_ALERTS
                                                                              0x00000800 = CAP_CLIENT_CONTEXT_TYPE_EMERGENCY_ALARM
                                                              Byte 16, 19  - Broadcast type mask as per CapClientBcastType (Default HQ_PUBLIC_BROADCAST)
                                                                              0x00000002 = SQ_PUBLIC_BROADCAST
                                                                              0x00000004 = HQ_PUBLIC_BROADCAST
                                                                              0x00000008 = TMAP_BROADCAST
                                                              Byte 20, 23  - Broadcast audio config as per CapClientSreamCapability (Default Audio config 48_2)
                                                                              0x00000001 = Audio config 8_1
                                                                              0x00000001 = Audio config 8_1
                                                                              0x00000002 = Audio config 8_2
                                                                              0x00000004 = Audio config 16_1
                                                                              0x00000008 = Audio config 16_2
                                                                              0x00000010 = Audio config 24_1
                                                                              0x00000020 = Audio config 24_2
                                                                              0x00000040 = Audio config 32_1
                                                                              0x00000080 = Audio config 32_2
                                                                              0x00000400 = Audio config 48_1
                                                                              0x00000800 = Audio config 48_2
                                                                              0x00001000 = Audio config 48_3
                                                                              0x00002000 = Audio config 48_4
                                                                              0x00004000 = Audio config 48_5
                                                                              0x00008000 = Audio config 48_6
                                                              Byte 24, 27   - SDU interval in us (Default 10ms)
                                                              Byte 28, 31   - Presentation delay in us (Default 20ms)
    ID                                                  : py source_app_configure.py --bcast_id "00 11 22 33"
                                                                Format
                                                              Byte 0      - Unused, set it to 00
                                                              Byte 1-3    - 6 digit Broadcast ID 
"""

import pywinusb.hid as hid
import sys
import argparse

SOURCE_APP_VENDOR_ID = 0x0a12
SOURCE_APP_VENDOR_PAGE = 0xff00
SOURCE_APP_USAGE_ID = 0x02
SOURCE_APP_CONFGIURE_REPORT_ID = 32

# Command types for configuring source app
HID_CMD_SET_MODE                        = 0
HID_CMD_SET_LEA_BROADCAST_NAME          = 1
HID_CMD_SET_LEA_BROADCAST_CODE          = 2
HID_CMD_SET_LEA_BROADCAST_ADV_SETTING   = 3
HID_CMD_SET_LEA_BROADCAST_AUDIO_CONFIG  = 4
HID_CMD_SET_LEA_BROADCAST_ID            = 5

HID_CMD_SET_LEA_BROADCAST_ADV_SETTING_LEN   = 20
HID_CMD_SET_LEA_BROADCAST_AUDIO_CONFIG_LEN  = 32
HID_CMD_SET_LEA_BROADCAST_ID_LEN            = 4

# Invalid values for audio/transport mode
INVALID_AUDIO_MODE = 0xFF
INVALID_TRANSPORT_MODE = 0xFF

# This minimum size required as 255 used in descriptor
HID_BUFFER_SIZE = 255

def build_hid_command(cmd, hex_str):
    cmd_data = bytearray.fromhex(hex_str)
    hex_str_bytes = bytearray(HID_BUFFER_SIZE)
    hex_str_bytes[0] = cmd
    hex_str_bytes[1] = len(cmd_data)
    for i in range(len(cmd_data)):
        hex_str_bytes[2+i] = cmd_data[i]

    return hex_str_bytes

def send_hid_command(hid_data_buffer):
    """
    This function will find a particular target_usage over feature reports on target_vendor_id related devices,
    then it will send the prepared report
    """

    # usually, you'll find and open the target device, here we'll browse for the current connected devices
    target_vendor_id = SOURCE_APP_VENDOR_ID
    target_usage = hid.get_full_usage_id(SOURCE_APP_VENDOR_PAGE, SOURCE_APP_USAGE_ID) # generic vendor page, usage_id = 2

    all_devices = hid.HidDeviceFilter(vendor_id = target_vendor_id).get_devices()

    if not all_devices:
        print("Can't find target device (vendor_id = 0x%04x)!" % target_vendor_id)
    else:
        # search for our target device
        for device in all_devices:
            try:
                device.open()
                # browse feature reports
                for report in device.find_feature_reports():
                    # Check if usage ID and report ID matches
                    if target_usage in report and getattr(report,'report_id') == SOURCE_APP_CONFGIURE_REPORT_ID:
                        # found out target!, Update the data and send
                        report[target_usage] = hid_data_buffer 
                        report.send()
                        print("HID command sent successfully\n")
                        return
            finally:
                device.close()
        print("The target device was found, but the requested usage does not exist!\n")


def send_lea_broadcast_name(src_name, src_name_len):
    hid_data = bytearray(HID_BUFFER_SIZE)
    # Fill in the HID data to send
    hid_data[0] = HID_CMD_SET_LEA_BROADCAST_NAME
    src_name_as_bytes = bytes(src_name, 'utf-8')
    hid_data[2 : 2 + src_name_len] = src_name_as_bytes
    hid_data[1] = ((src_name_len + 1) // 2) * 2

    # Send HID command
    send_hid_command(hid_data)

def send_lea_broadcast_code(src_code, src_code_len):
    hid_data = bytearray(HID_BUFFER_SIZE)
    hid_data[0] = HID_CMD_SET_LEA_BROADCAST_CODE
    if src_code_len == 0:
        # No code. ie,unencrypted
        hid_data[1] = 0

        # Send HID command
        send_hid_command(hid_data)
    # Validate broadcast code length
    elif (src_code_len % 2) == 0 and src_code_len >= 4 and src_code_len <= 16:
        hid_data[1] = src_code_len
        src_code_as_bytes = bytes(src_code, 'utf-8')
        hid_data[2 : 2 + src_code_len] = src_code_as_bytes
        
        # Send HID command
        send_hid_command(hid_data)
    else:
        print("Invalid code length, Min 4 to Max 16 digit (only of EVEN size)")

def send_lea_broadcast_adv_setting(adv_settings):
    hid_data = build_hid_command(HID_CMD_SET_LEA_BROADCAST_ADV_SETTING, adv_settings) 

    if hid_data[1] == HID_CMD_SET_LEA_BROADCAST_ADV_SETTING_LEN:
        # Send HID command
        send_hid_command(hid_data)
    else:
        print("Invalid advertising settings length")

def send_lea_broadcast_audio_config(audio_config):
    hid_data = build_hid_command(HID_CMD_SET_LEA_BROADCAST_AUDIO_CONFIG, audio_config) 

    if hid_data[1] == HID_CMD_SET_LEA_BROADCAST_AUDIO_CONFIG_LEN:
        # Send HID command
        send_hid_command(hid_data)
    else:
        print("Invalid audio config length")

def send_lea_broadcast_id(bcast_id):
    hid_data = build_hid_command(HID_CMD_SET_LEA_BROADCAST_ID, bcast_id) 

    if hid_data[1] == HID_CMD_SET_LEA_BROADCAST_ID_LEN:
        # Send HID command
        send_hid_command(hid_data)
    else:
        print("Invalid Broadcast ID length")

def send_dongle_audio_mode(audio_mode):
    send_dongle_audio_transport_mode(audio_mode, INVALID_TRANSPORT_MODE)

def send_dongle_transport_mode(transport_mode):
    send_dongle_audio_transport_mode(INVALID_AUDIO_MODE, transport_mode)

def send_dongle_audio_transport_mode(audio_mode, transport_mode):
    hid_data = bytearray(HID_BUFFER_SIZE)
    # Check if audio mode and transport mode values are allowed or not
    if ((audio_mode == INVALID_AUDIO_MODE or audio_mode < 3) and
            (transport_mode == INVALID_TRANSPORT_MODE or (transport_mode >= 1 and transport_mode <= 3))):
        # Fill in the HID data to send
        hid_data[0] = HID_CMD_SET_MODE
        hid_data[1] = 2 # length
        hid_data[2] = int(audio_mode)
        hid_data[3] = int(transport_mode)

        # Send HID command
        send_hid_command(hid_data)
    else:
        print("Given audio/transport_mode is not correct!")

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Source dongle configure')
    parser.add_argument("-n", "--name", 
                        dest='name',
                        type=str,
                        help='LE-Audio Broadcast source name')
    parser.add_argument("-c", "--code",
                        dest='code',
                        type=str,
                        help='LE-Audio Broadcast code with 4 to 16 (only of EVEN size) ascii char')
    parser.add_argument("--nc",
                        dest='no_code',
                        action='store_true',
                        help='To disable broadcast encryption')
    parser.add_argument("-a", "--audio_mode",
                        dest='audio_mode',
                        type=int,
                        help='Audio mode (0: High Quality, 1: Gaming, 2: LE-Audio Broadcast)')
    parser.add_argument("-t", "--transport_mode",
                        dest='transport_mode',
                        type=int,
                        help='Transport mode (1: BREDR, 2: LE, 3:Dual)')
    parser.add_argument("--at", "--audio_transport_mode",
                        dest='audio_transport_mode',
                        type=int,
                        # Number of arguments = 2 (ie, audio & transport mode)
                        nargs = 2,
                        # name of arguments to display in help
                        metavar = ('audio_mode', 'transport_mode'),
                        help='audio_mode (0: High Quality, 1: Gaming, 2: LE-Audio Broadcast), transport_mode (1: BREDR, 2: LE, 3:Dual)')
    parser.add_argument("--bcast_adv",
                        dest='bcast_adv',
                        type=str,
                        help='Advertisment Settings for PBP and TMAP Broadcast (See BapBroadcastSrcAdvParams/PS_KEY_BROADCAST_ADV_CONFIG for the format)')
    parser.add_argument("--bcast_conf",
                        dest='bcast_conf',
                        type=str,
                        help='BAP Audio Config Broadcast (See LE_AUDIO_CLIENT_BROADCAST_CONFIG_T/PS_KEY_BROADCAST_SRC_AUDIO_CONFIG for the format)')

    parser.add_argument("--bcast_id",
                        dest='bcast_id',
                        type=str,
                        help='Broadcast ID (6 digit ID, see PS_KEY_BROADCAST_ID for the format)')

    args = parser.parse_args()

    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(1)

    # Change LEA broadcast source name
    if args.name != None:
        src_name_len = len(args.name)
        send_lea_broadcast_name(args.name, src_name_len)

    # Change LEA broadcast code
    if args.no_code:
        send_lea_broadcast_code(None, 0)
    elif args.code != None:
        src_code_len = len(args.code)
        send_lea_broadcast_code(args.code, src_code_len)

    # Change audio mode
    if args.audio_mode != None:
        send_dongle_audio_mode(args.audio_mode)

    # Change transport mode
    if args.transport_mode != None:
        send_dongle_transport_mode(args.transport_mode)

    # Change audio and transport mode together
    if args.audio_transport_mode != None:
        send_dongle_audio_transport_mode(args.audio_transport_mode[0], args.audio_transport_mode[1])

    # Change LEA Broadcast advert settings
    if args.bcast_adv != None:
        send_lea_broadcast_adv_setting(args.bcast_adv)

    # Change LEA Broadcast Audio config
    if args.bcast_conf != None:
        send_lea_broadcast_audio_config(args.bcast_conf)

    # Change LEA Broadcast ID
    if args.bcast_id != None:
        send_lea_broadcast_id(args.bcast_id)

