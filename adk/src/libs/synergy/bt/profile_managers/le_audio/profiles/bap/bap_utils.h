/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP utility functions interface.
 */

/**
 * \defgroup BAP_UTILS BAP
 * @{
 */

#ifndef BAP_UTILS_H_
#define BAP_UTILS_H_

#include "bap_client_lib.h"
#include "bap_connection.h"
#include "bap_ase.h"
#include "csr_sched.h"
#include "buff_iterator.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct QblLtv
{
    uint8       length;
    uint8       type;
    uint8*      ltvFormattedDataStart;
    BUFF_ITERATOR iter;
} QblLtv;

bool bapParseAdvDataForUuid16(uint8* data,
                              uint16 dataLength,
                              uint16 baasUuid,
                              uint32* baasServiceData,
                              uint8 baasServiceDataLen,
                              char *bigName,
                              uint8* bigNameLen,
                              uint8 *serviceData,
                              uint8* serviceDataLen);

void qblLtvInitialise(QblLtv* ltv, uint8* ltvStart);

bool qblLtvDecodeCodecSpecificCapabilities(QblLtv* ltv, BapCodecConfiguration* codecConfig);

#ifdef INSTALL_LEA_UNICAST_CLIENT
bool qblLtvDecodeCodecSpecificConfig(QblLtv* ltv,
                                     BapAse* ase,
                                     uint8 codecId,
                                     AscsAseErrorStatus* errorStatus);

void bapUtilsCleanupBapConnection(BapConnection* connection);
#endif

uint8* qblLtvGetNextLtvStart(QblLtv* ltv);

uint16 qblLtvGetValueU16le(QblLtv* ltv);

uint32 qblLtvGetValueU24le(QblLtv* ltv);

uint32 qblLtvGetValueU32le(QblLtv* ltv);

#define  qblLtvGetValueU8(ltv)   buff_iterator_get_octet((ltv)->iter)

#define  qblLtvLengthIsCorrect(ltv, expectedLength)  ((ltv)->length == (expectedLength))

uint8 readUint8(uint8 **buf);

uint16 readUint16(uint8 **buf);

uint24 readUint24(uint8 **buf);

uint32 readUint32(uint8 **buf);

bool bapUtilsFindLtvValue(uint8 * ltvData,
                          uint8 ltvDataLength,
                          uint8 type,
                          uint8 * value,
                          uint8 valueLength);

void bapUtilsSendBapInitCfm(phandle_t phandle,
                            uint32 cid,
                            BapResult result,
                            BapRole role);

void bapUtilsSendBapDeinitCfm(phandle_t phandle,
                              uint32 cid,
                              BapRole role,
                              BapResult result,
                              BapHandles *handles);

void bapUtilsSendRegisterPacsNotificationCfm(phandle_t phandle,
                                             uint32 cid,
                                             BapResult result);

void bapUtilsSendPacsNotificationInd(phandle_t phandle,
                                     uint32 cid,
                                     BapPacsNotificationType notifyType,
                                     void* notifyValue);

void bapUtilsSendPacRecordNotificationInd(phandle_t phandle,
                                          uint32 cid,
                                          BapPacsNotificationType notifyType,
                                          const BapPacRecord* record,
                                          uint8 recordCount);


void bapUtilsSendDiscoverRemoteAudioCapabilityCfm(phandle_t phandle,
                                                  uint32 cid,
                                                  BapResult result,
                                                  BapPacRecordType recordType,
                                                  uint8 numPacRecords,
                                                  const BapPacRecord* pacRecords,
                                                  bool moreToCome);

void bapUtilsSendGetRemoteAudioLocationCfm(phandle_t phandle,
                                           uint32 cid,
                                           BapResult result,
                                           BapServerDirection direction,
                                           BapAudioLocation location);

void bapUtilsSendSetRemoteAudioLocationCfm(phandle_t phandle,
                                           uint32 cid,
                                           BapResult result);

void bapUtilsSendDiscoverAudioContextCfm(phandle_t phandle,
                                         uint32 cid,
                                         BapResult result,
                                         BapPacAudioContext context,
                                         BapPacAudioContextValue contextValue);

void bapUtilsSendRegisterAseCfm(phandle_t phandle,
                                BapResult result,
                                uint8 aseId,
                                BapProfileHandle cid);

void bapUtilsSendGetRemoteAseInfoCfm(phandle_t phandle,
                                     BapResult result,
                                     BapProfileHandle cid,
                                     uint16 numAses,
                                     uint8 * const aseIds,
                                     uint8 aseState,
                                     BapAseType type,
                                     uint8 aseInfoLen,
                                     uint8 *aseInfo);

void bapUtilsSendAseCodecConfigureInd(phandle_t phandle,
                                      BapResult result,
                                      BapProfileHandle streamGroupId,
                                      uint32 presentationDelayMin,
                                      uint32 presentationDelayMax,
                                      BapAseInfo *aseInfo,
                                      uint8  peerFraming,
                                      uint8  peerPhy,
                                      uint8  peerRetransmissionEffort,
                                      uint16 peerTransportLatency,
                                      BapCodecConfiguration *codecParams,
                                      bool clientInitiated);

