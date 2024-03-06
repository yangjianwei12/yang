/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP BapProfile interface implementation.
 */

/**
 * \addtogroup BAP_PROFILE_PRIVATE
 * @{
 */

#include <string.h>
#include "bap_client_list_container_cast.h"
#include "bap_profile.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
/*! \brief RTTI information for the BapProfile structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapProfile,  'A','s','P','r')

/*! \brief Make the BapClientPacRecord structure RTTI information visible.
 */
type_name_enable_verify_of_external_type(BapClientPacRecord)

BapProfile *bapProfileNew(uint16 id)
{
    BapProfile *profile = CsrPmemAlloc(sizeof(BapProfile));

    if (profile != NULL)
    {
        profile->id = id;

        profile->lastPacRecordId = 0xafaf;

        bapClientListElementInitialise(&profile->listElement);

        bapClientListInitialise(&profile->pacRecordList);

        type_name_initialise_rtti_member_variable(BapProfile, profile);
    }

    return profile;
}

void bapProfileAddPacRecord(BapProfile * const profile,
                            BapClientPacRecord * const pacRecord)
{
    bapClientListPush(&profile->pacRecordList, &pacRecord->listElement);

    ++profile->lastPacRecordId;

    if (profile->lastPacRecordId == INVALID_PAC_RECORD_ID)
    {
        ++profile->lastPacRecordId;
    }
}

Bool bapProfileRemovePacRecord(BapProfile* const profile,
                               uint16 pacRecordId)
{
    BapClientPacRecord* pacRecord = NULL;
    BapClientListElement* listElement;

    for (listElement = bapClientListPeekFront(&profile->pacRecordList);
        listElement != NULL;
        listElement = bapClientListElementGetNext(listElement))
    {
        pacRecord = CONTAINER_CAST(listElement, BapClientPacRecord, listElement);
        if (pacRecord->id == pacRecordId)
        {
            bapClientListRemove(&profile->pacRecordList, listElement);
            bapPacRecordDelete(pacRecord);
            return TRUE;
        }
    }

    return FALSE;
}

BapClientPacRecord *bapProfileFindPacRecordById(BapProfile * const profile,
                                                uint16 id)
{
    BapClientPacRecord *pacRecord = NULL;
    BapClientListElement *listElement;

    for (listElement = bapClientListPeekFront(&profile->pacRecordList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         pacRecord = CONTAINER_CAST(listElement, BapClientPacRecord, listElement);

         if (pacRecord->id == id)
         {
             break;
         }
         else
         {
             pacRecord = NULL;
         }
    }

    return pacRecord;
}


BapClientPacRecord *bapProfileFindPacRecordByCodecId(BapProfile * const profile,
                                                     BapServerDirection serverDirection,
                                                     BapCodecIdInfo* codecId)
{
    BapClientListElement *listElement;
    BapClientPacRecord *pacRecord = NULL;
    (void)serverDirection;

    for (listElement = bapClientListPeekFront(&profile->pacRecordList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
         pacRecord = CONTAINER_CAST(listElement, BapClientPacRecord, listElement);
         if (bapPacRecordGetCodecSubrecord(pacRecord) &&
            (pacRecord->codecSubrecord->codecId.codecId == codecId->codecId) && 
            (pacRecord->codecSubrecord->codecId.companyId == codecId->companyId) &&
            (pacRecord->codecSubrecord->codecId.vendorCodecId == codecId->vendorCodecId))
         {
             break;
         }
         else
         {
             pacRecord = NULL;
         }
    }

    return pacRecord;
}

uint8 bapProfileGetSerialisedPacRecords(BapProfile * const profile,
                                          uint8 serverDirection,
                                          size_t *totalSize,
                                          uint8 **serialisedPacRecords,
                                          uint32 *audioLocation)
{
    BapClientListElement *listElement;
    BapClientPacRecord *pacRecord = NULL;
    size_t outBufferOffset = 0;
    uint8 numberOfPacRecords = 0;
    uint8 *outSerialisedPacRecords = NULL;

    *totalSize = 0;

    for (listElement = bapClientListPeekFront(&profile->pacRecordList);
         listElement != NULL;
         listElement = bapClientListElementGetNext(listElement))
    {
        pacRecord = CONTAINER_CAST(listElement, BapClientPacRecord, listElement);

        if ((pacRecord->serverDirection == serverDirection) || (serverDirection == BAP_DIRECTION_FILTER_ALL_MASK))
        {
            if (bapPacRecordGetCodecSubrecord(pacRecord))
            {
                *totalSize += bapPacRecordGetSerialisedCodecSubrecordsSize(pacRecord);
                 *audioLocation = bapPacRecordGetAudioLocation(pacRecord);
                ++numberOfPacRecords;
            }
        }
    }

    if (numberOfPacRecords)
    {
        outSerialisedPacRecords = (uint8 *)CsrPmemAlloc(*totalSize * sizeof(uint8));

        for (listElement = bapClientListPeekFront(&profile->pacRecordList);
             listElement != NULL;
             listElement = bapClientListElementGetNext(listElement))
        {
            size_t serialised_pac_record_size;

            pacRecord = CONTAINER_CAST(listElement, BapClientPacRecord, listElement);

            if ((pacRecord->serverDirection == serverDirection) || (serverDirection == BAP_DIRECTION_FILTER_ALL_MASK))
            {
                if (bapPacRecordGetCodecSubrecord(pacRecord))
                {
                    /* Store the server_direction of the code subrecords. */
                    /* out_serialised_pac_records[out_buffer_offset++] = pac_record->server_direction; */

                    bapPacRecordGetSerialisedCodecSubrecords(pacRecord,
                                                             &serialised_pac_record_size,
                                                             &outSerialisedPacRecords[outBufferOffset]);

                    outBufferOffset += serialised_pac_record_size;
                }
            }
        }
    }

    *serialisedPacRecords = outSerialisedPacRecords;

    return numberOfPacRecords;
}

void bapProfileDelete(BapProfile * const profile)
{
    BapClientPacRecord *pacRecord;
    BapClientListElement *listElement;

    while (!bapClientListIsEmpty(&profile->pacRecordList))
    {
        listElement = bapClientListPop(&profile->pacRecordList);

        pacRecord = CONTAINER_CAST(listElement, BapClientPacRecord, listElement);

        if(pacRecord)
        {
            bapPacRecordDelete(pacRecord);
        }
    }

    CsrPmemFree(profile);
}

void *BapGetAttributeHandles(BapProfileHandle profileHandle, uint8 clntProfile)
{
    BapConnection* connection = NULL;
    BAP* bap = bapGetInstance();

    bapClientFindConnectionByCid(bap, profileHandle, &connection);

    if (connection)
    {
        if (clntProfile == BAP_CLIENT_PROFILE_PACS)
        {
            return (void*)GattPacsClientGetHandlesReq(connection->pacs.srvcHndl);
        }
        else if (clntProfile == BAP_CLIENT_PROFILE_ASCS)
        {
            return (void*)GattAscsClientGetHandlesReq(connection->ascs.srvcHndl);
        }
        else if (clntProfile == BAP_CLIENT_PROFILE_BASS)
        {
             return (void*)GattBassClientGetHandlesReq(connection->bass.srvcHndl);
        }
    }
    return NULL;
}
#endif
/**@}*/