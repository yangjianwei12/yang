/******************************************************************************
 Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "cap_client_private.h"
#include "cap_client_common.h"
#include "cap_client_util.h"
#include "cap_client_add_new_dev.h"
#include "cap_client_debug.h"

/* SDU interval in Micro seconds*/

#define CAP_SDU_INTERVAL_7500US    0x00001D4C
#define CAP_SDU_INTERVAL_10000US   0x00002710
#define CAP_SDU_INTERVAL_5000US    0x00001388
#define CAP_SDU_INTERVAL_6250US    0x0000186A
#define CAP_SDU_INTERVAL_15000US   0x00003A98


#define CAP_STREAM_CAPABILITY_MAX_LATENCY_100      100
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_95       95
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_85       85
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_80       80
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_75       75
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_31       31
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_24       24
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_20       20
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_15       15
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_10       10
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_8        8
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_60       60
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_45       45
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_50       50
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_65       65
#define CAP_STREAM_CAPABILITY_MAX_LATENCY_54       54


#define CAP_STREAM_CAPABILITY_RET_NUM_2            2
#define CAP_STREAM_CAPABILITY_RET_NUM_4            4
#define CAP_STREAM_CAPABILITY_RET_NUM_5            5
#define CAP_STREAM_CAPABILITY_RET_NUM_13           13
#define CAP_STREAM_CAPABILITY_RET_NUM_15           15

#define CAP_CLIENT_APTX_LITE_MAX_LATENCY_5_MS      5
#define CAP_CLIENT_APTX_LITE_MAX_LATENCY_7_MS      7
#define CAP_CLIENT_APTX_LITE_MAX_LATENCY_9_MS      9
#define CAP_CLIENT_APTX_LITE_MAX_LATENCY_12_MS     12

#define CAP_CLIENT_APTX_ADAPTIVE_MAX_LATENCY_59_MS 59

typedef struct
{
    CapClientContext   useCase;
    uint8              count;
    CapClientGroupInfo *gInfo;
} CapClientGroupInfoUseCase;

