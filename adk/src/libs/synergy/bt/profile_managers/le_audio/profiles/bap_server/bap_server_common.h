/****************************************************************************
* Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
* %%version
************************************************************************* ***/

#ifndef BAP_SERVER_COMMON_H_
#define BAP_SERVER_COMMON_H_

#include "bap_server_private.h"

#define BAP_METADATA_LTV_TYPE_PREFERRED_AUDIO_CONTEXTS                    0x01
#define BAP_METADATA_LTV_TYPE_STREAMING_AUDIO_CONTEXTS                    0x02
#define BAP_METADATA_LTV_TYPE_PROGRAM_INFO                                0x03
#define BAP_METADATA_LTV_TYPE_LANGUAGE                                    0x04
#define BAP_METADATA_LTV_TYPE_CCID_LIST                                   0x05
#define BAP_METADATA_LTV_TYPE_PARENTAL_RATING                             0x06
#define BAP_METADATA_LTV_TYPE_PROGRAM_INFO_URI                            0x07
#define BAP_METADATA_LTV_TYPE_EXTENDED_METADATA                           0xFE 
#define BAP_METADATA_LTV_TYPE_VENDOR_SPECIFIC                             0xFF

#define BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY                      0x01
#define BAP_CODEC_CONFIG_LTV_TYPE_FRAME_DURATION                          0x02
#define BAP_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION                0x03
#define BAP_CODEC_CONFIG_LTV_TYPE_OCTETS_PER_FRAME_CODEC                  0x04
#define BAP_CODEC_CONFIG_LTV_TYPE_CODEC_FRAME_BLOCKS_PER_SDU              0x05

#define BAP_CODEC_CONFIG_LTV_SIZE_AUDIO_CHANNEL_ALLOCATION                0x04
#define BAP_CODEC_CONFIG_LTV_SIZE_OCTETS_PER_CODEC_FRAME                  0x02

#define BAP_CODEC_CONFIG_LTV_TYPE_SAMPLING_FREQUENCY_VS                   0x81
#define BAP_CODEC_CONFIG_LTV_TYPE_AUDIO_CHANNEL_ALLOCATION_VS             0x83

#define BAP_LTV_LENGTH_OFFSET               0x00
#define BAP_LTV_TYPE_OFFSET                 0x01
#define BAP_LTV_VALUE_OFFSET                0x02

#define BAP_SERVER_DEFAULT_APTX_LITE_FRAME_DURATION   (5000)  /* 5 ms */

bool bapServerIsValidConectionId(BAP *bapInst,
                                 ConnId connectionId);

bool bapServerIsLtvValueInRange(uint8 * data,
                                uint8 dataLength,
                                uint8 ltvLength,
                                uint8 ltvType,
                                uint32 ltvMinValue,
                                uint32 ltvMaxValue);

void bapServerQosParamInit(void);

#endif
