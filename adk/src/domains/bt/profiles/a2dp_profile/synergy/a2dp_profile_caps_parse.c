/****************************************************************************
Copyright (c) 2022-2023 Qualcomm Technologies International, Ltd.


FILE NAME
    a2dp_profile_caps_parse.c

DESCRIPTION
    This file contains the functionality to parse and process AVDTP Service
    Compatibility lists

NOTES

*/
/*lint -e655 */

/****************************************************************************
    Header files
*/

#include "a2dp_typedef.h"
#include "a2dp_profile_caps.h"
#include "a2dp_profile_caps_parse.h"
#include "a2dp_profile_codec_aptx_adaptive.h"

#include <stdlib.h>


/* Returns true if the bit mask passed in only has a single bit set */
static bool oneBitSet(unsigned bit_mask)
{
    return ((bit_mask != 0) && ((bit_mask & (bit_mask-1)) == 0));
}


/* Locates the start of the specified codec specific information, which may be embedded within the supplied codec caps */
bool appA2dpFindStdEmbeddedCodecCaps(const uint8 **codec_caps, uint8 embedded_codec)
{
    const uint8 *caps = *codec_caps;

    /* Expect non-A2DP codec caps */
    if (caps[0] == (uint8)CSR_BT_AV_SC_MEDIA_CODEC)
    {
        if (caps[3] == CSR_BT_AV_NON_A2DP_CODEC)
        {
            /* Find embedded codec caps and verify they are for the expected codec */
            /* Embedded codec caps are always 10 bytes from start of root codec caps */
            if (caps[10] == (uint8)CSR_BT_AV_SC_MEDIA_CODEC &&
                caps[13] == embedded_codec)
            {
                *codec_caps += 10;
                return TRUE;
            }
        }
        else if (caps[3]==embedded_codec)
        {   /* codec_caps already points to the correct capabilities */
            return TRUE;
        }
    }

    return FALSE;
}

/* Locates the start of the specified codec specific information, which may be embedded within the supplied codec caps */
bool a2dpFindNonStdEmbeddedCodecCaps(const uint8 **codec_caps, uint32 embedded_vendor, uint16 embedded_codec)
{
    const uint8 *caps = *codec_caps;

    /* Expect non-A2DP codec caps */
    if (caps[0] == (uint8)CSR_BT_AV_SC_MEDIA_CODEC)
    {
        if (caps[3] == CSR_BT_AV_NON_A2DP_CODEC)
        {
            /* Find embedded codec caps and verify they are for the expected codec.
             * Embedded codec caps are always 10 bytes from start of root codec caps */
            if ((caps[10]==(uint8)CSR_BT_AV_SC_MEDIA_CODEC && caps[13]==CSR_BT_AV_NON_A2DP_CODEC) &&
                (appA2dpConvertUint8ToUint32(&caps[14])==embedded_vendor && appA2dpConvertUint8ToUint16(&caps[18])==embedded_codec))
            {
                *codec_caps += 10;
                return TRUE;
            }
            else if (appA2dpConvertUint8ToUint32(&caps[4])==embedded_vendor && appA2dpConvertUint8ToUint16(&caps[8])==embedded_codec)
            {   /* codec_caps already points to the correct capabilities */
                return TRUE;
            }
        }
    }
    
    return FALSE;
}


