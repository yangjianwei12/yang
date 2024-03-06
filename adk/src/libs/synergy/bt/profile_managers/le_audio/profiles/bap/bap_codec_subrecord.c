/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Codec subrecord - methods to operate on the BapCodecSubRecord type.
 */

#include <string.h>
#include "bap_utils.h"
#include "bap_codec_subrecord.h"


void bapCodecSubrecordSerialise(BapCodecSubRecord * const codecSubrecord,
                                BUFF_ITERATOR* buffIterator)
{
    bapCodecSubrecordSerialisePacRecordLength(codecSubrecord, buffIterator);
    bapCodecSubrecordSerialiseCodecId(codecSubrecord, buffIterator);
    bapCodecSubrecordSerialiseNumCodecSpecificParametersOctets(codecSubrecord, buffIterator);
    bapCodecSubrecordSerialiseCodecSpecificParameters(codecSubrecord, buffIterator);
}

void bapCodecSubrecordDeserialise(BapCodecSubRecord * const codecSubrecord,
                                  BUFF_ITERATOR* buffIterator)
{
    uint8 i;
    uint8 pacRecordLength;
    QblLtv ltv;
    uint8* ltvFormatData;

    /* codec subrecords consist of following fields

        Field                           Size(Octets)                        Notes
    ----------------------------------------------------------------------------------
    PAC_Record_Length[i]	1       	Length, in octets, of the [ith] PAC record. Includes the length of the Codec_ID field, the Codec_Specific_Capabilities_Length field, and the Codec_Specific_Capabilities field.
    Codec_ID[i]	            1       	Octet 0: Coding_Format value of the [ith] PAC record. 
                                    Coding_Format values are defined in the Bluetooth Assigned Numbers [1].
                                    Octet 1-2: Company _ID value of the [ith] PAC record. Shall be ignored if octet 0 is not 0xFF.
                                    Company_ID values are defined in the Bluetooth Assigned Numbers [1].
                                    Octet 3-4: Vendor-specific codec_ID value of the [ith] PAC record.  Shall be ignored if octet 0 is not 0xFF.
    Codec_Specific_Capabilities_Length[i]	1	Length of the Codec_Specific_Capabilities value of the ith PAC record.
    Codec_Specific_Capabilities[i]	Varies	Codec_Specific_Capabilities value of the [ith] PAC record.
                                            Shall be zero length if empty.
    PAC_Record_Length[i]                   1       Length, in octets, of the [ith] PAC record. Includes the length of the
                                                   Codec_ID field, the Codec_Specific_Capabilities_Length field, and the
                                                   Codec_Specific_Capabilities field.
    Codec_ID[i]                            1       Octet 0:   Coding_Format value of the [ith] PAC record.
                                                              Coding_Format values are defined in the Bluetooth Assigned
                                                              Numbers [1].
                                                   Octet 1-2: Company _ID value of the [ith] PAC record. Shall be ignored
                                                              if octet 0 is not 0xFF.
                                                              Company_ID values are defined in the Bluetooth Assigned Numbers [1].
                                                   Octet 3-4: Vendor-specific codec_ID value of the [ith] PAC record.  Shall
                                                              be ignored if octet 0 is not 0xFF.
    Codec_Specific_Capabilities_Length[i]  1       Length of the Codec_Specific_Capabilities value of the ith PAC record.
    Codec_Specific_Capabilities[i]       Varies    Codec_Specific_Capabilities value of the [ith] PAC record.
                                                   Shall be zero length if empty.
    PAC_Record_Length[i]	1       Length, in octets, of the [ith] PAC record. Includes the length of the Codec_ID field, the Codec_Specific_Capabilities_Length field, and the Codec_Specific_Capabilities field.
    Codec_ID[i]	            1       Octet 0: Coding_Format value of the [ith] PAC record. 
                                    Coding_Format values are defined in the Bluetooth Assigned Numbers [1].
                                    Octet 1-2: Company _ID value of the [ith] PAC record. Shall be ignored if octet 0 is not 0xFF.
                                    Company_ID values are defined in the Bluetooth Assigned Numbers [1].
                                    Octet 3-4: Vendor-specific codec_ID value of the [ith] PAC record.  Shall be ignored if octet 0 is not 0xFF.
    Codec_Specific_Capabilities_Length[i] 1 Length of the Codec_Specific_Capabilities value of the ith PAC record.
    Codec_Specific_Capabilities[i]  Varies Codec_Specific_Capabilities value of the [ith] PAC record.
                                            Shall be zero length if empty.
    */
    /* skip PAC record length octet */
    pacRecordLength = buff_iterator_get_octet(buffIterator);
    bapCodecSubrecordDeserialiseCodecId(codecSubrecord, buffIterator);
    bapCodecSubrecordDeserialiseNumCodecSpecificParametersOctets(codecSubrecord, buffIterator);

    /*
     * TODO: num_codec_specific_parameters_octets should be bounds checked to make sure an
     *       unrealistic 'bad' value doesn't make us consume all of our heap. That means we
     *       need to determine what is a 'realistic' maximum value.
     */
    if ((codecSubrecord->numCodecSpecificParametersOctets)
        && ( codecSubrecord->numCodecSpecificParametersOctets <= (pacRecordLength - SERIALISED_BAP_CODEC_SUBRECORD_CODEC_ID_SIZE_OCTETS )))
    {
        codecSubrecord->codecSpecificParameters = CsrPmemZalloc(codecSubrecord->numCodecSpecificParametersOctets * sizeof(uint8));
    }

    if (codecSubrecord->codecSpecificParameters)
    {
        for (i = 0; i < codecSubrecord->numCodecSpecificParametersOctets; ++i)
        {
            bapCodecSubrecordDeserialiseCodecSpecificParameters(codecSubrecord, i, buffIterator);
        }

         /* Decode LTV format data from the Codec specific parameter */
        for (ltvFormatData = &codecSubrecord->codecSpecificParameters[0];
             ltvFormatData < &codecSubrecord->codecSpecificParameters[0] + codecSubrecord->numCodecSpecificParametersOctets;
             ltvFormatData = qblLtvGetNextLtvStart(&ltv))
        {
            qblLtvInitialise(&ltv, ltvFormatData);

            /*if ( ! qblLtvDecodeCodecSpecificCapabilities(&ltv, codec_subrecord))
            {
                break; 
            }*/
        }
    }
}

bool bapCodecSubrecordCheckCodecConfiguration(BapCodecSubRecord * const codecSubrecord,
                                              BapCodecConfiguration * const codecConfiguration)
{
    if (codecSubrecord->samplingFrequencies & codecConfiguration->samplingFrequency)
    {
        return TRUE;
    }

    return FALSE;
}

void bapCodecSubrecordDelete(BapCodecSubRecord * const codecSubrecord)
{
    if (codecSubrecord->numCodecSpecificParametersOctets)
    {
        CsrPmemFree(codecSubrecord->codecSpecificParameters);
    }

    if (codecSubrecord->numAudioLocations)
    {
        CsrPmemFree(codecSubrecord->audioLocations);
    }
    CsrPmemFree(codecSubrecord);
}

