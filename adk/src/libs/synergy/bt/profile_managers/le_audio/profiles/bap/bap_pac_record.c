/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP PAC_RECORD interface implementation.
 */

/**
 * \addtogroup BAP_PAC_RECORD_PRIVATE
 * @{
 */

#include <string.h>
#include "bap_client_list_container_cast.h"
#include "bap_connection.h"
#include "bap_connection.h"
#include "bap_codec_subrecord.h"
#include "bap_pac_record.h"
#include "buff_iterator.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*! \brief RTTI information for the BapClientPacRecord structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapClientPacRecord,  'A','s','P','a')


BapClientPacRecord *bapPacRecordNew(struct BapProfile * const profile,
                                    BapServerDirection serverDirection,
                                    BapCodecSubRecord* codecSubrecord,
                                    uint16 pacRecordId)
{
    BapClientPacRecord *pacRecord;
    bool allocationSuccessful = TRUE;
    uint8 i;

    pacRecord = CsrPmemZalloc(sizeof(BapClientPacRecord));

    if (pacRecord == NULL)
        allocationSuccessful = FALSE;

    if (allocationSuccessful)
    {
        pacRecord->codecSubrecord = CsrPmemZalloc(sizeof(BapCodecSubRecord));

        if (pacRecord->codecSubrecord == NULL)
            allocationSuccessful = FALSE;
    }

    if ((allocationSuccessful) &&
        (codecSubrecord->numAudioLocations > 0))
    {
        pacRecord->codecSubrecord->audioLocations = CsrPmemZalloc(sizeof(uint16) * codecSubrecord->numAudioLocations);

        if (pacRecord->codecSubrecord->audioLocations == NULL)
            allocationSuccessful = FALSE;
    }

    if ((allocationSuccessful) &&
        (codecSubrecord->numCodecSpecificParametersOctets > 0))
    {
        pacRecord->codecSubrecord->codecSpecificParameters = CsrPmemZalloc(sizeof(uint8) * codecSubrecord->numCodecSpecificParametersOctets);

        if (pacRecord->codecSubrecord->codecSpecificParameters == NULL)
            allocationSuccessful = FALSE;
    }

    if (allocationSuccessful != TRUE)
    {
        if (pacRecord)
        {
            if (pacRecord->codecSubrecord)
            {
                /*
                 * The pac_record was zero filled so calling CsrPmemFree on all of these
                 * (even the ones that may be set to zero) is okay
                 */
                CsrPmemFree(pacRecord->codecSubrecord->codecSpecificParameters);
                CsrPmemFree(pacRecord->codecSubrecord->audioLocations);
                CsrPmemFree(pacRecord->codecSubrecord);
            }
            CsrPmemFree(pacRecord);
        }
        return NULL;
    }
    /*
     * At this stage we have memory allocated for a pac_record and a pac_record->codec_subrecord
     */
    bapClientListElementInitialise(&pacRecord->listElement);

    pacRecord->id = pacRecordId;
    pacRecord->profile = profile;
    pacRecord->serverDirection = serverDirection;

    pacRecord->codecSubrecord->channelMode = codecSubrecord->channelMode;
    for(i = 0; i < 5; i++)
    {
        pacRecord->codecSubrecord->codecId = codecSubrecord->codecId;
    }
    pacRecord->codecSubrecord->numAudioLocations = codecSubrecord->numAudioLocations;
    pacRecord->codecSubrecord->numCodecSpecificParametersOctets = codecSubrecord->numCodecSpecificParametersOctets;
    pacRecord->codecSubrecord->presentationDelayMaxMicroseconds = codecSubrecord->presentationDelayMaxMicroseconds;
    pacRecord->codecSubrecord->presentationDelayMinMicroseconds = codecSubrecord->presentationDelayMinMicroseconds;
    pacRecord->codecSubrecord->samplingFrequencies = codecSubrecord->samplingFrequencies;

    if (codecSubrecord->numAudioLocations)
    {
        memcpy(pacRecord->codecSubrecord->audioLocations,
               codecSubrecord->audioLocations,
               codecSubrecord->numAudioLocations * sizeof(uint16));
        CsrPmemFree(codecSubrecord->audioLocations);
    }
    if (codecSubrecord->numCodecSpecificParametersOctets)
    {
        memcpy(pacRecord->codecSubrecord->codecSpecificParameters,
               codecSubrecord->codecSpecificParameters,
               codecSubrecord->numCodecSpecificParametersOctets * sizeof(uint8));
        CsrPmemFree(codecSubrecord->codecSpecificParameters);
    }

    type_name_initialise_rtti_member_variable(BapClientPacRecord, pacRecord);

    return pacRecord;
}

Bool bapPacRecordCheckCodecConfiguration(BapClientPacRecord * const pacRecord,
                                         BapCodecConfiguration * const codecConfiguration)
{
    Bool result = FALSE;
    if (pacRecord != NULL)
    {
        if (pacRecord->codecSubrecord)
        {
            result = bapCodecSubrecordCheckCodecConfiguration(pacRecord->codecSubrecord,
                                                              codecConfiguration);
        }
    }

    return result;
}

size_t bapPacRecordGetSerialisedCodecSubrecordsSize(BapClientPacRecord * const pacRecord)
{
    size_t size = 0;
    if (pacRecord->codecSubrecord)
    {
        size = bapCodecSubrecordGetSerialisedSize(pacRecord->codecSubrecord);
    }
    return size;
}

uint32 bapPacRecordGetAudioLocation(BapClientPacRecord * const pacRecord)
{
    uint8 i = 0;
    uint32 audioLocation = 0;

    if (pacRecord->codecSubrecord)
    {
        for(i = 0; i < pacRecord->codecSubrecord->numAudioLocations; i++)
        {
            if(audioLocation)
            {
                audioLocation |= (uint32)(pacRecord->codecSubrecord->audioLocations[i]);
            }
            else
            {
                audioLocation = (uint32)(pacRecord->codecSubrecord->audioLocations[i]);
            }
        }
    }
    return audioLocation;
}

bool bapPacRecordGetSerialisedCodecSubrecords(BapClientPacRecord * const pacRecord,
                                              size_t *outputBufferSize,
                                              uint8 * const outputBuffer)
{
    BUFF_ITERATOR buffIterator;
    bool result = FALSE;
    buff_iterator_initialise(&buffIterator, outputBuffer);

    if (pacRecord->codecSubrecord)
    {
        bapCodecSubrecordSerialise(pacRecord->codecSubrecord, &buffIterator);

        *outputBufferSize = bapCodecSubrecordGetSerialisedSize(pacRecord->codecSubrecord);
        result = TRUE;
    }
    return result;
}

void bapPacRecordDelete(BapClientPacRecord * const pacRecord)
{
    if (pacRecord->codecSubrecord)
    {
        bapCodecSubrecordDelete(pacRecord->codecSubrecord);
    }

    CsrPmemFree(pacRecord);
}
#endif
/**@}*/