void bapUtilsSendStreamGroupCodecConfigureCfm(phandle_t phandle,
                                              BapResult result,
                                              BapProfileHandle streamGroupId);

void bapUtilsSendAseQosConfigureInd(phandle_t phandle,
                                    BapResult result,
                                    BapProfileHandle streamGroupId,
                                    uint32 presentationDelayMicroseconds,
                                    BapAseInfo *aseInfo);

void bapUtilsSendStreamGroupQosConfigureCfm(phandle_t phandle,
                                            BapResult result,
                                            BapProfileHandle streamGroupId);

void bapUtilsSendAseDisableNotificationInd(phandle_t phandle,
                                           uint16 cid,
                                           uint8 numAses,
                                           uint8 *aseId);

void bapUtilsSendAseEnableInd(phandle_t phandle,
                              BapResult result,
                              BapProfileHandle streamGroupId,
                              BapAseInfo *aseInfo,
                              uint8 metadataLength,
                              uint8* metadata);

void bapUtilsSendStreamGroupEnableCfm(phandle_t phandle,
                                      BapResult result,
                                      BapProfileHandle streamGroupId);

void bapUtilsSendAseUpdateMetadataInd(phandle_t phandle,
                                      BapResult result,
                                      BapProfileHandle streamGroupId,
                                      BapAseInfo *aseInfo,
                                      uint16 context,
                                      uint8 metadataLength,
                                      uint8* metadata);

void bapUtilsSendStreamGroupMetadataCfm(phandle_t phandle,
                                        BapResult result,
                                        BapProfileHandle streamGroupId);

void bapUtilsSendAseDisableInd(phandle_t phandle,
                               BapResult result,
                               BapProfileHandle streamGroupId,
                               BapAseInfo *aseInfo,
                               bool clientInitiated);

void bapUtilsSendStreamGroupDisableCfm(phandle_t phandle,
                                       BapResult result,
                                       BapProfileHandle streamGroupId);

void bapUtilsSendStreamGroupReceiverReadyCfm(phandle_t phandle,
                                             BapResult result,
                                             BapProfileHandle streamGroupId,
                                             uint8 readyType);

void bapUtilsSendAseReceiverReadyInd(phandle_t phandle,
                                     BapProfileHandle streamGroupId,
                                     uint8 ready_type,
                                     BapResult result,
                                     BapAseInfo *aseInfo,
                                     bool clientInitiated);

void bapUtilsSendAseReleaseInd(phandle_t phandle,
                               BapResult result,
                               BapProfileHandle streamGroupId,
                               BapAseInfo *aseInfo,
                               bool clientInitiated);

void bapUtilsSendStreamGroupReleaseCfm(phandle_t phandle,
                                       BapResult result,
                                       BapProfileHandle streamGroupId);

void bapUtilsSendAseReleasedInd(phandle_t phandle,
                                BapProfileHandle streamGroupId,
                                BapAseInfo *aseInfo);

void putMessageSynergy(CsrSchedQid q, CsrUint16 mi, void *mv);

void bapUtilsSendCigConfigureCfm(phandle_t phandle,
                                 CmIsocConfigureCigCfm *prim);

void bapUtilsSendCigTestConfigureCfm(phandle_t phandle,
                                     CmIsocConfigureCigTestCfm *prim);

void bapUtilsSendRemoveCigCfm(phandle_t phandle,
                              CmIsocRemoveCigCfm *prim);

void bapUtilsSendStreamGroupCisConnectInd(phandle_t phandle,
                                          BapResult result,
                                          BapProfileHandle streamGroupId,
                                          uint16 cisHandle,
                                          BapUnicastClientCisParam *cisParams,
                                          bool clientInitiated);

void bapUtilsSendStreamGroupCisConnectCfm(phandle_t phandle,
                                          BapResult result,
                                          BapProfileHandle streamGroupId);

void bapUtilsSendCisDisconnectInd(phandle_t phandle,
                                  uint16 reason,
                                  BapProfileHandle streamGroupId,
                                  uint16 cisHandle);

void bapUtilsSendCisDisconnectCfm(phandle_t phandle,
                                  BapResult result,
                                  BapProfileHandle streamGroupId,
                                  uint16 cisHandle);

void bapUtilsSendSetupDatapathCfm(phandle_t phandle, 
                                  uint16 handle,
                                  BapResult result, 
                                  BapProfileHandle streamGroupId);

void bapUtilsSendRemoveDatapathCfm(phandle_t phandle,
                                   uint16 handle,
                                   BapResult result);

void bapSetUtilsSendControlPointOpCfm(phandle_t phandle,
                                      BapProfileHandle handle,
                                      BapResult result);

uint8 bapMapPacsSamplingFreqToAscsValue(uint16 sampFreq);

#ifdef __cplusplus
}
#endif

#endif /* BAP_UTILS_H_ */

/**@}*/
