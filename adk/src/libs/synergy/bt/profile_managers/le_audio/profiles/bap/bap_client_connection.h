/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP initiator connection interface.
 */

/**
 * \defgroup BapClientConnection BAP
 * @{
 */

#ifndef BAP_CLIENT_CONNECTION_H_
#define BAP_CLIENT_CONNECTION_H_

#include "bap_client_type_name.h"
#include "bap_client_list_util.h"
#include "bap_pac_record.h"
#include "bap_connection.h"
#include "bap_client_list_util_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    BapClientListElement  listElement;
    BapConnection         connection;
    struct BapStreamGroup* streamGroup;
    union
    {
        struct
        {
            phandle_t rspPhandle;
        } getRemotePacRecords;
    } stateInfo;
    type_name_declare_rtti_member_variable
} BapClientConnection;

BapConnection *bapClientConnectionNew(BAP * const bap,
                                      phandle_t phandle,
                                      TYPED_BD_ADDR_T * const addrt);

void bapClientConnectionGetAllCisesInCig(BapClientConnection * const this,
                                         BapClientList* cisList,
                                         uint8 cigId);


BapResult bapClientConnectionSendConfigCodecOp(BapClientConnection * const clientConnection,
                                               uint8 numAses,
                                               BapAseCodecConfiguration ** const codecConfiguration);

BapResult bapClientConnectionSetAsePdSendConfigQosOp(BapClientConnection * const clientConnection,
                                                     uint8 numAses,
                                                     BapAseQosConfiguration** const aseQosConfiguration);

BapResult bapClientConnectionSendEnableOp(BapClientConnection * const clientConnection,
                                          uint8 numAses,
                                          BapAseEnableParameters **const aseEnableParams);

BapResult bapClientConnectionSendMetadataOp(BapClientConnection * const clientConnection,
                                            uint8 numAses,
                                            BapAseMetadataParameters **const aseMetadataParams);

BapResult bapClientConnectionSendHandshakeOp(BapClientConnection * const clientConnection,
                                             uint8 readyType,
                                             uint8 numAses,
                                             uint8 *aseIds);

BapResult bapClientConnectionSendCisConnect(BapClientConnection * const clientConnection,
                                            uint8 cisCount,
                                            BapUnicastClientCisConnection **const cisConnParams);

BapResult bapClientConnectionSendDisableOp(BapClientConnection * const clientConnection,
                                           uint8 numAses,
                                           BapAseParameters **const aseDisableParams);

BapResult bapClientConnectionSendReleaseOp(BapClientConnection * const clientConnection,
                                           uint8 numAses,
                                           BapAseParameters **const aseReleaseParams);

void bapClientConnectionHandleReadAseInfoCfm(BapConnection * const connection,
                                             uint8 numAses,
                                             uint8 *aseInfo,
                                             uint8 aseState,
                                             BapAseType type);

void bapClientConnectionRcvAscsMsg(BapClientConnection * const clientConnection,
                                   AscsMsg * const msg);

#define bapClientConnectionSetStreamGroup(conn, streamGrp) ((conn)->streamGroup = (streamGrp))

#ifdef __cplusplus
}
#endif

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#endif /* BAP_CLIENT_CONNECTION_H_ */

/**@}*/