/*
    Determines if SBC Codecs from both devices are compatible.

    SBC Codec Specific Information
        Octet 0 Bits 4-7    Sampling Frequency
                Bits 0-4    Channel Mode
        Octet 1 Bits 4-7    Block Length
                Bits 2-3    Subbands
                Bite 0-1    Allocation Method
        Octet 2             Minimum Bitpool Value
        Octet 3             Maximum Bitpool Value
*/
static bool areSBCCodecsCompatible(const uint8 *local_caps, 
                                   const uint8 *remote_caps,
                                   uint8 local_losc,
                                   uint8 remote_losc,
                                   bool initiating,
                                   uint8 *error_code)
{
    /* check length so we don't read off end of buffer */
    if (local_losc < 6)
        return FALSE;

    if (remote_losc < 6)
        return FALSE;
    /* do sampling frequency bits overlap? */
    if ( !((local_caps[0] >> 4) & (remote_caps[0] >> 4)))
    {
        /* Sampling Frequency is not supported */
        *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_SAMPLING_FREQ;
        return FALSE;
    }
    /* make sure only a single bit is set for the sampling frequency */
    if (!oneBitSet(remote_caps[0] & 0xf0) && !initiating)
    {
        /* Sampling Frequency is not valid or multiple values have been selected */
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_SAMPLING_FREQ;
        return FALSE;
    }
    /* do channel mode bits overlap? */
    if ( !((local_caps[0] & 15) & (remote_caps[0] & 15)))
    {
        /*Channel Mode is not supported */
        *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_CHANNEL_MODE;
        return FALSE;
    }
    /* make sure only a single bit is set for the channel mode */
    if (!oneBitSet(remote_caps[0] & 0xf) && !initiating)
    {
        
        /*Channel Mode is not valid or multiple values have been selected*/
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_CHANNEL_MODE;
        return FALSE;
    }
    /* do Block Length bits overlap? */
    if ( !((local_caps[1] >> 4) & (remote_caps[1] >> 4)))
    {
        /*None of the values have been selected for Block Length*/
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_BLOCK_LENGTH;
        return FALSE;
    }
    /* make sure only a single bit is set in the block length mask */
    if (!oneBitSet(remote_caps[1] & 0xf0) && !initiating)
    {
        /*Multiple values have been selected for Block Length*/
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_BLOCK_LENGTH;
        return FALSE;
    }
    /* do Subbands bits overlap? */
    if ( !(((local_caps[1] >> 2) & 3) & ((remote_caps[1] >> 2) & 3)))
    {
        /*Subbands Mode is not supported */
        *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_SUBBANDS;
        return FALSE;
    }
    /* make sure only a single bit is set in the subbands mask */
    if (!oneBitSet(remote_caps[1] & 0xc) && !initiating)
    {
        /* Multiple values have been selected for Subbands*/
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_SUBBANDS;
        return FALSE;
    }
    /* do Allocation Method bits overlap? */
    if ( !((local_caps[1] & 3) & (remote_caps[1] & 3)))
    {
        /*Allocation Method is not supported */
        *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_ALLOC_METHOD;
        return FALSE;
    }
    /* make sure only a single bit is set in the allocation method mask */
    if (!oneBitSet((remote_caps[1] & 0x3)) && !initiating)
    {
        /* make sure only a single bit is set in the ALLOC mask */
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_ALLOC_METHOD;
        return FALSE;
    }
    /* check Min/Max Bitpool Values are in range */
    if (initiating)
    {
        /* local min is greater than remote max */
        if (local_caps[2] > remote_caps[3])
        {
            /*Minimum Bitpool Value is not valid*/
            *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_MIN_BITPOOL;
            return FALSE;
        }
        /* local max is less than remote min */
        if (remote_caps[2] > local_caps[3])
        {
            /*Maximum Bitpool Value is not valid*/
            *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_MAX_BITPOOL;
            return FALSE;
        }

    }
    else
    {
        /* remote max greater than local max */
        if (remote_caps[3] > local_caps[3])
        {
            /*Maximum Bitpool Value is not valid*/
            *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_MAX_BITPOOL;
            return FALSE;
        }
        /* remote min less than local min */
        if (remote_caps[2] < local_caps[2])
        {
            /*Minimum Bitpool Value is not valid*/
            *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_MIN_BITPOOL;
            return FALSE;
        }

    }
    /* match */
    return TRUE;
}

