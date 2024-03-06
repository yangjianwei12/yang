/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Profile interface.
 */

/**
 * \defgroup BAP BapProfile
 * @{
 */

#ifndef BAP_PROFILE_H_
#define BAP_PROFILE_H_

#include "bap_client_type_name.h"
#include "bap_client_list.h"
#include "bap_client_list_element.h"
#include "bap_pac_record.h"
#include "bap_client_debug.h"
#include "bap_connection.h"
#include "bap_client_list_util_private.h"
#include "bap_client_list_util.h"

#include "gatt_pacs_client.h"
#include "gatt_ascs_client.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BapProfile
{
    BapClientListElement listElement;
    BapClientList pacRecordList;
    uint16  id;
    uint16 lastPacRecordId;
    type_name_declare_rtti_member_variable
} BapProfile;

BapProfile *bapProfileNew(uint16 id);

void bapProfileAddPacRecord(BapProfile * const profile,
                            BapClientPacRecord * const pacRecord);

Bool bapProfileRemovePacRecord(BapProfile* const profile,
                               uint16 pacRecordId);

BapClientPacRecord *bapProfileFindPacRecordById(BapProfile * const profile,
                                                uint16 id);


BapClientPacRecord *bapProfileFindPacRecordByCodecId(BapProfile * const profile,
                                                     BapServerDirection serverDirection,
                                                     BapCodecIdInfo* codecId);

uint8 bapProfileGetSerialisedPacRecords(BapProfile * const profile,
                                          uint8 serverDirection,
                                          size_t *totalSize,
                                          uint8 **serialisedCodecRecords,
                                          uint32 *audioLocation);

void bapProfileDelete(BapProfile * const profile);

#ifdef __cplusplus
}
#endif

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif /* BAP_PROFILE_H_ */

/**@}*/
