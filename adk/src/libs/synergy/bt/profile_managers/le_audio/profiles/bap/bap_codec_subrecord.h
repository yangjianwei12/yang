/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Codec subrecord - methods to operate on the BapCodecSubRecord type.
 */

#ifndef BAP_CODEC_SUBRECORD_H_
#define BAP_CODEC_SUBRECORD_H_

#include <string.h>
#include "bap_client_type_name.h"
#include "bap_client_list_element.h"
#include "bap_client_lib.h"
#include "buff_iterator.h"

/*
 * methods to serialised/deserialise BapCodecSubRecord structures
 */
/* codec subrecords consist of following fields

    Field                           Size(Octets)                        Notes
------------------------------------------------------
    PAC_Record_Length[i]	1       	Length, in octets, of the [ith] PAC record. Includes the length of the Codec_ID field, the Codec_Specific_Capabilities_Length field, and the Codec_Specific_Capabilities field.
    Codec_ID[i]	            1       	Octet 0: Coding_Format value of the [ith] PAC record. 
                                    Coding_Format values are defined in the Bluetooth Assigned Numbers [1].
                                    Octet 1-2: Company _ID value of the [ith] PAC record. Shall be ignored if octet 0 is not 0xFF.
                                    Company_ID values are defined in the Bluetooth Assigned Numbers [1].
                                    Octet 3-4: Vendor-specific codec_ID value of the [ith] PAC record.  Shall be ignored if octet 0 is not 0xFF.
    Codec_Specific_Capabilities_Length[i]	1	Length of the Codec_Specific_Capabilities value of the ith PAC record.
    Codec_Specific_Capabilities[i]	Varies	Codec_Specific_Capabilities value of the [ith] PAC record.
                                            Shall be zero length if empty.
*/

#define SERIALISED_BAP_CODEC_SUBRECORD_CODEC_LENGTH_OCTETS (1)
#define SERIALISED_BAP_CODEC_SUBRECORD_CODEC_ID_SIZE_OCTETS (5)
/*#define SERIALISED_BAP_CODEC_SUBRECORD_NUM_AUDIO_LOCATIONS_SIZE_OCTETS (1)
#define SERIALISED_BAP_CODEC_SUBRECORD_AUDIO_LOCATION_SIZE_OCTETS (2)
#define SERIALISED_BAP_CODEC_SUBRECORD_CHANNEL_MODE_SIZE_OCTETS (2)
#define SERIALISED_BAP_CODEC_SUBRECORD_SAMPLING_FREQUENCY_SIZE_OCTETS (1) */
#define SERIALISED_BAP_CODEC_SUBRECORD_CODEC_PARAMETERS_SIZE_OCTETS (1)

#define SERIALISED_BAP_CODEC_SUBRECORD_SIZE(numAudioLocations, numCodecSpecificParams)                                      \
                                             (SERIALISED_BAP_CODEC_SUBRECORD_CODEC_LENGTH_OCTETS+                              \
                                             SERIALISED_BAP_CODEC_SUBRECORD_CODEC_ID_SIZE_OCTETS +                              \
                                             SERIALISED_BAP_CODEC_SUBRECORD_CODEC_PARAMETERS_SIZE_OCTETS +                       \
                                             (numCodecSpecificParams))

/*
 * BapCodecSubRecord methods
 */
#define bapCodecSubrecordSerialiseCodecId(codecSubrecord, iter) \
    buff_iterator_get_octet(iter) = codecSubrecord->codecId.codecId;\
    buff_iterator_get_octet(iter) = (uint8)(codecSubrecord->codecId.companyId && 0x00FF);\
    buff_iterator_get_octet(iter) = (uint8)((codecSubrecord->codecId.companyId && 0xFF00) >> 8);\
    buff_iterator_get_octet(iter) = (uint8)(codecSubrecord->codecId.vendorCodecId && 0x00FF);\
    buff_iterator_get_octet(iter) = (uint8)((codecSubrecord->codecId.vendorCodecId && 0xFF00) >> 8)/* copy the value then move the data pointer along */

#define bapCodecSubrecordDeserialiseCodecId(codecSubrecord, iter) \
    codecSubrecord->codecId.codecId = buff_iterator_get_octet(iter);\
    codecSubrecord->codecId.companyId = (uint16)buff_iterator_get_octet(iter);\
    codecSubrecord->codecId.companyId = (uint16)(buff_iterator_get_octet(iter) << 8);\
    codecSubrecord->codecId.vendorCodecId = (uint16)buff_iterator_get_octet(iter);\
    codecSubrecord->codecId.vendorCodecId = (uint16)(buff_iterator_get_octet(iter) << 8)/* copy the value then move the data pointer along */

#define bapCodecSubrecordSerialisePacRecordLength(codecSubrecord, iter) \
    buff_iterator_get_octet(iter) = codecSubrecord->numCodecSpecificParametersOctets + \
    SERIALISED_BAP_CODEC_SUBRECORD_CODEC_ID_SIZE_OCTETS + SERIALISED_BAP_CODEC_SUBRECORD_CODEC_LENGTH_OCTETS

#define bapCodecSubrecordSerialiseSamplingFrequencies(codecSubrecord, iter) \
    buff_iterator_get_octet(iter) = codecSubrecord->samplingFrequencies  /* copy the value then move the data pointer along */

#define bapCodecSubrecordDeserialiseSamplingFrequencies(codecSubrecord, iter) \
    (codecSubrecord)->samplingFrequencies = buff_iterator_get_octet(iter)  /* copy the value then move the data pointer along */

#define bapCodecSubrecordSerialiseNumCodecSpecificParametersOctets(codecSubrecord, iter) \
    buff_iterator_get_octet(iter) = codecSubrecord->numCodecSpecificParametersOctets  /* copy the value then move the data pointer along */

#define bapCodecSubrecordDeserialiseNumCodecSpecificParametersOctets(codecSubrecord, iter) \
    (codecSubrecord)->numCodecSpecificParametersOctets = buff_iterator_get_octet(iter)  /* copy the value then move the data pointer along */

#define bapCodecSubrecordSerialiseCodecSpecificParameters(codecSubrecord, iter) \
    if (codecSubrecord->numCodecSpecificParametersOctets && \
        (codecSubrecord->codecSpecificParameters != NULL))  \
        memcpy((iter)->data, &codecSubrecord->codecSpecificParameters[0], codecSubrecord->numCodecSpecificParametersOctets); \
    (iter)->data += codecSubrecord->numCodecSpecificParametersOctets

#define bapCodecSubrecordDeserialiseCodecSpecificParameters(codecSubrecord, i, iter) \
    (codecSubrecord)->codecSpecificParameters[i] = buff_iterator_get_octet(iter)


#define bapCodecSubrecordGetSerialisedSize(codecSubrecord) \
    SERIALISED_BAP_CODEC_SUBRECORD_SIZE((codecSubrecord)->numAudioLocations, (codecSubrecord)->numCodecSpecificParametersOctets)

void bapCodecSubrecordSerialise(BapCodecSubRecord * const codecSubrecord,
                                BUFF_ITERATOR* buffIterator);

void bapCodecSubrecordDeserialise(BapCodecSubRecord * const codecSubrecord,
                                  BUFF_ITERATOR* buffIterator);

void bapCodecSubrecordDelete(BapCodecSubRecord * const codecSubrecord);

bool bapCodecSubrecordCheckCodecConfiguration(BapCodecSubRecord * const codecSubrecord,
                                              BapCodecConfiguration * const codecConfiguration);


#endif /* BAP_CODEC_SUBRECORD_H_ */