#ifndef A2DP_SBC_ONLY
/*
    Determines if MPEG-2,4 AAC codecs from both devices are compatible.

    Codec Specific Information

        Octet0              Object Type
        Octet1              Sampling Frequency
        Octet2  Bits 4-7    Sampling Frequency
                Bits 2-3    Channels
                Bits 0-1    RFA
        Octet3  Bits 7      VBR
                Bits 0-6    Bit rate
        Octet4              Bit rate
        Octet5              Bit rate
*/
static bool areMPEG24AACCodecsCompatible(const uint8 *local_caps,const uint8 *remote_caps,
                                        uint8 local_losc, uint8 remote_losc,
                                        bool initiating,uint8 *error_code)
{
    unsigned local_frequency;
    unsigned remote_frequency;

    /* check length so we don't read off end of buffer */
    if (local_losc < 8)
        return FALSE;

    if (remote_losc < 8)
        return FALSE;

#ifndef DISABLE_A2DP_1P4_ERROR_CODE    
    if ((remote_caps[0] & AAC_MPEG2_AAC_LC) &&
         (remote_caps[0] & AAC_MPEGD_DRC))
    {
        /* Combination of ObjectType and DRC is invalid */
        *error_code = RESULT_CODE_A2DP_INVALID_DRC;
        return FALSE;
    }
    
    /* make sure DRC bit in octet 0 is not set */
    if (remote_caps[0] & AAC_MPEGD_DRC)
    {
        /* Snk does not support for DRC yet */
        *error_code = RESULT_CODE_A2DP_NOT_SUPPORTED_DRC;
        return FALSE;
    }
#endif

    /* make sure only a single bit is set in the object type mask */
    if (!oneBitSet(remote_caps[0]) && !initiating)
    {
        /* make sure only a single bit is set in the Object type mask */
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_OBJECT_TYPE;
        return FALSE;
    }

    /* Object Type bits overlap? */
    if (!(local_caps[0] & remote_caps[0]))
    {
        /*Object Type is not supported */
        *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_OBJECT_TYPE;
        return FALSE;
    }

    /* Sampling Frequency bits overlap? */
    local_frequency = local_caps[1] | ((local_caps[2] & 0xf0) << 4);
    remote_frequency = remote_caps[1] | ((remote_caps[2] & 0xf0) << 4);
    if (!(local_frequency & remote_frequency))
    {
        /*Sampling Frequency not supported */
        *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_SAMPLING_FREQ;
        return FALSE;
    }

    /* make sure only a single bit is set in the sampling frequency mask */
    if (!oneBitSet(remote_frequency) && !initiating)
    {
        /* make sure only a single bit is set in the Sampling freq mask */
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_SAMPLING_FREQ;
        return FALSE;
    }

    /* Channels bits overlap? */
    if ( !((local_caps[2] & 0x0c) & (remote_caps[2] & 0x0c)))
    {
        /*Channels not supported */
        *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_CHANNELS;
        return FALSE;
    }

    /* make sure only a single bit is set in the channels mask */
    if (!oneBitSet((remote_caps[2] & 0x0c)) && !initiating)
    {
        /* make sure only a single bit is set in the Channel mask */
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_CHANNELS;
        return FALSE;
    }

    /* VBR can be either value, so ignore */
    /* Bit Rates are just max values so ignore */

    /* match */
    return TRUE;
}
/* Determine if aptX classic, LL and HD codecs are compatible. */
static bool areAptxCodecsCompatible(const uint8 *local_caps, const uint8 *remote_caps, uint8 local_losc, uint8 remote_losc,
                                    uint8 channel_mode_mask, uint8 sample_rate_mask)
{
    /* check length to prevent read off end of buffer */
    if ((local_losc < 7) || (remote_losc < 7))
        return FALSE;

    /* Are Channel Modes compatible? */
    if ((local_caps[6] & remote_caps[6] & channel_mode_mask) == 0)
        return FALSE;

    /* Are Sample Rates compatible? */
    if ((local_caps[6] & remote_caps[6] & sample_rate_mask) == 0)
        return FALSE;

    return TRUE;
}


