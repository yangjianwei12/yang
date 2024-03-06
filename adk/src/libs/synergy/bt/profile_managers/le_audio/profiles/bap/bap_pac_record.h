/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP PAC record interface.
 */

/**
 * \defgroup BAP BapClientPacRecord
 * @{
 */

#ifndef BAP_PAC_RECORD_H_
#define BAP_PAC_RECORD_H_

#include "bap_codec_subrecord.h"
#include "bap_client_type_name.h"
#include "bap_client_list.h"
#include "bap_client_list_element.h"
#include "bap_client_lib.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_PAC_RECORD_ID (0xffff)

typedef struct
{
    BapClientListElement        listElement;
    uint16                      id;
    BapServerDirection          serverDirection;
    BapCodecSubRecord*          codecSubrecord;
    struct BapProfile*         profile;
    type_name_declare_rtti_member_variable
} BapClientPacRecord;

BapClientPacRecord *bapPacRecordNew(struct BapProfile * const profile,
                                    BapServerDirection serverDirection,
                                    BapCodecSubRecord* codecSubrecord,
                                    uint16 pacRecordId);
/*
bool bapPacRecordAddCodecSubrecord(BapClientPacRecord * const pacRecord,
                                   BapCodecSubRecord * const codecSubrecord);
*/
Bool bapPacRecordCheckCodecConfiguration(BapClientPacRecord * const pacRecord,
                                         BapCodecConfiguration * const codecConfiguration);

size_t bapPacRecordGetSerialisedCodecSubrecordsSize(BapClientPacRecord * const pacRecord);

uint32 bapPacRecordGetAudioLocation(BapClientPacRecord * const pacRecord);

bool bapPacRecordGetSerialisedCodecSubrecords(BapClientPacRecord * const pacRecord,
                                              size_t *outputBufferSize,
                                              uint8 * const outputBuffer);

void bapPacRecordDelete(BapClientPacRecord * const pacRecord);


#define bapPacRecordDeserialiseDirection(pacRecord, buffIterator) \
        (pacRecord)->serverDirection = *(buffIterator)->data++;

#define bapPacRecordGetCodecSubrecord(pacRecord)   (pacRecord)->codecSubrecord

#ifdef __cplusplus
}
#endif

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif /* BAP_PAC_RECORD_H_ */

/**@}*/
