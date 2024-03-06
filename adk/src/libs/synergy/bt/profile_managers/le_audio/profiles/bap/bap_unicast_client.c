/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include <string.h>
#include "tbdaddr.h"
#include "bap_client_lib.h"
#include "csr_bt_profiles.h"
#include "csr_bt_tasks.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT

void BapUnicastClientReadAseInfoReq(BapProfileHandle handle,
                                    uint8 aseId,
                                    BapAseType aseType)
{
    BapInternalUnicastClientReadAseInfoReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientReadAseInfoReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_READ_ASE_INFO_REQ;
    pPrim->handle = handle;
    pPrim->aseId = aseId;
    pPrim->aseType = aseType;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastRegisterAseNotificationReq(BapProfileHandle handle,
                                          uint8 aseId, 
                                          bool notifyEnable)
{
    BapInternalUnicastClientRegisterAseNotificationReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientRegisterAseNotificationReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_REGISTER_ASE_NOTIFICATION_REQ;
    pPrim->handle = handle;
    pPrim->aseId = aseId;
    pPrim->notifyEnable = notifyEnable;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientCodecConfigReq(BapProfileHandle handle,
                                    uint8 numAseCodecConfigurations,
                                    const BapAseCodecConfiguration * aseCodecConfigurations)
{
    uint8 i;
    BapInternalUnicastClientCodecConfigureReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientCodecConfigureReq));
    
    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_CODEC_CONFIGURE_REQ;
    pPrim->handle = handle;

    pPrim->numAseCodecConfigurations = numAseCodecConfigurations;

    for (i = 0; i < numAseCodecConfigurations; ++i)
    {
        pPrim->aseCodecConfigurations[i] = CsrPmemAlloc(sizeof(BapAseCodecConfiguration));
        if(aseCodecConfigurations)
            memcpy(pPrim->aseCodecConfigurations[i], (aseCodecConfigurations+i), sizeof(BapAseCodecConfiguration));

    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}


void BapUnicastClientQosConfigReq(BapProfileHandle handle,
                                  uint8 numAseQosConfigurations,
                                  const BapAseQosConfiguration * aseQosConfigurations)
{
    uint8 i;
    BapInternalUnicastClientQosConfigureReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientQosConfigureReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_QOS_CONFIGURE_REQ;
    pPrim->handle = handle;

    pPrim->numAseQosConfigurations = numAseQosConfigurations;

    for (i = 0; i < numAseQosConfigurations; ++i)
    {
        pPrim->aseQosConfigurations[i] = CsrPmemAlloc(sizeof(BapAseQosConfiguration));
        if(aseQosConfigurations)
            memcpy(pPrim->aseQosConfigurations[i], (aseQosConfigurations+i), sizeof(BapAseQosConfiguration));
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientEnableReq(BapProfileHandle handle,
                               uint8 numAseEnableParameters,
                               const BapAseEnableParameters * aseEnableParameters)
{
    uint16 i;
    BapInternalUnicastClientEnableReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientEnableReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_ENABLE_REQ;

    pPrim->handle = handle;

    pPrim->numAseEnableParameters = numAseEnableParameters;

    for (i = 0; i < numAseEnableParameters; ++i)
    {
        pPrim->aseEnableParameters[i] = CsrPmemAlloc(sizeof(BapAseEnableParameters));

        if (aseEnableParameters)
        {
            pPrim->aseEnableParameters[i]->aseId = (aseEnableParameters + i)->aseId;
            pPrim->aseEnableParameters[i]->streamingAudioContexts = (aseEnableParameters + i)->streamingAudioContexts;
            pPrim->aseEnableParameters[i]->metadataLen = (aseEnableParameters + i)->metadataLen;
            pPrim->aseEnableParameters[i]->metadata = NULL;

            if (pPrim->aseEnableParameters[i]->metadataLen > 0)
            {
                if ((aseEnableParameters + i)->metadata != NULL)
                {
                    pPrim->aseEnableParameters[i]->metadata = CsrPmemAlloc(sizeof(uint8) * pPrim->aseEnableParameters[i]->metadataLen);

                    memcpy(pPrim->aseEnableParameters[i]->metadata, (aseEnableParameters + i)->metadata,
                        (sizeof(uint8) * pPrim->aseEnableParameters[i]->metadataLen));

                    CsrPmemFree((aseEnableParameters + i)->metadata);
                }

            }
        }

    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientUpdateMetadataReq(BapProfileHandle handle,
                                       uint8 numAseMetadataParameters,
                                       const BapAseMetadataParameters * aseMetadataParameters)
{
    BapInternalUnicastClientUpdateMetadataReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientUpdateMetadataReq));
    uint8 i;
    
    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_UPDATE_METADATA_REQ;
    pPrim->handle = handle;

    pPrim->numAseMetadataParameters = numAseMetadataParameters;

    for (i = 0; i < pPrim->numAseMetadataParameters; ++i)
    {
        pPrim->aseMetadataParameters[i] = CsrPmemAlloc(sizeof(BapAseMetadataParameters));
        if(aseMetadataParameters)
            memcpy(pPrim->aseMetadataParameters[i], (aseMetadataParameters+i), sizeof(BapAseMetadataParameters));
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientReceiverReadyReq(BapProfileHandle handle,
                                      uint8 readyType,
                                      uint8 numAse,
                                      const uint8 * aseIds)
{
    BapInternalUnicastClientReceiverReadyReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientReceiverReadyReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_RECEIVER_READY_REQ;

    pPrim->handle = handle;
    pPrim->readyType = readyType;
    pPrim->numAses = numAse;
    if (numAse && (aseIds != NULL))
    {
        pPrim->aseIds = CsrPmemAlloc(pPrim->numAses * sizeof(uint8));
        memcpy(pPrim->aseIds, aseIds, pPrim->numAses * sizeof(uint8));
    }
 
    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}


void BapUnicastClientDisableReq(BapProfileHandle handle,
                                uint8 numAseDisableParameters,
                                const BapAseParameters * aseDisableParameters)
{
    BapInternalUnicastClientDisableReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientDisableReq));
    uint16 i;

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_DISABLE_REQ;

    pPrim->handle = handle;
    pPrim->numAseDisableParameters = numAseDisableParameters;
    
    for (i = 0; i < numAseDisableParameters; ++i)
    {
        pPrim->aseDisableParameters[i] = CsrPmemAlloc(sizeof(BapAseParameters));
        if(aseDisableParameters)
            memcpy(pPrim->aseDisableParameters[i], (aseDisableParameters+i), sizeof(BapAseParameters));
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientReleaseReq(BapProfileHandle handle,
                                uint8 numAseReleaseParameters,
                                const BapAseParameters * aseReleaseParameters)
{
    BapInternalUnicastClientReleaseReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientReleaseReq));
    uint16 i;

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_RELEASE_REQ;
    pPrim->handle = handle;
    pPrim->numAseReleaseParameters = numAseReleaseParameters;
    
    for (i = 0; i < numAseReleaseParameters; ++i)
    {
        pPrim->aseReleaseParameters[i] = CsrPmemAlloc(sizeof(BapAseParameters));
        if(aseReleaseParameters)
            memcpy(pPrim->aseReleaseParameters[i], (aseReleaseParameters+i), sizeof(BapAseParameters));
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientCigConfigReq(phandle_t appHandle,
                                  const BapUnicastClientCigParameters *cigParameters)
{
    BapInternalUnicastClientCigConfigureReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientCigConfigureReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_CIG_CONFIGURE_REQ;
    pPrim->handle = appHandle;

    if(cigParameters)
    {
        memcpy(&pPrim->cigParameters, cigParameters, sizeof(BapUnicastClientCigParameters));
    }

    if(pPrim->cigParameters.cisCount)
    {
        pPrim->cigParameters.cisConfig = CsrPmemZalloc(pPrim->cigParameters.cisCount *
                        sizeof(BapUnicastClientCisConfig));
        if(cigParameters && cigParameters->cisConfig)
        {
            memcpy(pPrim->cigParameters.cisConfig, cigParameters->cisConfig,
                (pPrim->cigParameters.cisCount * sizeof(BapUnicastClientCisConfig)));
        }
    }
    
    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientCigTestConfigReq(phandle_t appHandle,
                                      const BapUnicastClientCigTestParameters *cigTestParams)
{
    BapInternalUnicastClientCigTestConfigureReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientCigTestConfigureReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_CIG_TEST_CONFIGURE_REQ;
    pPrim->handle = appHandle;

    if(cigTestParams)
    {
        memcpy(&pPrim->cigTestParameters, cigTestParams, sizeof(BapUnicastClientCigTestParameters));
    }

    if(pPrim->cigTestParameters.cisCount)
    {
        pPrim->cigTestParameters.cisTestConfig = CsrPmemZalloc(pPrim->cigTestParameters.cisCount *
                        sizeof(BapUnicastClientCisTestConfig));
        if(cigTestParams && cigTestParams->cisTestConfig)
        {
            memcpy(pPrim->cigTestParameters.cisTestConfig, cigTestParams->cisTestConfig,
                (pPrim->cigTestParameters.cisCount * sizeof(BapUnicastClientCisTestConfig)));
        }
    }
    
    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientCigRemoveReq(phandle_t appHandle,
                                  uint8 cigId)
{
    BapInternalUnicastClientCigRemoveReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientCigRemoveReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_CIG_REMOVE_REQ;
    pPrim->handle = appHandle;
    pPrim->cigId = cigId;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientCisConnectReq(BapProfileHandle handle,
                                   uint8  cisCount,
                                   const BapUnicastClientCisConnection *cisConnParameters)
{
    BapInternalUnicastClientCisConnectReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientCisConnectReq));
    uint8 i;
    
    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_CIS_CONNECT_REQ;
    pPrim->handle = handle;
    pPrim->cisCount = cisCount;

    if(cisCount)
    {
        for(i = 0; i<cisCount; i++)
        {
            pPrim->cisConnParameters[i] = CsrPmemZalloc(sizeof(BapUnicastClientCisConnection));
            if(cisConnParameters+i)
            {
                memcpy(pPrim->cisConnParameters[i], (cisConnParameters+i),
                                     sizeof(BapUnicastClientCisConnection));
            }
        }
    }
    
    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapUnicastClientCisDiconnectReq(BapProfileHandle handle,
                                     uint16  cisHandle,
                                     uint8 disconReason)
{
    BapInternalUnicastClientCisDisconnectReq *pPrim = CsrPmemZalloc(sizeof(BapInternalUnicastClientCisDisconnectReq));

    pPrim->type = BAP_INTERNAL_UNICAST_CLIENT_CIS_DISCONNECT_REQ;
    pPrim->handle = handle;
    pPrim->cisHandle = cisHandle;
    pPrim->reason = disconReason;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}
#endif

void BapClientSetupDataPathReq(BapProfileHandle handle,
                               const BapSetupDataPath *dataPathParameter)
{
    BapInternalSetupDataPathReq *pPrim = CsrPmemZalloc(sizeof(BapInternalSetupDataPathReq));

    pPrim->type = BAP_INTERNAL_SETUP_DATA_PATH_REQ;
    pPrim->handle = handle;
    if(dataPathParameter)
    {
        memcpy(&pPrim->dataPathParameter, dataPathParameter,
                                        sizeof(BapSetupDataPath));
    }

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

void BapClientRemoveDataPathReq(BapProfileHandle handle,
                                uint16 isoHandle,
                                uint8 dataPathDirection)
{
    BapInternalRemoveDataPathReq *pPrim = CsrPmemZalloc(sizeof(BapInternalRemoveDataPathReq));

    pPrim->type = BAP_INTERNAL_REMOVE_DATA_PATH_REQ;
    pPrim->handle = handle;
    pPrim->isoHandle = isoHandle;
    pPrim->dataPathDirection = dataPathDirection;

    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}

#ifdef INSTALL_LEA_UNICAST_CLIENT
void BapSetControlPointOpReq(BapProfileHandle handle,
                             bool controlOpResponse,
                             bool longWrite)
{
    BapInternalSetControlPointOpReq *pPrim =  CsrPmemZalloc(sizeof(BapInternalSetControlPointOpReq));

    pPrim->type = BAP_INTERNAL_SET_CONTROL_POINT_OP_REQ;
    pPrim->handle = handle;
    pPrim->controlOpResponse = controlOpResponse;
    pPrim->longWrite = longWrite;
    CsrSchedMessagePut(CSR_BT_BAP_IFACEQUEUE, BAP_PRIM, pPrim);
}
#endif /* #ifdef INSTALL_LEA_UNICAST_CLIENT */