/*
    Determines if Vendor Codecs from both devices are compatible.

    Vendor Specific Codec
       Octet 0-3 VendorID
       Octet 4,5 CodecID
       Octet 5-n values.
*/
static bool areVendorCodecsCompatible(const uint8 *local_caps, const uint8 *remote_caps,
                                     uint8 local_losc, uint8 remote_losc)
{
    uint32      local_vendor_id;
    uint32      remote_vendor_id;
    unsigned    local_codec_id;
    unsigned    remote_codec_id;

    /* check length so we don't read off end of buffer.
       Called functions check for greater length if needed */
    if (local_losc < 6)
        return FALSE;

    if (remote_losc < 6)
        return FALSE;

    /* extract Vendor and Codec IDs */
    local_vendor_id = appA2dpConvertUint8ToUint32(local_caps);
    remote_vendor_id = appA2dpConvertUint8ToUint32(remote_caps);
    local_codec_id = (local_caps[4] << 8) | local_caps[5];
    remote_codec_id = (remote_caps[4] << 8) | remote_caps[5];

    /* Check codecs are the same */
    if ((local_vendor_id != remote_vendor_id) ||
        (local_codec_id != remote_codec_id))
        return FALSE;

    switch (remote_vendor_id)
    {
        case A2DP_APT_VENDOR_ID:
            /* APT-X Vendor ID */
            switch (remote_codec_id)
            {
                case A2DP_CSR_APTX_CODEC_ID:
                    /* APT-X Standard Codec */
                    return areAptxCodecsCompatible(local_caps, remote_caps, local_losc, remote_losc, 0x02, 0xF0);
            }
            break;

        case A2DP_QTI_VENDOR_ID:
            /* QTI Vendor ID */
            switch (remote_codec_id)
            {
                case A2DP_QTI_APTX_AD_CODEC_ID:
                    return appA2dpAreAptxAdCodecsCompatible(local_caps, remote_caps, local_losc, remote_losc);

                case A2DP_QTI_APTXHD_CODEC_ID:
                    return areAptxCodecsCompatible(local_caps, remote_caps, local_losc, remote_losc, 0x02, 0xF0);
            }
            break;

        default:
            break;
    }

    /* Unknown Codec.
       As the Vendor and Codec IDs match, assume that the application knows
       how to handle this.
    */
    return TRUE;
}
#endif

/****************************************************************************/
uint32 appA2dpConvertUint8ToUint32(const uint8 *ptr)
{
    return (((uint32)ptr[0] << 24) | ((uint32)ptr[1] << 16) | ((uint32)ptr[2] << 8) | (uint32)ptr[3]);
}

/****************************************************************************/
uint16 appA2dpConvertUint8ToUint16(const uint8 *ptr)
{
    return (uint16)(((uint16)ptr[0] << 8) | (uint16)ptr[1]);
}


