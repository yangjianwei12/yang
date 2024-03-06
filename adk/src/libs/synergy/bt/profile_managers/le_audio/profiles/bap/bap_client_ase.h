/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_CLIENT_ASE_H
#define BAP_CLIENT_ASE_H

#include "bap_client_type_name.h"
#include "bap_client_lib.h"
#include "bap_ase.h"
#include "bap_client_ase_fsm_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_UNICAST_CLIENT

/*! \brief BapClientAse struct.
 */
typedef struct
{
    BapAse                           ase;
    fsm16_state_t                    state;
    type_name_declare_rtti_member_variable
} BapClientAse;

BapClientAse *bapClientAseNew(BapAseCodecConfiguration * const aseCodecConfiguration,
                              BapClientPacRecord * const pacRecord,
                              struct BapConnection * const connection,
                              struct BapCis* const cis,
                              struct BapStreamGroup * const streamGroup);

BapResult bapClientAseCodecConfigure(BapClientAse * const clientAse,
                                     BapInternalUnicastClientCodecConfigureReq * const primitive);

BapResult bapClientAseQosConfigure(BapClientAse * const clientAse,
                                   BapInternalUnicastClientQosConfigureReq * const primitive);

BapResult bapClientAseEnable(BapClientAse * const clientAse,
                             BapInternalUnicastClientEnableReq *const primitive);

BapResult bapClientAseCisConnect(BapClientAse * const clientAse,
                                 BapInternalUnicastClientCisConnectReq *const primitive);

BapResult bapClientAseDisable(BapClientAse * const clientAse,
                              BapInternalUnicastClientDisableReq *const primitive);

BapResult bapClientAseRelease(BapClientAse * const clientAse,
                              BapInternalUnicastClientReleaseReq *const primitive);

BapResult bapClientAseUpdateMetadata(BapClientAse * const clientAse,
                                     BapInternalUnicastClientUpdateMetadataReq *const primitive);

BapResult bapClientAseReceiverReady(BapClientAse * const clientAse,
                                    BapInternalUnicastClientReceiverReadyReq *const primitive);

#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */
#ifdef __cplusplus
}
#endif

#endif /* BAP_CLIENT_ASE_H */