static uint8 capClientGetLc3FrameDurationFromCapability(CapClientSreamCapability config)
{
    uint8 frameDuration = 0x00;

    switch(config)
    {
         case CAP_CLIENT_STREAM_CAPABILITY_8_1:
         case CAP_CLIENT_STREAM_CAPABILITY_16_1:
         case CAP_CLIENT_STREAM_CAPABILITY_24_1:
         case CAP_CLIENT_STREAM_CAPABILITY_32_1:
         case CAP_CLIENT_STREAM_CAPABILITY_48_1:
         case CAP_CLIENT_STREAM_CAPABILITY_48_3:
         case CAP_CLIENT_STREAM_CAPABILITY_48_5:
         case CAP_CLIENT_STREAM_CAPABILITY_441_1:
         {
              frameDuration = BAP_SUPPORTED_FRAME_DURATION_7P5MS;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_8_2:
         case CAP_CLIENT_STREAM_CAPABILITY_16_2:
         case CAP_CLIENT_STREAM_CAPABILITY_24_2:
         case CAP_CLIENT_STREAM_CAPABILITY_32_2:
         case CAP_CLIENT_STREAM_CAPABILITY_48_2:
         case CAP_CLIENT_STREAM_CAPABILITY_48_4:
         case CAP_CLIENT_STREAM_CAPABILITY_48_6:
         case CAP_CLIENT_STREAM_CAPABILITY_441_2:
         {
             frameDuration = BAP_SUPPORTED_FRAME_DURATION_10MS;
         }
         break;

         default:
             CAP_CLIENT_INFO("\n(CAP): capClientGetLc3FrameDurationFromCapability: Unsupported Config\n");
             break;
    }

    return frameDuration;
}

static uint8 capClientGetAptxLiteFrameDurationFromCapability(CapClientSreamCapability config)
{
    CSR_UNUSED(config);
    return BAP_SUPPORTED_FRAME_DURATION_NONE;
}

static uint8 capClientGetAptxAdaptiveFrameDurationFromCapability(CapClientSreamCapability config)
{
    CSR_UNUSED(config);
    return BAP_SUPPORTED_FRAME_DURATION_NONE;
}

static uint32 capClientGetLc3SduIntervalForCapability(CapClientSreamCapability config)
{
    uint32 sduInterval = 0;

    switch(config)
    {
         case CAP_CLIENT_STREAM_CAPABILITY_8_1:
         case CAP_CLIENT_STREAM_CAPABILITY_16_1:
         case CAP_CLIENT_STREAM_CAPABILITY_24_1:
         case CAP_CLIENT_STREAM_CAPABILITY_32_1:
         case CAP_CLIENT_STREAM_CAPABILITY_48_1:
         case CAP_CLIENT_STREAM_CAPABILITY_48_3:
         case CAP_CLIENT_STREAM_CAPABILITY_48_5:
         case CAP_CLIENT_STREAM_CAPABILITY_441_1:
         {
             sduInterval = CAP_SDU_INTERVAL_7500US;
         }
         break;
         case CAP_CLIENT_STREAM_CAPABILITY_8_2:
         case CAP_CLIENT_STREAM_CAPABILITY_16_2:
         case CAP_CLIENT_STREAM_CAPABILITY_24_2:
         case CAP_CLIENT_STREAM_CAPABILITY_32_2:
         case CAP_CLIENT_STREAM_CAPABILITY_48_2:
         case CAP_CLIENT_STREAM_CAPABILITY_48_4:
         case CAP_CLIENT_STREAM_CAPABILITY_48_6:
         case CAP_CLIENT_STREAM_CAPABILITY_441_2:
         {
             sduInterval = CAP_SDU_INTERVAL_10000US;
         }
         break;

         default:
             CAP_CLIENT_INFO("\n(CAP): capClientGetLc3SduIntervalForCapability: Unsupported Config\n");
             break;
    }

    return sduInterval;
}

static uint32 capClientGetAptxLiteSduIntervalForCapability(CapClientSreamCapability config, bool isJointStereoInSink)
{
    uint32 sduInterval = 0;
    switch (config)
    {
        case CAP_CLIENT_STREAM_CAPABILITY_16_1:
        case CAP_CLIENT_STREAM_CAPABILITY_48_1:
        {
            if(isJointStereoInSink == TRUE)
                sduInterval = CAP_SDU_INTERVAL_5000US;
            else
                sduInterval = CAP_SDU_INTERVAL_6250US;
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetAptxLiteSduIntervalForCapability: Unsupported Config\n");
            break;
    }

    return sduInterval;
}

static uint32 capClientGetAptxAdaptiveSduIntervalForCapability(CapClientSreamCapability config, bool isJointStereoInSink)
{
    uint32 sduInterval = 0;

    switch (config)
    {
        case CAP_CLIENT_STREAM_CAPABILITY_48_1:
        case CAP_CLIENT_STREAM_CAPABILITY_96:
        {
            sduInterval = CAP_SDU_INTERVAL_15000US;
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetAptxAdaptiveSduIntervalForCapability: Unsupported Config\n");
            break;
    }

    /* Values are not defined for joint stereo config yet, so default EB values will be picked */
    CSR_UNUSED(isJointStereoInSink);
    return sduInterval;
}

static uint16 capClientGetLc3SduSizeFromCapability(CapClientSreamCapability config, 
                                                  CapClientCigConfigMode mode,
                                                  uint8 locCount,
                                                  bool isJointStereoInSink)
{
    CSR_UNUSED(locCount);

    uint16 sduSize = 0;

    switch(config)
    {
         case CAP_CLIENT_STREAM_CAPABILITY_8_1:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_26;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_8_2:
         case CAP_CLIENT_STREAM_CAPABILITY_16_1:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_30;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_16_2:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_40;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_24_1:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_45;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_24_2:
         case CAP_CLIENT_STREAM_CAPABILITY_32_1:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_60;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_32_2:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_80;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_1:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_75;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_2:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_100;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_3:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_90;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_4:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_120;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_5:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_117;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_6:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_155;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_441_1:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_97;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_441_2:
         {
             sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_130;
         }
         break;

         default:
             CAP_CLIENT_INFO("\n(CAP): capClientGetLc3SduSizeFromCapability: Unsupported Config\n");
         break;
    }

    /* SDU size for joint stereo is double that of Mono*/
    if (((mode & CAP_CLIENT_MODE_JOINT_STEREO) == CAP_CLIENT_MODE_JOINT_STEREO) && (isJointStereoInSink))
    {
        sduSize = sduSize << 1;
    }

    return sduSize;
}

static uint16 capClientGetAptxLiteSduSizeFromCapability(CapClientSreamCapability config,
                                                   CapClientCigConfigMode mode,
                                                   uint8 locCount,
                                                   bool isJointStereoInSink)
{
    uint16 sduSize = 0;

    if (((mode & CAP_CLIENT_MODE_JOINT_STEREO) == CAP_CLIENT_MODE_JOINT_STEREO)
               && (isJointStereoInSink == TRUE))
    {
        if (config == CAP_CLIENT_STREAM_CAPABILITY_48_1 && (locCount > 1))
            sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_205;
        else if(config == CAP_CLIENT_STREAM_CAPABILITY_16_1 && (locCount == 1))
            sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_40;
        else
            CAP_CLIENT_INFO("\n(CAP): capClientGetAptxLiteSduSizeFromCapability: Unsupported Config\n");
    }
    else if (config == CAP_CLIENT_STREAM_CAPABILITY_48_1)
    {
        sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_108;
    }
    else if (config == CAP_CLIENT_STREAM_CAPABILITY_16_1)
    {
        sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_50;
    }
    else
    {
        CAP_CLIENT_INFO("\n(CAP): capClientGetAptxLiteSduSizeFromCapability: Unsupported Config\n");
    }

    CAP_CLIENT_INFO("\n(CAP): capClientGetAptxLiteSduSizeFromCapability: sduSize =%x, locCount =%x, isJointStereoInSink = %x\n", sduSize, locCount, isJointStereoInSink);
    return sduSize;
}

static uint16 capClientGetAptxAdaptiveSduSizeFromCapability(CapClientSreamCapability config,
                                                       CapClientCigConfigMode mode,
                                                       uint8 locCount,
                                                       bool isJointStereoInSink)
{
    uint16 sduSize = CAP_CLIENT_STREAM_CAPABILITY_OCTETS_300;

    CSR_UNUSED(config);
    CSR_UNUSED(mode);
    CSR_UNUSED(locCount);
    CSR_UNUSED(isJointStereoInSink);

    return sduSize;
}

static uint8 capClientGetLc3MaxLatencyFromCapability(CapClientSreamCapability config,
                                                 CapClientTargetLatency highReliability)
{ 
    uint8 maxLatency = 0;
    bool highMaxLatency = (highReliability == CAP_CLIENT_TARGET_HIGH_RELIABILITY);

    switch(config)
    {
         case CAP_CLIENT_STREAM_CAPABILITY_8_1:
         case CAP_CLIENT_STREAM_CAPABILITY_16_1:
         case CAP_CLIENT_STREAM_CAPABILITY_24_1:
         case CAP_CLIENT_STREAM_CAPABILITY_32_1:
         {
             maxLatency = highMaxLatency ? 
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_75 :
                           CAP_STREAM_CAPABILITY_MAX_LATENCY_8;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_8_2:
         case CAP_CLIENT_STREAM_CAPABILITY_16_2:
         case CAP_CLIENT_STREAM_CAPABILITY_24_2:
         case CAP_CLIENT_STREAM_CAPABILITY_32_2:
         {
             maxLatency = highMaxLatency ? 
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_95 : 
                             CAP_STREAM_CAPABILITY_MAX_LATENCY_10;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_1:
         case CAP_CLIENT_STREAM_CAPABILITY_48_3:
         case CAP_CLIENT_STREAM_CAPABILITY_48_5:
         {
             maxLatency = highMaxLatency ? 
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_75 : 
                              CAP_STREAM_CAPABILITY_MAX_LATENCY_15;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_2:
         {
             maxLatency = highMaxLatency ? 
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_95 : 
                               CAP_STREAM_CAPABILITY_MAX_LATENCY_20;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_48_4:
         case CAP_CLIENT_STREAM_CAPABILITY_48_6:
         {
             maxLatency = highMaxLatency ? 
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_100 :
                                CAP_STREAM_CAPABILITY_MAX_LATENCY_20;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_441_1:
         {
             maxLatency = highMaxLatency ? 
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_80 :
                                  CAP_STREAM_CAPABILITY_MAX_LATENCY_24;
         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_441_2:
         {
             maxLatency = highMaxLatency ? 
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_85 : 
                                  CAP_STREAM_CAPABILITY_MAX_LATENCY_31;
         }
         break;

         default:
             CAP_CLIENT_INFO("\n(CAP): capClientGetLc3MaxLatencyFromCapability: Unsupported Config\n");
             break;
    }
    return maxLatency;
}

static uint8 capClientGetAptxLiteMaxLatencyFromCapability(CapClientSreamCapability config,
    bool isJointStereoInSink)
{
    uint8 maxLatency = 0;

    switch (config)
    {
        case CAP_CLIENT_STREAM_CAPABILITY_48_1:
        {
            maxLatency = isJointStereoInSink ? CAP_CLIENT_APTX_LITE_MAX_LATENCY_5_MS : CAP_CLIENT_APTX_LITE_MAX_LATENCY_7_MS;
        }
        break;
        case CAP_CLIENT_STREAM_CAPABILITY_16_1:
        {
            maxLatency = isJointStereoInSink ? CAP_CLIENT_APTX_LITE_MAX_LATENCY_9_MS : CAP_CLIENT_APTX_LITE_MAX_LATENCY_12_MS;
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetAptxLiteMaxLatencyFromCapability: Unsupported Config\n");
            break;
    }

    return maxLatency;
}

static uint8 capClientGetAptxAdaptiveMaxLatencyFromCapability(CapClientSreamCapability config,
    bool isJointStereoInSink)
{
    uint8 maxLatency = CAP_CLIENT_APTX_ADAPTIVE_MAX_LATENCY_59_MS;

    CSR_UNUSED(config);
    CSR_UNUSED(isJointStereoInSink);

    return maxLatency;
}

uint8 capClientGetCodecIdFromCapability(CapClientSreamCapability config)
{
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;
    uint8 codecId = 0;

    if (codec == CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC || codec == CAP_CLIENT_STREAM_CAPABILITY_UNKNOWN)
    {
        codecId = BAP_CODEC_ID_LC3;
    }
    else
    {
        codecId = BAP_CODEC_ID_VENDOR_DEFINED;
    }

    return codecId;
}

uint16 capClientGetVendorCodecIdFromCapability(CapClientSreamCapability config)
{
    uint16 vendorCodecId = 0;
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;

    switch (codec)
    {
        case CAP_CLIENT_STREAM_CAP_CODEC_NONE:
        case CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC:
        {
            vendorCodecId = 0;
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_LITE:
        {
            vendorCodecId = BAP_VS_CODEC_ID_APTX_LITE;
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE:
        {
            vendorCodecId = BAP_VS_CODEC_ID_APTX_ADAPTIVE;
        }
        break;

        default:
        {
            CAP_CLIENT_INFO("\n(CAP): capClientGetVendorCodecIdFromCapability: Unrecognised VS Codec ID\n");
        }
        break;
    }

    return vendorCodecId;
}

uint16 capClientGetCompanyIdFromCapability(CapClientSreamCapability config)
{
    uint16 companyId = 0;
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;

    switch (codec)
    {
        case CAP_CLIENT_STREAM_CAP_CODEC_NONE:
        case CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC:
        {
            companyId = 0;
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_LITE:
        case CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE:
        {
            companyId = BAP_COMPANY_ID_QUALCOMM;
        }
        break;

        default:
            break;
    }

    return companyId;
}

uint8 capClientGetFrameDurationFromCapability(CapClientSreamCapability config)
{
    uint8 frameDuration = BAP_SUPPORTED_FRAME_DURATION_10MS;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;

    switch (codec)
    {
        case CAP_CLIENT_STREAM_CAP_CODEC_NONE:
        case CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC:
        {
            frameDuration = capClientGetLc3FrameDurationFromCapability(cap);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_LITE:
        {
            frameDuration = capClientGetAptxLiteFrameDurationFromCapability(cap);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE:
        {
            frameDuration = capClientGetAptxAdaptiveFrameDurationFromCapability(cap);
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetFrameDurationFromCapability: Unsupported Codec\n");
            break;
    }

    return frameDuration;
}

uint32 capClientGetSduIntervalForCapability(CapClientSreamCapability config, bool isJointStereoInSink)
{
    uint32 sduInterval = 0;

    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;

    switch (codec)
    {
        case CAP_CLIENT_STREAM_CAP_CODEC_NONE:
        case CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC:
        {
            sduInterval = capClientGetLc3SduIntervalForCapability(cap);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_LITE:
        {
            sduInterval = capClientGetAptxLiteSduIntervalForCapability(cap, isJointStereoInSink);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE:
        {
            sduInterval = capClientGetAptxAdaptiveSduIntervalForCapability(cap, isJointStereoInSink);
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetSduIntervalForCapability: Unsupported Codec\n");
            break;
    }

    return sduInterval;
}


uint8 capClientGetFramingForCapability(CapClientSreamCapability config)
{
    uint8 framing = 0;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;

    if((cap == CAP_CLIENT_STREAM_CAPABILITY_441_2)
                  ||(cap == CAP_CLIENT_STREAM_CAPABILITY_441_1))
        framing = 1;

    return framing;
}

/* Recommended Retransmission Number - Since there is no recommended number as of now for Aptx Lite we keep numbers same as LC3 */

uint8 capClientGetRtnFromCapability(CapClientSreamCapability config,
                             CapClientTargetLatency highReliablity)
{
    uint8 rtn = 0;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;

    switch (codec)
    {
        case CAP_CLIENT_STREAM_CAP_CODEC_NONE:
        case CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC:
        case CAP_CLIENT_STREAM_CAP_VS_APTX_LITE:
        {
            switch (cap)
            {
                case CAP_CLIENT_STREAM_CAPABILITY_8_1:
                case CAP_CLIENT_STREAM_CAPABILITY_8_2:
                case CAP_CLIENT_STREAM_CAPABILITY_16_1:
                case CAP_CLIENT_STREAM_CAPABILITY_16_2:
                case CAP_CLIENT_STREAM_CAPABILITY_24_1:
                case CAP_CLIENT_STREAM_CAPABILITY_24_2:
                case CAP_CLIENT_STREAM_CAPABILITY_32_1:
                case CAP_CLIENT_STREAM_CAPABILITY_32_2:
                {
                    rtn = CAP_STREAM_CAPABILITY_RET_NUM_2;
                }
                break;
                case CAP_CLIENT_STREAM_CAPABILITY_48_1:
                case CAP_CLIENT_STREAM_CAPABILITY_48_2:
                case CAP_CLIENT_STREAM_CAPABILITY_48_3:
                case CAP_CLIENT_STREAM_CAPABILITY_48_4:
                case CAP_CLIENT_STREAM_CAPABILITY_48_5:
                case CAP_CLIENT_STREAM_CAPABILITY_48_6:
                case CAP_CLIENT_STREAM_CAPABILITY_441_1:
                case CAP_CLIENT_STREAM_CAPABILITY_441_2:
                {
                    rtn = CAP_STREAM_CAPABILITY_RET_NUM_5;
                }
                break;
            }
            rtn = (highReliablity == CAP_CLIENT_TARGET_HIGH_RELIABILITY) ?
                    CAP_STREAM_CAPABILITY_RET_NUM_13 : rtn;
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE:
        {
            rtn = CAP_STREAM_CAPABILITY_RET_NUM_15;
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetRtnFromCapability: Unsupported Codec\n");
        break;
    }
    return rtn;
}

/* Need to be confirmed from test team */
uint8 capClientGetRtnFromCapabilityBcast(CapClientSreamCapability config,
                                     CapClientTargetLatency highReliablity)
{
    CSR_UNUSED(highReliablity);

    uint8 rtn = 0;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;

    switch (cap)
    {
    case CAP_CLIENT_STREAM_CAPABILITY_8_1:
    case CAP_CLIENT_STREAM_CAPABILITY_8_2:
    case CAP_CLIENT_STREAM_CAPABILITY_16_1:
    case CAP_CLIENT_STREAM_CAPABILITY_16_2:
    case CAP_CLIENT_STREAM_CAPABILITY_24_1:
    case CAP_CLIENT_STREAM_CAPABILITY_24_2:
    case CAP_CLIENT_STREAM_CAPABILITY_32_1:
    case CAP_CLIENT_STREAM_CAPABILITY_32_2:
    {
        rtn = CAP_STREAM_CAPABILITY_RET_NUM_2;
    }
    break;
    case CAP_CLIENT_STREAM_CAPABILITY_48_1:
    case CAP_CLIENT_STREAM_CAPABILITY_48_2:
    case CAP_CLIENT_STREAM_CAPABILITY_48_3:
    case CAP_CLIENT_STREAM_CAPABILITY_48_4:
    case CAP_CLIENT_STREAM_CAPABILITY_48_5:
    case CAP_CLIENT_STREAM_CAPABILITY_48_6:
    case CAP_CLIENT_STREAM_CAPABILITY_441_1:
    case CAP_CLIENT_STREAM_CAPABILITY_441_2:
    {
        rtn = CAP_STREAM_CAPABILITY_RET_NUM_4;
    }
    break;
    }
    return rtn;
}




/* Octets Per Frame for given config */

uint16 capClientGetSduSizeFromCapability(CapClientSreamCapability config,
                                         CapClientCigConfigMode mode,
                                         uint8 locCount,
                                         bool isJointStereoInSink)
{
    uint16 sduSize = 0;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;
    
    switch (codec)
    {
        case CAP_CLIENT_STREAM_CAP_CODEC_NONE:
        case CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC:
        {
            sduSize = capClientGetLc3SduSizeFromCapability(cap, mode ,locCount, isJointStereoInSink);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_LITE:
        {
            sduSize = capClientGetAptxLiteSduSizeFromCapability(cap, mode, locCount, isJointStereoInSink);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE:
        {
            sduSize = capClientGetAptxAdaptiveSduSizeFromCapability(cap, mode, locCount, isJointStereoInSink);
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetSduSizeFromCapability: Unsupported Codec\n");
            break;
    }

    return sduSize;
}

/* Sampling frequency remains same for both LC3 and Aptx Lite*/
uint16 capClientGetSamplingFreqFromCapability(CapClientSreamCapability config)
{
    uint16 samplingFreq = BAP_SAMPLING_FREQUENCY_RFU;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;

    switch(cap)
    {
         case CAP_CLIENT_STREAM_CAPABILITY_8_1:
         case CAP_CLIENT_STREAM_CAPABILITY_8_2:
         {
             samplingFreq = BAP_SAMPLING_FREQUENCY_8kHz;
         }
         break;
         case CAP_CLIENT_STREAM_CAPABILITY_16_1:
         case CAP_CLIENT_STREAM_CAPABILITY_16_2:
         {
             samplingFreq = BAP_SAMPLING_FREQUENCY_16kHz;
         }
         break;
         case CAP_CLIENT_STREAM_CAPABILITY_24_1:
         case CAP_CLIENT_STREAM_CAPABILITY_24_2:
         {
             samplingFreq = BAP_SAMPLING_FREQUENCY_24kHz;
         }
         break;
         case CAP_CLIENT_STREAM_CAPABILITY_32_1:
         case CAP_CLIENT_STREAM_CAPABILITY_32_2:
         {
             samplingFreq = BAP_SAMPLING_FREQUENCY_32kHz;

         }
         break;
         case CAP_CLIENT_STREAM_CAPABILITY_48_1:
         case CAP_CLIENT_STREAM_CAPABILITY_48_3:
         case CAP_CLIENT_STREAM_CAPABILITY_48_5:
         case CAP_CLIENT_STREAM_CAPABILITY_48_2:
         case CAP_CLIENT_STREAM_CAPABILITY_48_4:
         case CAP_CLIENT_STREAM_CAPABILITY_48_6:
         {
             samplingFreq = BAP_SAMPLING_FREQUENCY_48kHz;
         }
         break;
         case CAP_CLIENT_STREAM_CAPABILITY_441_1:
         case CAP_CLIENT_STREAM_CAPABILITY_441_2:
         {
             samplingFreq = BAP_SAMPLING_FREQUENCY_44_1kHz;

         }
         break;

         case CAP_CLIENT_STREAM_CAPABILITY_96:
         {
             samplingFreq = BAP_SAMPLING_FREQUENCY_96kHz;
         }
         break;
    }
    return samplingFreq;
}

uint8 capClientGetMaxLatencyFromCapability(CapClientSreamCapability config,
                                   CapClientTargetLatency highReliability,
                                   bool isJointStereoInSink)
{
    uint8 maxLatency = 0;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;
    CapClientSreamCapability codec = config & CAP_CLIENT_CODEC_ID_MASK;

    switch (codec)
    {
        case CAP_CLIENT_STREAM_CAP_CODEC_NONE:
        case CAP_CLIENT_STREAM_CAPABILITY_LC3_EPC:
        {
            maxLatency = capClientGetLc3MaxLatencyFromCapability(cap, highReliability);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_LITE:
        {
            maxLatency = capClientGetAptxLiteMaxLatencyFromCapability(cap, isJointStereoInSink);
        }
        break;

        case CAP_CLIENT_STREAM_CAP_VS_APTX_ADAPTIVE:
        {
            maxLatency = capClientGetAptxAdaptiveMaxLatencyFromCapability(cap, isJointStereoInSink);
        }
        break;

        default:
            CAP_CLIENT_INFO("\n(CAP): capClientGetMaxLatencyFromCapability: Unsupported Codec\n");
            break;
    }
    return maxLatency;
}

/* Need to be confirmed from test team */
uint8 capClientGetMaxLatencyFromCapabilityBcast(CapClientSreamCapability config,
                                            CapClientTargetLatency highReliability)
{
    uint8 maxLatency = 0;
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;
    bool highMaxLatency = (highReliability == CAP_CLIENT_TARGET_HIGH_RELIABILITY);

    switch (cap)
    {
    case CAP_CLIENT_STREAM_CAPABILITY_8_1:
    case CAP_CLIENT_STREAM_CAPABILITY_16_1:
    case CAP_CLIENT_STREAM_CAPABILITY_24_1:
    case CAP_CLIENT_STREAM_CAPABILITY_32_1:
    {
        maxLatency = highMaxLatency ?
            CAP_STREAM_CAPABILITY_MAX_LATENCY_45 :
                 CAP_STREAM_CAPABILITY_MAX_LATENCY_8;
    }
    break;
    case CAP_CLIENT_STREAM_CAPABILITY_8_2:
    case CAP_CLIENT_STREAM_CAPABILITY_16_2:
    case CAP_CLIENT_STREAM_CAPABILITY_24_2:
    case CAP_CLIENT_STREAM_CAPABILITY_32_2:
    {
        maxLatency = highMaxLatency ?
            CAP_STREAM_CAPABILITY_MAX_LATENCY_60 :
                   CAP_STREAM_CAPABILITY_MAX_LATENCY_10;
    }
    break;
    case CAP_CLIENT_STREAM_CAPABILITY_48_1:
    case CAP_CLIENT_STREAM_CAPABILITY_48_3:
    case CAP_CLIENT_STREAM_CAPABILITY_48_5:
    {
        maxLatency = highMaxLatency ?
            CAP_STREAM_CAPABILITY_MAX_LATENCY_50 :
               CAP_STREAM_CAPABILITY_MAX_LATENCY_15;
    }
    break;
    case CAP_CLIENT_STREAM_CAPABILITY_48_2:
    case CAP_CLIENT_STREAM_CAPABILITY_48_4:
    case CAP_CLIENT_STREAM_CAPABILITY_48_6:
    {
        maxLatency = highMaxLatency ?
            CAP_STREAM_CAPABILITY_MAX_LATENCY_65 :
                       CAP_STREAM_CAPABILITY_MAX_LATENCY_20;
    }
    break;
    case CAP_CLIENT_STREAM_CAPABILITY_441_1:
    {
        maxLatency = highMaxLatency ?
            CAP_STREAM_CAPABILITY_MAX_LATENCY_54 :
            CAP_STREAM_CAPABILITY_MAX_LATENCY_24;
    }
    break;
    case CAP_CLIENT_STREAM_CAPABILITY_441_2:
    {
        maxLatency = highMaxLatency ?
            CAP_STREAM_CAPABILITY_MAX_LATENCY_60 :
            CAP_STREAM_CAPABILITY_MAX_LATENCY_31;
    }
    break;
    }

    return maxLatency;
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
#ifdef CAP_CLIENT_IOP_TEST_MODE
uint16 capClientGetSDUFromCapabilityForIOP(CapClientSreamCapability config, uint16 sdu, CapClientContext useCase)
{
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;

    switch (cap)
    {
        case CAP_CLIENT_STREAM_CAPABILITY_8_2:
        {
            sdu = 30;
        }
        break;

        case CAP_CLIENT_STREAM_CAPABILITY_16_2:
        {
            sdu = 40;
        }
        break;

        case CAP_CLIENT_STREAM_CAPABILITY_32_2:
        {
            sdu = 80;
        }
        break;

        case CAP_CLIENT_STREAM_CAPABILITY_48_2:
        {
            sdu = 100;
        }
        break;

        case CAP_CLIENT_STREAM_CAPABILITY_48_4:
        {
            sdu = 120;
        }
        break;

        case CAP_CLIENT_STREAM_CAPABILITY_48_6:
        {
            sdu = 155;
        }
        break;

        default:
            break;

    }

    return ((uint16)sdu);

}

uint16 capClientGetMTLFromCapabilityForIOP(CapClientSreamCapability config, uint16 mtl, CapClientContext useCase)
{
    CapClientSreamCapability cap = config & ~CAP_CLIENT_CODEC_ID_MASK;

    switch (cap)
    {
    case CAP_CLIENT_STREAM_CAPABILITY_8_2:
    {
        mtl = 27;
    }
    break;

    case CAP_CLIENT_STREAM_CAPABILITY_16_2:
    {
        mtl = 28;

        if (useCase == CAP_CLIENT_CONTEXT_TYPE_LIVE
            || useCase == CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)
            mtl = 17;
    }
    break;

    case CAP_CLIENT_STREAM_CAPABILITY_32_2:
    {
        mtl = 27;
    }
    break;

    case CAP_CLIENT_STREAM_CAPABILITY_48_6:
    {
        mtl = 85;
    }
    break;

    case CAP_CLIENT_STREAM_CAPABILITY_48_2:
    {
        mtl = 85;
    }
    break;

    case CAP_CLIENT_STREAM_CAPABILITY_48_4:
    {
        mtl = 84;
        if (useCase == CAP_CLIENT_CONTEXT_TYPE_LIVE
            || useCase == CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)
            mtl = 17;
    }
    break;

    case CAP_CLIENT_STREAM_CAPABILITY_16_1:
    case CAP_CLIENT_STREAM_CAPABILITY_48_1:
    {
        if(useCase == CAP_CLIENT_CONTEXT_TYPE_GAME_WITH_VBC)
            mtl = 11;
        else if (useCase == CAP_CLIENT_CONTEXT_TYPE_GAME)
            mtl = 12;
    }

    break;
    default:
        break;

    }
    return ((uint16)mtl);
}
#endif /* CAP_CLIENT_IOP_TEST_MODE */

static void capClientStopStreamResetAseState(BapAseElement* ase, CapClientContext useCase)
{
    if (ase && ase->inUse && ase->useCase == useCase)
    {
        ase->inUse = FALSE;
        ase->removeDatapath = FALSE;
        ase->cisHandle = 0;
        ase->state = BAP_ASE_STATE_CODEC_CONFIGURED;
        ase->datapathDirection = 0;
        ase->cig = NULL;
        ase->useCase = 0;
    }
}

void capClientSendActiveGroupChangeInd(ServiceHandle newGroupId, ServiceHandle previous, AppTask appTask)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientActiveGroupChangedInd);

    message->activeGroupId = newGroupId;
    message->previousGroupId = previous;

    CapClientMessageSend(appTask, CAP_CLIENT_ACTIVE_GROUP_CHANGED_IND, message);
}

CapClientGroupInstance* capClientSetNewActiveGroup(CAP_INST *inst,
                                            ServiceHandle newGroupId,
                                            bool discoveryComplete)
{
    CSR_UNUSED(discoveryComplete);
    CapClientGroupInstance *cap = NULL;
    CapClientHandleElem* handle = NULL;
    /* GroupId does not belong to Active Group */
    /* Search If the Group is one of the available ones*/


    handle = (CapClientHandleElem*)CAP_CLIENT_FIND_CAP_GROUPID(inst->capClientGroupList, newGroupId);

    if(handle == NULL)
    {
        /* Send Invalid Group ID message ERR to application*/
        inst->addNewDevice = FALSE;

        /* Nowhere to Send Error Response. Just Log the error and
         * return */
        /* CAP_DEBUG_LOG("\n NULL INSTANCE ERROR \n");*/
        return NULL;
    }

    /* If new group is same as active group then just return cap Instance */

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(inst->activeGroupId);

    if (cap == NULL)
    {
        return NULL;
    }


    if(inst->activeGroupId == newGroupId)
    {
        return cap;
    }

    cap = (CapClientGroupInstance*)CAP_CLIENT_GET_GROUP_INST_DATA(newGroupId);

    if (cap == NULL)
    {
        return NULL;
    }

    /* Send deInit Group Switch message Indication to the app */

    capClientSendActiveGroupChangeInd(newGroupId, inst->activeGroupId, inst->appTask);

    /* Switch to the new group */
    inst->activeGroupId = newGroupId;
    inst->appTask = cap->appTask;

    return cap;
}

void capClientBuildCodecConfigQuery(CapClientGroupInstance *cap,
                                  CapClientSreamCapability config,
                                  BapCodecConfiguration* codec,
                                  uint32 audioLocation,
                                  bool isSink,
                                  bool isJointStereoInSink)
{
    uint8 locCount = capClientNumOfBitSet(audioLocation);
    uint16 sduSize = 0;
    codec->audioChannelAllocation = audioLocation;
    codec->frameDuaration = capClientGetFrameDurationFromCapability(config);
    codec->lc3BlocksPerSdu = BAP_DEFAULT_LC3_BLOCKS_PER_SDU;
    codec->octetsPerFrame = capClientGetSduSizeFromCapability(config, cap->cigConfigMode, locCount, isJointStereoInSink);
    codec->samplingFrequency = capClientGetSamplingFreqFromCapability(config);

    /*Override default values with app supplied values*/
    codec->lc3BlocksPerSdu = (cap->activeCig->unicastParam.codecBlocksPerSdu == 0) ?
        codec->lc3BlocksPerSdu : cap->activeCig->unicastParam.codecBlocksPerSdu;

    if (isSink && (cap->activeCig->sinkConfig == config))
    {
        sduSize = cap->activeCig->unicastParam.sduSizeCtoP;
    }

    if (!isSink && (cap->activeCig->srcConfig == config))
    {
        sduSize = cap->activeCig->unicastParam.sduSizePtoC;
    }

    codec->octetsPerFrame = (sduSize == 0) ? codec->octetsPerFrame : sduSize;
    
#ifdef CAP_CLIENT_IOP_TEST_MODE
    if ((cap->cigConfigMode & CAP_CLIENT_IOP_TEST_CONFIG_MODE) == CAP_CLIENT_IOP_TEST_CONFIG_MODE)
    {
        codec->octetsPerFrame = capClientGetSDUFromCapabilityForIOP(config, codec->octetsPerFrame, cap->useCase);
    }
#endif
}

void *CapClientGetProfileAttributeHandles(ServiceHandle groupId, uint32 cid,  CapClientProfile clientProfile)
{
    CapClientGroupInstance *capClient;
    void* handleInfo = NULL;

    capClient = CAP_CLIENT_GET_GROUP_INST_DATA(groupId);

    if (capClient == NULL)
    {
        CAP_CLIENT_ERROR("capClientGetProfileAttributeHandles: CAP IS NULL!!");
        return NULL;
    }

    switch (clientProfile)
    {
        case CAP_CLIENT_PROFILE_PACS:
        case CAP_CLIENT_PROFILE_ASCS:
        case CAP_CLIENT_PROFILE_VCP:
        case CAP_CLIENT_PROFILE_CSIP:
#ifndef EXCLUDE_CSR_BT_MICP_MODULE
        case CAP_CLIENT_PROFILE_MICP:
#endif
        {
           handleInfo =  capClientGetHandlesInfo(cid, capClient, clientProfile);
        }
        break;

        default :
        {
            CAP_CLIENT_ERROR("Wrong profile is passed!!!\n");
        }
    }
    return handleInfo;
}

CapClientBool capClientManageError(BapInstElement* bap, uint8 bapListCount)
{
    /* Traverse baplist to get the bap error count */
    BapInstElement* tmp = bap;
    uint8 bapErrCount = 0;

    while (tmp)
    {
        if (tmp->recentStatus != CAP_CLIENT_RESULT_SUCCESS)
            bapErrCount++;

        tmp = tmp->next;
    }

    /* Check for the bapError count , if all the bap status is failure then send status as bap failure
     * And return else continue to setup data path in case of partial or complete success of bap */

    return (bapListCount == bapErrCount) ? TRUE : FALSE;
}

void capClientCapGetCigSpecificParams(CapClientCigElem* cig,
                                       CapClientGroupInstance* cap)
{
    cap->activeCig = cig;
    cap->requiredSrcs = cig->configuredSrcAses;
    cap->numOfSourceAses  = cig->configuredSrcAses;
    cap->requiredSinks = cig->configureSinkAses;
    cap->useCase  = cig->context;
    cap->cigDirection = cig->cigDirection;
    cap->totalCisCount = cig->cisHandleCount;
}

CapClientProfileMsgQueueElem* capClientGetNextMsgElem(CapClientGroupInstance* gInst)
{
    CapClientProfileMsgQueueElem* msgElem = NULL;
    CapClientBool empty = CAP_CLIENT_IS_MSG_QUEUE_EMPTY(gInst->capClientMsgQueue);

    /* Perform queue operations only if queue is not empty */
    if (!empty)
    {
        CAP_CLIENT_MSG_QUEUE_REMOVE(&gInst->capClientMsgQueue);

        msgElem = (CapClientProfileMsgQueueElem*)
                 CAP_CLIENT_MSG_QUEUE_GET_FRONT(&gInst->capClientMsgQueue);
    }

    return msgElem;
}

void capClientClearAllStreamVariables(BapInstElement* bap)
{
    bap->cisCount = 0;
    bap->rtn = 0;
    bap->serverSinkSourceStreamCount = 0;
    bap->datapathReqCount = 0;
    bap->asesInUse = 0;
    bap->releasedAses = 0;
}

void capClientStopStreamIterateAses(BapInstElement* bap, CapClientContext useCase)
{
    BapAseElement* ase = (BapAseElement*)bap->sinkAseList.first;

    for (; ase; ase = ase->next)
    {
        capClientStopStreamResetAseState(ase, useCase);
    }

    ase = (BapAseElement*)bap->sourceAseList.first;

    for (; ase; ase = ase->next)
    {
        capClientStopStreamResetAseState(ase, useCase);
    }

}


void capClientCopyMetadataToBap(bool isSink,
                                uint8 metadataLength, 
                                uint8* metadata,
                                BapInstElement *bap)
{
    if (metadataLength && metadata)
    {
        if (bap->vsMetadata == NULL)
        {
            bap->vsMetadata = (CapClientVsMetadata*)CsrPmemZalloc(sizeof(CapClientVsMetadata));
        }

        if (isSink)
        {
            if (bap->vsMetadata->sinkVsMetadataLen &&
                bap->vsMetadata->sinkVsMetadata)
            {
                CsrPmemFree(bap->vsMetadata->sinkVsMetadata);
                bap->vsMetadata->sinkVsMetadata = NULL;
            }

            bap->vsMetadata->sinkVsMetadataLen = metadataLength;
            bap->vsMetadata->sinkVsMetadata = (uint8*)CsrPmemZalloc(metadataLength);
            SynMemCpyS(bap->vsMetadata->sinkVsMetadata,
                metadataLength,
                metadata,
                metadataLength);
        }
        else
        {
            if (bap->vsMetadata->srcVsMetadataLen &&
                bap->vsMetadata->srcVsMetadata)
            {
                CsrPmemFree(bap->vsMetadata->srcVsMetadata);
                bap->vsMetadata->srcVsMetadata = NULL;
            }

            bap->vsMetadata->srcVsMetadataLen = metadataLength;
            bap->vsMetadata->srcVsMetadata = (uint8*)CsrPmemZalloc(metadataLength);
            SynMemCpyS(bap->vsMetadata->srcVsMetadata,
                metadataLength,
                metadata,
                metadataLength);
        }
    }
}

static void capClientFindCisIdInBapList(CsrCmnListElm_t* elem, void* value)
{
    BapAseElement* bElem = (BapAseElement *)elem;
    CapClientGroupInfoUseCase* gUseInfo = (CapClientGroupInfoUseCase *)value;

    CapClientGroupInfo *gInfo = gUseInfo->gInfo;

    uint8 count = 0;

    if (bElem->useCase == gUseInfo->useCase)
    {
        for (count = 0; count < gUseInfo->gInfo->idCount; count++)
        {
            if (gInfo->idInfo[count].cisId == 0xFF)
            {
                gInfo->idInfo[count].cisId = bElem->cisId;
                gInfo->idInfo[count].direction = bElem->datapathDirection;

                break;
            }
        }
    }
}


static void capClientFindCisId(CsrCmnListElm_t* elem, void* value)
{
    BapInstElement* bElem = (BapInstElement *)elem;
    CapClientGroupInfoUseCase* gUseInfo = (CapClientGroupInfoUseCase *)value;

    CsrCmnListIterate(&bElem->sinkAseList, capClientFindCisIdInBapList ,gUseInfo);
    CsrCmnListIterate(&bElem->sourceAseList, capClientFindCisIdInBapList ,gUseInfo);
}


static void capClientGetCisInfo(CapClientGroupInstance *capClient, CapClientGroupInfoUseCase *gUseInfo)
{
    CsrCmnListIterate(&capClient->bapList, capClientFindCisId, gUseInfo);
}

static void initialiseStreamInfo(CapClientGroupInfo *gInfo, uint8 count)
{
    for (int i = 0; i < count; i++)
    {
        gInfo->idInfo[i].cisId = 0xFF;
        gInfo->idInfo[i].direction = 0xFF;
    }
}

static void capClientFindCisIdCountBapList(CsrCmnListElm_t* elem, void* value)
{
    BapAseElement* bElem = (BapAseElement *)elem;
    CapClientGroupInfoUseCase *gUseInfo = (CapClientGroupInfoUseCase *)value;

    if (bElem->useCase == gUseInfo->useCase)
    {
        gUseInfo->count++;
    }
}

static void capClientFindCisIdCount(CsrCmnListElm_t* elem, void* value)
{
    BapInstElement* bElem = (BapInstElement *)elem;
    CapClientGroupInfoUseCase *gUseInfo = (CapClientGroupInfoUseCase *)value;

    CsrCmnListIterate(&bElem->sinkAseList, capClientFindCisIdCountBapList ,gUseInfo);
    CsrCmnListIterate(&bElem->sourceAseList, capClientFindCisIdCountBapList ,gUseInfo);
}

static void getTotalCisCount(CapClientGroupInstance *capClient, CapClientGroupInfoUseCase *gUseInfo)
{
    CsrCmnListIterate(&capClient->bapList, capClientFindCisIdCount, gUseInfo);
}

CapClientGroupInfo *CapClientGetStreamInfo(ServiceHandle groupId, uint8 id, uint8 flag)
{
    CapClientGroupInstance *capClient = NULL;
    CapClientGroupInfoUseCase *gUseInfo = NULL;
    CapClientGroupInfo *gInfo = NULL;
    uint8 count = 0;

    capClient = CAP_CLIENT_GET_GROUP_INST_DATA(groupId);

    if (capClient == NULL)
    {
        CAP_CLIENT_ERROR("CapClientGetStreamInfo: CAP IS NULL!!");
        return NULL;
    }

    switch (flag)
    {
        case CAP_CLIENT_CIG:
        {
            CapClientCigElem *cig = CAP_CLIENT_GET_CIG_FROM_CIGID(capClient->cigList, id);

            if (cig == NULL)
            {
                CAP_CLIENT_ERROR("CapClientGetStreamInfo: CIG IS NULL!!");
                return NULL;
            }

            gUseInfo = (CapClientGroupInfoUseCase*) CsrPmemZalloc(sizeof(CapClientGroupInfoUseCase));

            gInfo = (CapClientGroupInfo*) CsrPmemAlloc(sizeof(CapClientGroupInfo));

            gUseInfo->useCase = cig->context;
            gUseInfo->count = 0;
            gUseInfo->gInfo = gInfo;

            /* Get the total cis count for the use case corresponding to the given cig id */
            getTotalCisCount(capClient, gUseInfo);

            count = gUseInfo->count;

            gInfo->idInfo = (CapClientStreamInfo*) CsrPmemZalloc(sizeof(CapClientStreamInfo) * count);

            gInfo->idCount = count;

            initialiseStreamInfo(gInfo, count);

            capClientGetCisInfo(capClient, gUseInfo);
        }
        break;
        case CAP_CLIENT_BIG:
        {
            /* To-Do */
        }
        break;

        default:
        {
            CAP_CLIENT_ERROR("Wrong flag is passed!!!\n");
        }
    }

    if (gUseInfo)
    {
    	CsrPmemFree(gUseInfo);
    }

    return gInfo;
}

#ifndef EXCLUDE_CSR_BT_MICP_MODULE
void capClientMicpServiceReqSend(CapClientGroupInstance* cap,
                               CAP_INST *inst,
                               CapClientOptionalServices serviceId,
                               CapClientMicpReqSender reqSender)
{
    /* Check for respetive service id and perform the operation, this can be extended in future for the new
     * services as well */
    if (serviceId == CAP_CLIENT_OPTIONAL_SERVICE_MICP)
    {
        MicpInstElement* micp = NULL;

        if (cap->requestCid == 0)
        {
            micp = (MicpInstElement*)cap->micpList->first;
        }
        else
        {
            micp = (MicpInstElement*)CAP_CLIENT_GET_MICP_ELEM_FROM_CID(cap->micpList, cap->requestCid);
        }

        reqSender(micp, inst, cap->requestCid);
   }
}
#endif /* EXCLUDE_CSR_BT_MICP_MODULE */
#endif /* INSTALL_LEA_UNICAST_CLIENT */

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT
static void capClientBcastAsstReqSend(CapClientGroupInstance* cap,
                               CAP_INST* inst,
                               CapClientBcastAsstReqSender reqSender)
{
    BapInstElement* bap = NULL;

    if (cap->requestCid == 0)
    {
        bap = (BapInstElement*)cap->bapList.first;
    }
    else
    {
        bap = (BapInstElement*)CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cap->requestCid);
    }

    reqSender(bap, cap->requestCid, inst->profileTask);

}

void capClientSendBcastCommonCfmMsgSend(AppTask appTask,
                                        CAP_INST* inst,
                                        CapClientGroupInstance* cap,
                                        uint32 cid,
                                        CapClientResult result,
                                        CapClientPrim msgType)
{
    BapInstElement* bap = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstCommonMsg);
    message->groupId = inst->activeGroupId;
    message->result = result;
    message->statusLen = 0;
    message->status = NULL;

    if (cap && (result == CAP_CLIENT_RESULT_SUCCESS))
    {
        bap = (BapInstElement*)cap->bapList.first;
        message->statusLen = cap->bapList.count;

        if (cid)
        {
            bap = (BapInstElement*)
                CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, cid);
            message->statusLen = 1;
        }

        if (message->statusLen)
            message->status = (CapClientDeviceStatus*)
                     CsrPmemZalloc(sizeof(CapClientDeviceStatus) * message->statusLen);

        if (bap && message->status)
        {
            uint8 i = 0;

            do
            {
                message->status[i].cid = bap->cid;
                message->status[i].result = bap->recentStatus;

                if (bap->recentStatus != CAP_CLIENT_RESULT_SUCCESS)
                    result = bap->recentStatus;

                bap = bap->next;
                i++;
            } while (bap && cid == 0);
        }
        else
        {
            CsrPmemFree(message->status);
            message->status = NULL;
            message->statusLen = 0;
            message->result = CAP_CLIENT_RESULT_FAILURE_BAP_ERR;
        }
    }

    message->result = result;

    CapClientMessageSend(appTask, msgType, message);
}

void capClientSendSelectBcastAsstCommonCfmMsgSend(AppTask appTask,
                                                  CAP_INST* inst,
                                                  CapClientGroupInstance* cap,
                                                  uint8  infoCount,
                                                  CapClientDelegatorInfo* info,   
                                                  CapClientResult result,
                                                  CapClientPrim msgType)
{
    uint8 i;
    BapInstElement* bap = NULL;
    MAKE_CAP_CLIENT_MESSAGE(CapClientBcastAsstCommonMsg);
    message->groupId = inst->activeGroupId;
    message->statusLen = 0;
    message->status = NULL;

    if (cap && (result == CAP_CLIENT_RESULT_SUCCESS))
    {
        if (infoCount && info)
        {
            message->statusLen = infoCount;
            message->status = (CapClientDeviceStatus*)
                              CsrPmemZalloc(sizeof(CapClientDeviceStatus) * infoCount);

            for (i = 0; i < infoCount; i++)
            {
                bap = (BapInstElement*)
                         CAP_CLIENT_GET_BAP_ELEM_FROM_PHANDLE(cap->bapList, info[i].cid);
                message->status[i].cid = info[i].cid;
                message->status[i].result = bap->recentStatus;

                if (bap->recentStatus != CAP_CLIENT_RESULT_SUCCESS)
                    result = bap->recentStatus;
            }

        }
    }
    message->result = result;
    CapClientMessageSend(appTask, msgType, message);
}

void capClientSendBapBcastAsstReq(CapClientGroupInstance* cap,
                                 CAP_INST* inst,
                                 CapClientBcastAsstReqSender reqSender)
{
    CsipInstElement* csip = NULL;

    if (capClientIsGroupCoordinatedSet(cap)
           && !capClientIsGroupIntsanceLocked(cap))
    {
        csip = (CsipInstElement*)(cap->csipList.first);
        capClientSetCsisLockState(csip, &inst->csipRequestCount, TRUE);
    }
    else
    {
        CAP_CLIENT_INFO("\n capClientSendBapBroadcastAssistantStartScanReq: Set Locked or Not Coordinated Set! \n");
        capClientBcastAsstReqSend(cap, inst, reqSender);
    }
}

CapClientResult capClientBroadcastAssistantValidOperation(ServiceHandle groupId,
                                                         AppTask profileTask,
                                                         CAP_INST* inst,
                                                         CapClientGroupInstance* cap)
{
    CapClientResult result = CAP_CLIENT_RESULT_SUCCESS;
    CapClientProfileTaskListElem* task = NULL;


    if (groupId != inst->activeGroupId)
    {
        result = CAP_CLIENT_RESULT_INVALID_OPERATION;
    }

    if (cap == NULL)
    {
        result = CAP_CLIENT_RESULT_NULL_INSTANCE;
    }
    else
    {

        task = (CapClientProfileTaskListElem*)
            CAP_CLIENT_GET_TASK_ELEM_FROM_APPHANDLE(&cap->profileTaskList, profileTask);

        if (task == NULL)
        {
            /* Reject the api call if the call is not from registered app*/
            result = CAP_CLIENT_RESULT_TASK_NOT_REGISTERED;
        }
    }

    return result;
}

#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

void capClientSendSetParamCfm(AppTask appTask, CapClientResult result, uint32 profileHandle)
{
    MAKE_CAP_CLIENT_MESSAGE(CapClientSetParamCfm);
    message->result = result;
    message->profileHandle = profileHandle;

    CapClientMessageSend(appTask, CAP_CLIENT_SET_PARAM_CFM, message);
}