/****************************************************************************
NAME
    a2dpFindMatchingCodecSpecificInformation

DESCRIPTION
    Returns pointer to start of codec specific information if
    the local and remote codecs are compatible.

    IMPORTANT: It assumes the basic structure of the caps is valid. Call
    gavdpValidateServiceCaps() first to make sure.

RETURNS
    void
*/
const uint8* appA2dpFindMatchingCodecSpecificInformation(const uint8 *local_caps,
                                                         const uint8 *remote_caps,
                                                         bool initiating,
                                                         uint8 *error_code)
{
    const uint8 *local_codec = local_caps;
    const uint8 *remote_codec = remote_caps;

    /*Initializing error code to default unsupported configuration*/
    *error_code = CSR_BT_RESULT_CODE_A2DP_UNSUPPORTED_CONFIGURATION;

    /* find the codec specific info in both caps */
    if (!appA2dpFindCodecSpecificInformation(&local_codec, NULL))
        return NULL;

    if (!appA2dpFindCodecSpecificInformation(&remote_codec, NULL))
        return NULL;

    /* check they are the same type */
    if ((local_codec[2] == remote_codec[2]) && /* media type */
        (local_codec[3] == remote_codec[3]))   /* media codec */
    {
        /* we have a matching codec, now check the fields */
        if (local_codec[2] == (CSR_BT_AV_AUDIO<<2))
        {
            switch (local_codec[3])
            {
                case CSR_BT_AV_SBC:
                    /* check SBC codecs are compatible */
                    if (areSBCCodecsCompatible(local_codec+4, remote_codec+4, local_codec[1], remote_codec[1], initiating,error_code))
                        return remote_codec;
                    break;

#ifndef A2DP_SBC_ONLY
                case CSR_BT_AV_MPEG24_AAC:
                    /* check MPEG-2,4 AAC codecs are compatible. */
                    if (areMPEG24AACCodecsCompatible(local_codec+4,remote_codec+4,local_codec[1],remote_codec[1], initiating,error_code))
                        return remote_codec;
                    break;

                case CSR_BT_AV_NON_A2DP_CODEC:
                    /* check non-a2dp codecs are compatible */
                    if (areVendorCodecsCompatible(local_codec+4,remote_codec+4,local_codec[1],remote_codec[1]))
                        return remote_codec;
                    break;
#endif

                default:
                    /* unknown - default to accepting new codecs */
                    return remote_codec;
            }
        }
        else
        {
            /* unknown - default to accepting new codecs */
            return remote_codec;
        }
    }
    else if ((remote_codec[3] >=0  && remote_codec[3] <=4) || (remote_codec[3] == 0xFF))
    {
        /* Valid codec Type */
        if (remote_codec[3] != local_codec[3])
        {
            *error_code = CSR_BT_RESULT_CODE_A2DP_NOT_SUPPORTED_CODEC_TYPE;
        }
        /* else: when coedec type is matching but not the media type,the error code would
         * be CSR_BT_RESULT_CODE_A2DP_UNSUPPORTED_CONFIGURATION */
    }
    else
    {
        *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_CODEC_TYPE;
    }
    return NULL;
}

