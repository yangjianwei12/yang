/*******************************************************************************

Copyright (C) 2019-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_GATT_MESSAGE_HANDLER_H
#define BAP_GATT_MESSAGE_HANDLER_H

#include "bap_client_type_name.h"
#include "bap_client_list_element.h"
#include "gatt_ascs_client.h"
#include "gatt_pacs_client.h"
#include "gatt_pacs_client_private.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Forward decl
 */

AppTask getAscpTask(void);

void bapGattMessageHandler(BAP * const bap,
                           uint16 primiveId,
                           void *primitive);

/* Server handlers */

void bapServerGattInit(void);

/* Client Handlers */

void bapClientGattAscsClientInit(BAP * const bap);

void bapClientGattStoreDiscoveredService(BAP * const bap,
                                         uint16 cid,
                                         uint16 uuidType,
                                         uint32 *uuid,
                                         uint16 start,
                                         uint16 end,
                                         bool more);

void bapClientGattAseDiscovery(BapConnection * const connection, 
                               uint8 aseId,
                               BapAseType aseType);

void bapClientGattRegisterAseNotification(BapConnection * const connection,
                                          uint8 aseId,
                                          bool notifyType);

void bapClientGattInitCfm(BAP * const bap,
                          GattAscsClientInitCfm *cfm);

void bapClientGattHandleAsesReadCfm(BAP * const bap,
                                    GattAscsClientReadAseInfoCfm *ind);

void bapClientGattHandleAsesReadStateCfm(BAP * const bap,
                                         GattAscsClientReadAseStateCfm *cfm);

void bapClientGattHandleAseWriteCfm(BAP * const bap,
                                    GattAscsClientWriteAseCfm *cfm);

/* ASCS Client GATT Notification handler */
void bapClientGattHandleNotification(BAP * const bap,
                                     GattAscsClientIndicationInd *ind);

void bapClientGattPacsClientInitCfm(BAP* const bap,
                                    GattPacsClientInitCfm* cfm);

void bapClientGattHandleRegisterPacsNotificationCfm(BAP* const bap,
                                                    GattPacsClientNotificationCfm* cfm);

void bapClientGattHandleReadPacRecordCfm(BAP* const bap,
                                         GattPacsClientReadPacRecordCfm* cfm);

void bapClientGattHandleReadAudioLocationCfm(BAP* const bap,
                                             GattPacsClientReadAudioLocationCfm* cfm);

void bapClientGattHandleWriteAudioLocationCfm(BAP* const bap,
                                              GattPacsClientWriteAudioLocationCfm* cfm);

void bapClientGattHandleReadAudioContextCfm(BAP* const bap,
                                            GattPacsClientReadAudioContextCfm* cfm);

void bapClientGattHandlePacRecordNotificationInd(BAP* const bap,
                                                 GattPacsClientPacRecordNotificationInd* ind);

void bapClientGattHandleAudioLocationNotificationInd(BAP* const bap,
                                                     GattPacsClientAudioLocationNotificationInd* ind);

void bapClientGattHandleAudioContextNotificationInd(BAP* const bap,
                                                    GattPacsClientAudioContextNotificationInd* ind);

#ifdef __cplusplus
}
#endif
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

#endif /* BAP_GATT_MESSAGE_HANDLER_H */