/****************************************************************************
NAME
    appA2dpValidateServiceCaps

DESCRIPTION
    Attempts to validate that a support Service Capabilities list can
    be parsed and contains reasonable values.

    This function should allow all valid values, even if the local hardware/software
    does not support them.

    It is also used to validate the caps returned by the remote device, so should not
    be dependent on local settings.

    The 'reconfigure' flag is used to adjust the validation rules depending on if
    the Capabilities supplied are complete, or part of a reconfiguration.

    When 'only_check_structure' is TRUE this function only tests that the structure
    is correct, it does not verify if mandatory entries are present.

    When 'ignore_bad_serv_category' is TRUE this function does not return an error for
    service categories that are out of spec.

    When the function returns FALSE, it will write the service category with the error
    and the AVDTP error code to error_category and error_code parameters.

RETURNS
    void
*/
bool appA2dpValidateServiceCaps(const uint8 *caps, uint16 caps_size,
                                bool reconfigure, bool only_check_structure,
                                bool ignore_bad_serv_category, uint8 *error_category,
                                uint8 *error_code)
{
    bool has_media_transport = FALSE;
    bool has_codec = FALSE;

    do
    {
        uint8 service;
        uint8 losc;

        /* each entry must contain at least two bytes; Service Category and LOSC. */
        if (caps_size < 2)
        {
            *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_PAYLOAD_FORMAT;
            return FALSE;
        }

        /* read header */
        service = *(caps++);
        losc = *(caps++);
        caps_size-=2;

        /* keep the error current */
        *error_category = service;

        /* is there enough space to contain the declared LOSC */
        if (losc > caps_size)
        {
            *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_PAYLOAD_FORMAT;
            return FALSE;
        }

        /*
            Perform some checks on the entries
        */
        switch (service)
        {
            case (uint8)CSR_BT_AV_SC_MEDIA_TRANSPORT:
                has_media_transport = TRUE;
                if (losc != 0)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_MEDIA_TRANSPORT_FORMAT;
                    return FALSE;
                }
                break;

            case (uint8)CSR_BT_AV_SC_REPORTING:
                /* Reporting losc is always 0. (AVDTP 8.21.3) */
                if (losc != 0)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_PAYLOAD_FORMAT;
                    return FALSE;
                }
                break;

            case (uint8)CSR_BT_AV_SC_RECOVERY:
                /* Check caps match those defined in spec (AVDTP 8.21.4) */
                if ((losc != 3) || (caps_size < 3))
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_RECOVERY_FORMAT;
                    return FALSE;
                }
                /* Recovery Type: 1 is only valid value */
                if (caps[0] != 1)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_RECOVERY_TYPE;
                    return FALSE;
                }
                /* MRWS: check range */
                if ((caps[1] < 0x01) || (caps[1] > 0x18))
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_RECOVERY_FORMAT;
                    return FALSE;
                }
                /* MNMP: check range */
                if ((caps[2] < 0x01) || (caps[2] > 0x18))
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_RECOVERY_FORMAT;
                    return FALSE;
                }
                break;

            case (uint8)CSR_BT_AV_SC_CONTENT_PROTECTION:
                /* content protection must at least containt 16bit Type */
                if (losc < 2)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_CP_FORMAT;
                    return FALSE;
                }
                break;

            case (uint8)CSR_BT_AV_SC_HDR_COMPRESSION:
                if (losc != 1)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_ROHC_FORMAT;
                    return FALSE;
                }
                break;

            case (uint8)CSR_BT_AV_SC_MULTIPLEXING:
                /* As per spec (AVDTP 8.21.8), if the acceptor supports multiplexing, then losc should be have media transport entry */
                if (losc < 2)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_MULTIPLEXING_FORMAT;
                    return FALSE;
                }
                break;

            case (uint8)CSR_BT_AV_SC_MEDIA_CODEC:
                /* Actual codec parameters are validated during configuration
                   in findMatchingCodecSpecificInformation(). We just check
                   the structure here. */
                if (losc < 2)
                {
                    /* must contain a media type and codec type. */
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_PAYLOAD_FORMAT;
                    return FALSE;
                }

                if (has_codec)
                {   /* Duplicate codec castegory found in caps */
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_PAYLOAD_FORMAT;
                    return FALSE;
                }
                
                /* Codec category found in caps */
                has_codec = TRUE;

                break;

            case (uint8)CSR_BT_AV_SC_DELAY_REPORTING:
                /* Delay Reporting losc is always 0. (AVDTP 8.21.9) */
                if (losc != 0)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_PAYLOAD_FORMAT;
                    return FALSE;
                }
                break;
                
            default:
                if (!ignore_bad_serv_category)
                {
                    *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_SERV_CATEGORY;
                    return FALSE;
                }
                break;
        }

        /* move to next entry (losc validated above) */
        caps += losc;
        caps_size -= losc;
    }
    while (caps_size != 0);

    if (!only_check_structure)
    {
        /* check that the media transport is present when not reconfiguring, and not present when reconfiguring. */
        if ((!has_media_transport && !reconfigure) || (has_media_transport && reconfigure))
        {
            *error_category = (uint8)CSR_BT_AV_SC_MEDIA_TRANSPORT;
            *error_code = CSR_BT_RESULT_CODE_A2DP_INVALID_CAPABILITIES;
            return FALSE;
        }

        /* check that there is a codec present. */
        if (!has_codec)
        {
            *error_category = (uint8)CSR_BT_AV_SC_MEDIA_CODEC;
            *error_code = CSR_BT_RESULT_CODE_A2DP_BAD_PAYLOAD_FORMAT;
            return FALSE;
        }
    }

    /* go to the end without any errors. */
    return TRUE;
}

/****************************************************************************
NAME
    a2dpFindCodecSpecificInformation

DESCRIPTION
    Finds the next codec block in a list of caps.
    Passed pointer and size are updated to point to the search result.
    IMPORTANT: It assumes the basic structure of the caps is valid. Call
    gavdpValidateServiceCaps() first to make sure.

RETURNS
    void
*/
bool appA2dpFindCodecSpecificInformation(const uint8 **caps, uint16 *caps_size)
{
    if (*caps == 0)
        return FALSE;

    if (caps_size != NULL)
    {
        while (*caps_size != 0)
        {
            uint8 service = (*caps)[0];
            uint8 losc = (*caps)[1];

            if (service == (uint8)CSR_BT_AV_SC_MEDIA_CODEC)
                return TRUE;

            /* Check size so we don't move to an invalid location (should
               only occur if caps are malformed) */
            if ((2 + losc) > *caps_size)
                return FALSE;         

            /* move to next entry */
            *caps += 2 + losc;
            *caps_size -= 2 + losc;
        }
        return FALSE;
    }
    else
    {
        while ((*caps)[0] != (uint8)CSR_BT_AV_SC_MEDIA_CODEC)
            *caps += 2 + (*caps)[1];

        return TRUE;
    }
}

/****************************************************************************
NAME
    appA2dpAreServicesCategoriesCompatible

DESCRIPTION
    Checks the Services requested in a SET_CONFIG or RECONFIGURE command
    are supported by the local SEP.  It only checks for the Service entry
    and DOES NOT validate the actual service capabilities - that should
    be done by other functions e.g. gavdpFindMatchingCodecSpecificInformation

    IMPORTANT: It assumes the basic structure of the caps is valid. Call
    gavdpValidateServiceCaps() first to make sure.

RETURNS
    TRUE if OK, FALSE is Configuration contains entry not in local caps.
*/
bool appA2dpAreServicesCategoriesCompatible(const uint8 *local_caps, uint16 local_caps_size,
                                        const uint8 *config_caps, uint16 config_caps_size,
                                        uint8 *unsupported_service)
{
    uint16 i,j;

    /* loop through configuration */
    for(i=0;i<config_caps_size;i+=config_caps[i+1]+2)
    {
        uint8 service = config_caps[i];
        bool match = FALSE;

        /* check entry is in local caps */
        for(j=0;!match && j<local_caps_size;j+=local_caps[j+1]+2)
        {
            /* compare local service to caps */
            if (local_caps[j] == service)
                match = TRUE;
        }

        /* didn't find Service in local caps, fail */
        if (!match)
        {
            /* report unsupported cap */
            *unsupported_service = service;
            return FALSE;
        }
    }

    return TRUE;
}

a2dpContentProtection appA2dpGetContentProtection(const uint8 *ptr, const uint16 size_ptr,
                                                  const uint8 **returned_caps)
{
    uint16 size_caps = size_ptr;
    const uint8 *caps = ptr;

    while (size_caps != 0)
    {
        uint8 service = caps[0];
        uint8 losc = caps[1];

        if (service == (uint8)CSR_BT_AV_SC_CONTENT_PROTECTION)
        {
            if (losc >= 2)
                if ((caps[2] == A2DP_CP_TYPE_SCMS_LSB) && (caps[3] == A2DP_CP_TYPE_SCMS_MSB))
                {
                    if (returned_caps)
                    {
                        *returned_caps = caps;
                    }
                    return A2DP_SCMS_CONTENT_PROTECTION;
                }
        }

        /* Check size so we don't move to an invalid location (should only occur 
           if caps are malformed) */
        if ((2 + losc) > size_caps)
        {
            /* Invalid caps, return none */
            if (returned_caps)
            {
                *returned_caps = NULL;
            }
            return A2DP_NO_CONTENT_PROTECTION;
        }

        /* move to next entry */
        caps += 2 + losc;
        size_caps -= 2 + losc;
    }

    if (returned_caps)
    {
        *returned_caps = NULL;
    }
    return A2DP_NO_CONTENT_PROTECTION;
}

bool appA2dpIsServiceSupported (uint8 service, const uint8 *caps, uint16 size_caps)
{
    while (size_caps != 0)
    {
        if (service == caps[0])
        {
            return TRUE;
        }

        /* Check size so we don't move to an invalid location (should only 
           occur if caps are malformed) */
        if ((2 + caps[1]) > size_caps)
            return FALSE;

        /* move to next entry */
        size_caps -= 2 + caps[1];
        caps += 2 + caps[1];
    }

    return FALSE;
}
