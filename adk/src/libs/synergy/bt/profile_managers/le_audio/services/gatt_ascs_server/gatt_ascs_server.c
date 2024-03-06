/******************************************************************************
 Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #7 $
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "gatt_ascs_server_db.h"
#include "service_handle.h"
#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_msg_handler.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"
#include "gatt_ascs_server_debug.h"

#include "bap_server_lib.h"

#define ASCS_SERVER_DEBUG

#if defined(ASCS_SERVER_DEBUG)
/* Static variable storing the internal of the ASCS Server to allow debugging
 * in pydbg. The strcuture can be accessed as follow:
 * apps1.fw.env.cus["gattAscsServer.c"].localvars["pGascs"].deref
 * apps1.fw.env.struct("pGascs")
 **/
static GattAscsServer* pGascs;
#define GATT_ASCS_SERVER_SET_GASCS_DEBUG(pgascs) (pGascs = (pgascs))
#else
#define GATT_ASCS_SERVER_SET_GASCS_DEBUG(pgascs) ((void)0)
#endif /*ASCS_SERVER_DEBUG*/

#ifdef INSTALL_ASCS_NOTIFY_CB
static GattAscsAseNtfHook GattAscsAseNtfCb = NULL;
#endif

ServiceHandle ascsServiceHandle;

static uint8* ascsAseConstructQosConfiguredCharacteristicValue(GattAscsServerAse* ase, uint8* characteristicLength);

void ascsAseControlPointCharacteristicReset(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 opCode)
{
    /*
     * Note: Don't memset the entire struct as it will overwrite the clientCfg
     */
    int i;
    aseControlPointNotify->opCode = opCode;
    aseControlPointNotify->numAses = 0;

    for ( i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
    {
        aseControlPointNotify->aseResponses[i].reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
        aseControlPointNotify->aseResponses[i].responseCode = GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS;
    }
}

void ascsAseControlPointCharacteristicAddAseResponse(GattAscsAseControlPointNotify* aseControlPointNotify, AscsAseResponse* aseResponse)
{
    int i;
    const uint8 numAses = MIN(aseControlPointNotify->numAses, GATT_ASCS_NUM_ASES_MAX);

    if (aseControlPointNotify->numAses == GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES)
    {
        return; /* The entire op has been aborted already, in ASCS 'validation r03' this can be because the server received an invalid length op or an unrecognised op code */
    }

    /* Find if we already have this ASE response in the ASE CP Characteristic (and it just needs updating) */
    for ( i = 0; i < numAses; i++)
    {
        if (aseControlPointNotify->aseResponses[i].aseId == aseResponse->aseId)

        {
            /* Allow 'success'to be overwritten with an error response code
             * but don't overwrite an existing error response.
             */
            if (aseControlPointNotify->aseResponses[i].responseCode == GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS)
            {
                aseControlPointNotify->aseResponses[i] = *aseResponse;
            }
            return;
        }
    }

    /* We don't already have this ASE response, we need to add it */
    if (aseControlPointNotify->numAses < GATT_ASCS_NUM_ASES_MAX)
    {
        aseControlPointNotify->aseResponses[aseControlPointNotify->numAses] = *aseResponse;
        aseControlPointNotify->numAses++;
    }
}

void ascsAseControlPointCharacteristicAddSuccessResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS;

    ascsAseControlPointCharacteristicAddAseResponse(aseControlPointNotify, &response);
}

void ascsAseControlPointCharacteristicAddInvalidAseIdResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_ID;

    ascsAseControlPointCharacteristicAddAseResponse(aseControlPointNotify, &response);
}

void ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify)
{
    aseControlPointNotify->numAses = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES; /* special value defined in ASCS 'validation r03' used for invalid length op or unrecognised op code */

    aseControlPointNotify->aseResponses[0].aseId = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_ASE_ID; /* ASCS 'validation r03' use aseId = 0 in response to an invalid length op or unrecognised op code */
    aseControlPointNotify->aseResponses[0].reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    aseControlPointNotify->aseResponses[0].responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_LENGTH_OPERATION;
}

void ascsAseControlPointCharacteristicAddUnsupportedOpcodeResponse(GattAscsAseControlPointNotify* aseControlPointNotify)
{
    aseControlPointNotify->numAses = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES; /* special value defined in ASCS 'validation r03' used for an invalid length op or unrecognised op code */

    aseControlPointNotify->aseResponses[0].aseId = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_ASE_ID;  /* ASCS 'validation r03' use aseId = 0 in response to an invalid length op or unrecognised op code */
    aseControlPointNotify->aseResponses[0].reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    aseControlPointNotify->aseResponses[0].responseCode = GATT_ASCS_ASE_RESPONSE_CODE_UNSUPPORTED_OPCODE;
}

void ascsAseControlPointCharacteristicAddInvalidTransitionResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                                       uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_STATE_TRANSITION;

    ascsAseControlPointCharacteristicAddAseResponse(aseControlPointNotify, &response);

}

void ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                                           uint8 aseId,
                                                                           uint8 reasonCode)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = reasonCode;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_CONFIGURATION_PARAMETER_VALUE;

    ascsAseControlPointCharacteristicAddAseResponse(aseControlPointNotify, &response);
}

void ascsAseControlPointCharacteristicAddRejectedParameterValueResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                                            uint8 aseId,
                                                                            uint8 reasonCode)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = reasonCode;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_REJECTED_CONFIGURATION_PARAMETER_VALUE;

    ascsAseControlPointCharacteristicAddAseResponse(aseControlPointNotify, &response);
}

void ascsAseControlPointCharacteristicAddUnspecifiedErrorResponseCode(GattAscsAseControlPointNotify* aseControlPointNotify, uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_UNSPECIFIED_ERROR;

    ascsAseControlPointCharacteristicAddAseResponse(aseControlPointNotify, &response);
}


uint8* ascsAseControlPointConstructCharacteristicValue(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                       uint8* characteristicLength)
{
    GattAscsBuffIterator iter;
    uint8* characteristicValue;
    uint8 numAseResponses;
    int i;

    struct
    {
        /*
         * Fields in the ASE Control Point Characteristic : ASCS d09r06
         */
        uint8 opCode;
        uint8 numAses;
        /*     ASCS_ASE_RESPONSE_T aseResponses[];        * variable length */
    } aseRespone;

    /* Check to see if the GATT_ASCS_ASE_CONTROL_POINT_NOTIFY_T is capable of constructing a Notify */
    if (aseControlPointNotify->numAses == 0)
    {
        *characteristicLength = 0;
        return NULL;
    }

    *characteristicLength = sizeof(aseRespone);

    /*
     * The ASCS 'Validation r03' spec uses the 'numAses' field AND the response code to convey essentially the same information: the op has been aborted
     * This means the numAses field does NOT represent the number of ASE responses in the aseControlPointNotify for an invalid length op or
     * invalid op code, instead, for these cases the number of ASE entries is actually 1, but the numAses field is set to 0xFF
     */
    numAseResponses = (aseControlPointNotify->numAses == GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES)? 1 : aseControlPointNotify->numAses;
    numAseResponses = MIN(numAseResponses, GATT_ASCS_NUM_ASES_MAX);

    *characteristicLength += numAseResponses * sizeof(AscsAseResponse);

    characteristicValue = zpmalloc(*characteristicLength);

    ascsBuffIteratorInitialise(&iter, characteristicValue, *characteristicLength);

    /*
     * populate the characteristicValue
     */
    ascsBuffIteratorWrite8(&iter, aseControlPointNotify->opCode);
    ascsBuffIteratorWrite8(&iter, aseControlPointNotify->numAses);
    for ( i = 0; i < numAseResponses; i++)
    {
        ascsBuffIteratorWrite8(&iter, aseControlPointNotify->aseResponses[i].aseId);
        ascsBuffIteratorWrite8(&iter, aseControlPointNotify->aseResponses[i].responseCode);
        ascsBuffIteratorWrite8(&iter, aseControlPointNotify->aseResponses[i].reason);
    }

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristicValue);
        characteristicValue = NULL;
        *characteristicLength = 0;
    }

    return characteristicValue;
}

static bool ascsAseClientNotify(const GattAscsServer *ascsServer,
                                ConnectionId cid, GattAscsServerAse* ase,
                                GattAscsServerConfigureCodecServerReqInfo*  stateSpecificData)
{
    uint8 characteristicLength;
    uint8* characteristicValue;
    uint16 aseCharacteristic = 0;

    if(ase->clientCfg != GATT_ASCS_CLIENT_CONFIG_NOTIFY)
    {
        /* Notify is not enabled by the client. Server does not need to send
            any notifications to the client */
        return TRUE;
    }

    characteristicValue = ascsAseConstructCharacteristicValue(ase,
                                                              &characteristicLength,
                                                              stateSpecificData);

    if (characteristicValue == NULL)
    {
        return FALSE;
    }

    /*TODO: As we support more ASEs this switch will grow*/
    switch(ase->aseId)
    {
        case 1:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_1;
        }
        break;
#ifdef HANDLE_ASCS_ASE_CHAR_2
        case 2:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_2;
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_3
        case 3:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_3;
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_4
        case 4:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_4;
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_5
        case 5:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_5;
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_6
        case 6:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_6;
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_7
        case 7:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_7;
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_8
        case 8:
        {
            aseCharacteristic = (uint16) HANDLE_ASCS_ASE_CHAR_8;
        }
        break;
#endif
        default:
        {
            /*this should never happen*/
            GATT_ASCS_SERVER_PANIC("ASE ID does not match the numer of"
                                            "characteristics supported");
        }
        break;
    }

#ifdef INSTALL_ASCS_NOTIFY_CB
    if(GattAscsAseNtfCb != NULL)
    {
        GattAscsAseNtfCb(cid, ase->aseId, (void *)characteristicValue);

        if(ase->state == GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED)
        {
            /* update the values if anything modified */
            CodecConfigInfo *notify = (CodecConfigInfo *)characteristicValue;

            ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin = ((uint32)(notify->presentationDelayMin[0]) |
                (uint32)(notify->presentationDelayMin[1] << 8) | (uint32)(notify->presentationDelayMin[2] << 16));
        }
    }
#endif /* INSTALL_ASCS_NOTIFY_CB */

        /* Send notification to GATT Manager */
    CsrBtGattNotificationEventReqSend(ascsServer->gattId,
                                      cid,
                                      aseCharacteristic,
                                      characteristicLength,
                                      characteristicValue);


    return TRUE;
}

static uint8* ascsAseConstructIdleAndReleasingCharacteristicValue(GattAscsServerAse* ase, uint8* characteristicLength)
{
    GattAscsBuffIterator iter;
    uint8* characteristicValue;
    struct
    {
        /*
        * Fields in the ASE Characteristic : ASCS d09r06 Table 4.2 (idle and release have no
        * 'additional parameters')
        */

        uint8 aseId;
        uint8 aseState;
    }aseStateInfo;

    *characteristicLength = sizeof(aseStateInfo);

    characteristicValue = zpmalloc(*characteristicLength);

    ascsBuffIteratorInitialise(&iter, characteristicValue, *characteristicLength);

    /*  populate the characteristicValue */
    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristicValue);
        characteristicValue = NULL;
        *characteristicLength = 0;
    }

    return characteristicValue;
}

static uint8* ascsAseConstructCodecConfiguredCharacteristicValue(GattAscsServerAse* ase,
                                                                 uint8* characteristicLength,
                                                                 GattAscsServerConfigureCodecServerReqInfo* configureCodecServerInfo)
{
    GattAscsBuffIterator iter;
    uint8* characteristicValue;
    GattAscsServerConfigureCodecServerReqInfo ascsServerConfigureCodecServerInfo = {0};

    *characteristicLength = sizeof(CodecConfigInfo);

    *characteristicLength += ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength;

    characteristicValue = zpmalloc(*characteristicLength);

    ascsBuffIteratorInitialise(&iter, characteristicValue, *characteristicLength);

    /*  populate the characteristicValue */

    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);

    if (configureCodecServerInfo)
    {
        ascsServerConfigureCodecServerInfo = *configureCodecServerInfo;
    }
    else
    {
        BapServerUnicastPopulateConfigureCodecData(0, /*cid - this needs to be a genuine value if/when BAP uses it */
                                                   ase->aseId,
                                                   &ascsServerConfigureCodecServerInfo);
    }
    ascsBuffIteratorWrite8(&iter,  ascsServerConfigureCodecServerInfo.framing);
    ascsBuffIteratorWrite8(&iter,  ascsServerConfigureCodecServerInfo.phyPreference);
    ascsBuffIteratorWrite8(&iter,  ascsServerConfigureCodecServerInfo.retransmissionNumber);
    ascsBuffIteratorWrite16(&iter, ascsServerConfigureCodecServerInfo.transportLatencyMax);
    ascsBuffIteratorWrite24(&iter, ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin);

    /* If PD Min value is less than BAP defined 40ms*/
    if( ascsServerConfigureCodecServerInfo.presentationDelayMax >= ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin )
    {
        ascsBuffIteratorWrite24(&iter, ascsServerConfigureCodecServerInfo.presentationDelayMax);
    }
    else /* If PD Min value is greater than BAP defined 40ms*/
    {
        ascsBuffIteratorWrite24(&iter, ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin);
    }

    ascsBuffIteratorWrite24(&iter, ascsServerConfigureCodecServerInfo.preferredPresentationDelayMin);
    ascsBuffIteratorWrite24(&iter, ascsServerConfigureCodecServerInfo.preferredPresentationDelayMax);

    ascsBuffIteratorWrite8(&iter,  ase->dynamicData->cachedConfigureCodecInfo.codecId.codingFormat);
    ascsBuffIteratorWrite16(&iter, ase->dynamicData->cachedConfigureCodecInfo.codecId.companyId);
    ascsBuffIteratorWrite16(&iter, ase->dynamicData->cachedConfigureCodecInfo.codecId.vendorSpecificCodecId);

    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength);
    ascsBuffIteratorWriteMultipleOctets(&iter,
                                        ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration,
                                        ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength);

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristicValue);
        characteristicValue = NULL;
        *characteristicLength = 0;
    }

    return characteristicValue;
}

static uint8* ascsAseConstructGenericCharacteristicValue(GattAscsServerAse* ase, uint8* characteristicLength)
{
    GattAscsBuffIterator iter;
    uint8* characteristicValue;

    struct
    {
        /*
        * Fields in the ASE Characteristic : ASCS d09r06
        */
        uint8 aseId;
        uint8 aseState;
        uint8 cigId;
        uint8 cisId;
        uint8 metadataLength;
       /*  uint8 metadata[];  variable length */
    }aseChar;

    if ((ase->dynamicData == NULL) ||
        (ase->dynamicData->cachedConfigureQosInfoFromServer == NULL))
    {
        *characteristicLength = 0;
        return NULL;
    }
    *characteristicLength = sizeof(aseChar);

    *characteristicLength += ase->dynamicData->metadataLength;

    characteristicValue = zpmalloc(*characteristicLength);

    ascsBuffIteratorInitialise(&iter, characteristicValue, *characteristicLength);

    /*
     * populate the characteristicValue
     */

    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureQosInfoFromServer->cigId);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureQosInfoFromServer->cisId);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->metadataLength);

    ascsBuffIteratorWriteMultipleOctets(&iter, ase->dynamicData->metadata, ase->dynamicData->metadataLength);

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristicValue);
        characteristicValue = NULL;
        *characteristicLength = 0;
    }

    return characteristicValue;
}

uint8* ascsAseConstructCharacteristicValue(GattAscsServerAse* ase,
                                           uint8* characteristicLength,
                                           GattAscsServerConfigureCodecServerReqInfo* stateSpecificData)
{
    uint8* characteristicValue = NULL;

    switch(ase->state)
    {
    case GATT_ASCS_SERVER_ASE_STATE_IDLE:
        /* Fall through to construct characteristic with no 'Additional ASE parameters' */
    case GATT_ASCS_SERVER_ASE_STATE_RELEASING:
        characteristicValue = ascsAseConstructIdleAndReleasingCharacteristicValue(ase, characteristicLength);
        break;
    case GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED:
        characteristicValue = ascsAseConstructCodecConfiguredCharacteristicValue(ase, characteristicLength, stateSpecificData);
        break;
    case GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED:
        characteristicValue = ascsAseConstructQosConfiguredCharacteristicValue(ase, characteristicLength);
        break;
    case GATT_ASCS_SERVER_ASE_STATE_ENABLING:
        /* Fall through to construct generic characteristic */
    case GATT_ASCS_SERVER_ASE_STATE_STREAMING:
        /* Fall through to construct generic characteristic */
    case GATT_ASCS_SERVER_ASE_STATE_DISABLING:
        characteristicValue = ascsAseConstructGenericCharacteristicValue(ase, characteristicLength);
        break;

    };

    return characteristicValue;
}

static uint16 csrBtGattAccessIndGetBufferSize(const CsrBtGattAccessInd *accessInd, uint8 bufferIndex)
{
    if (bufferIndex < accessInd->numWriteUnits)
        return accessInd->writeUnit[bufferIndex].valueLength;
    else
        return 0;
}

static uint8 accessIndIteratorIsOutOfBoundRead(AccessIndIterator *accessIndIter)
{
    if ((accessIndIter->bufferIndex >= accessIndIter->accessInd->numWriteUnits) ||
            accessIndIter->offset >= accessIndIter->accessInd->writeUnit[accessIndIter->bufferIndex].valueLength)/* should never happen unles changed forcibly outside the functions */
    {
        accessIndIter->error = TRUE;
    }
    return accessIndIter->error;
}

static void accessIndIteratorIncr(AccessIndIterator *accessIndIter)
{
    accessIndIter->offset++;
    if (accessIndIter->offset >= csrBtGattAccessIndGetBufferSize(accessIndIter->accessInd, accessIndIter->bufferIndex))
    {
        accessIndIter->offset = 0;
        accessIndIter->bufferIndex++;
    }
}

void accessIndIteratorInitialise(AccessIndIterator *accessIndIter, const CsrBtGattAccessInd *accessInd) /*<- this can be renamed 'accessIndIteratorInitialise'
                                                                 but need to check ASCS unit tests*/
{
    if (accessInd == NULL)
    {
        accessIndIter->error = TRUE;
    }
    accessIndIter->accessInd = accessInd;
    accessIndIter->bufferIndex = 0;
    accessIndIter->offset = 0;
    accessIndIter->error = FALSE;
}

uint8 accessIndIteratorRead8(AccessIndIterator* accessIndIter)
{
    if (accessIndIteratorIsOutOfBoundRead(accessIndIter) == FALSE)
    {
        uint8 returnVal = accessIndIter->accessInd->writeUnit[accessIndIter->bufferIndex].value[accessIndIter->offset];
        accessIndIteratorIncr(accessIndIter);
        return returnVal;
    }
    return 0;
}

uint16 accessIndIteratorRead16(AccessIndIterator* accessIndIter)
{
    uint16 value;
    value  = accessIndIteratorRead8(accessIndIter);
    value += accessIndIteratorRead8(accessIndIter) << 0x08;
    return value;
}

uint32 accessIndIteratorRead24(AccessIndIterator* accessIndIter)
{
    uint32 value;
    value  = accessIndIteratorRead8(accessIndIter);
    value += accessIndIteratorRead8(accessIndIter) << 0x08;
    value += accessIndIteratorRead8(accessIndIter) << 0x10;
    return value;
}

uint8* accessIndIteratorReadMultipleOctets(AccessIndIterator* accessIndIter, uint8 numOctets)
{
    uint8* dest = NULL;

    if (!numOctets)
    {
        return NULL;
    }

    dest = zpmalloc(numOctets);
    {/*! TODO: optimise this to use memcpy */
        int i;
        for (i = 0; i < numOctets; i++)
        {
            dest[i] = accessIndIteratorRead8(accessIndIter);
            if (accessIndIter->error == TRUE)
            {
                free(dest);
                return NULL;
            }
        }
    }

    return dest;
}

void ascsBuffIteratorInitialise(GattAscsBuffIterator* iter, uint8* buffer, uint16 size)
{
    iter->dataStart = buffer;
    iter->data = buffer;
    iter->error = FALSE;
    iter->size = size;
}

bool ascsBuffIteratorWrite8(GattAscsBuffIterator* iter, uint8 value)
{
    if ((iter->data - iter->dataStart) < iter->size)
        *iter->data++ = value;
    else
        iter->error = TRUE;

    return ( ! iter->error);
}

bool ascsBuffIteratorWrite16(GattAscsBuffIterator* iter, uint16 value)
{
    bool result;

    result = ascsBuffIteratorWrite8(iter, value & 0x00FF);
    if (result)
        result = ascsBuffIteratorWrite8(iter, (value >> 0x08) & 0x00FF);

    return result;
}

bool ascsBuffIteratorWrite24(GattAscsBuffIterator* iter, uint32 value)
{
    bool result;

    result = ascsBuffIteratorWrite8(iter, value & 0x00FF);
    if (result)
        result = ascsBuffIteratorWrite8(iter, (value >> 0x08) & 0x00FF);
    if (result)
        result = ascsBuffIteratorWrite8(iter, (value >> 0x10) & 0x00FF);

    return result;
}

bool ascsBuffIteratorWriteMultipleOctets(GattAscsBuffIterator* iter, uint8* src, uint8 numOctets)
{
    if (numOctets)
    {
        uint8* dest = iter->data;

        ascsBuffIteratorSkipOctets(iter, numOctets);
        if (! ascsBuffIteratorErrorDetected(iter))
        {
            size_t dstRemainingOctets = (size_t)iter->size - (dest - iter->dataStart);
            SynMemCpyS(dest, dstRemainingOctets, src, numOctets);
        }
    }
    return ( ! iter->error );
}

void ascsBuffIteratorSkipOctets(GattAscsBuffIterator* iter, uint8 numOctets)
{
    iter->data += numOctets;
    if ((iter->data - iter->dataStart) <= iter->size)
    {
        /* no error: iter->data still points within the bounds of the buffer */
    }
    else
        iter->error = TRUE;
}

static void ascsAseInitialise(GattAscsServerAse* ase, uint8 aseId)
{
    CsrMemSet(ase, 0, sizeof(GattAscsServerAse));

    /* Fields that need to be initialised to none zero values should be initialised here */

    ase->aseId = aseId;

    /* Set default value of clientCfg to 0xFFFF */
    ase->clientCfg = GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG;

    /* commented (to save code space) because these initial values are zero
    ase->state = GATT_ASCS_SERVER_ASE_STATE_IDLE;
    ase->metadata = NULL;
    */

    /* TODO We may want to store the characteristic handle in the ase instance (i.e. add a characteristic handle field in
     *      GATT_ASCS_SERVER_ASE_T) and assign it here, possibly passing the handle into ascsServerAseIitialise()
     */
    /* TODO We may want to assign ASE ids differently, i.e. not just use ASE id 1 and 2. The ASE ids could be determined in
     *      this function and (if we have an ase characteristic handle field as described in the 'TODO' above) the ase id
     *      does not need to be numerically related to the ase characteristic handle
     */
}

static void ascsConnectionInitialise(GattAscsConnection* connection, ConnectionId cid)
{
    int idx;
    connection->cid = cid;

    /* Set default value of clientCfg to 0xFFFF */
    connection->aseControlPointNotify.clientCfg = GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG;

    for (idx = 0; idx < GATT_ASCS_NUM_ASES_MAX; idx++)
    {
        /* NOTE: Don't use aseId = 0. In ASCS 'Validation R03' an aseId of 0 is reserved for an invalid length operation or unrecognised an op code.
         *       Use aseId values: 1, 2, 3, ... */
        ascsAseInitialise(&connection->ase[idx], idx + 1);
    }
}

GattAscsConnection* ascsFindConnection(const GattAscsServer *ascsServer, ConnectionId cid)
{
    int i;
    const uint8 numConnections = MIN(ascsServer->numConnections, GATT_ASCS_NUM_CONNECTIONS_MAX);
    for (i = 0; i < numConnections; i++)
    {
        if (ascsServer->connection[i]->cid == cid)
            return ascsServer->connection[i];
    }
    return NULL;
}

static GattAscsConnection* ascsAddConnection(GattAscsServer *ascsServer, ConnectionId cid)
{
    GattAscsConnection* connection;

    if(cid == CSR_BT_CONN_ID_INVALID)
        return NULL;

    connection = ascsFindConnection(ascsServer, cid);

    if (connection == NULL)
    {
        if (ascsServer->numConnections < GATT_ASCS_NUM_CONNECTIONS_MAX)
        {
            connection = (GattAscsConnection *) calloc(1, sizeof(GattAscsConnection));
            if (connection == NULL)
            {
                GATT_ASCS_SERVER_PANIC("Failed to allocate a GattAscsConnection");
            }

            ascsConnectionInitialise(connection, cid);

            ascsServer->connection[ascsServer->numConnections++] = connection;
        }
    }

    return connection;
}

static void ascsAseFreeDynamicData(GattAscsServerAse* ase)
{
    if(ase->dynamicData != NULL)
    {
        /* If codecConfiguration is NULL it is still safe to call free */
        free(ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration);
        /* It is not necessary  to set the codecConfiguration to NULL or
         * codecConfigurationLength to 0 because the whole of ase->dynamicData is being freed */

        free(ase->dynamicData->cachedConfigureQosInfoFromServer);

        /* If metadata is NULL it is still safe to call free */
        free(ase->dynamicData->metadata);
        /* It is not necessary  to set the metadata to NULL or metadataLength to 0
         * because the whole of ase->dynamicData is being freed */

        /* If dynamicData is NULL it is still safe to call free */
        free(ase->dynamicData);
        ase->dynamicData = NULL;
    }
}

void ascsRemoveConnection(GattAscsServer *ascsServer, GattAscsConnection* connection)
{
    bool moveConnectionToLowerIndex = FALSE;
    int i;

    const uint8 numConnections = MIN(ascsServer->numConnections, GATT_ASCS_NUM_CONNECTIONS_MAX);
    for (i = 0; i < numConnections; i++)
    {
        if (moveConnectionToLowerIndex)
        {
            ascsServer->connection[i-1] = ascsServer->connection[i];
        }
        else
        {
            /* the 'else' fixes a Klocwork issue -
             * without it, we could in theory call ascsConnectionDestruct()
             * with the same 'connection' more than once.
             */
            if (ascsServer->connection[i] == connection)
            {
                ascsConnectionDestruct(connection);
                moveConnectionToLowerIndex = TRUE;
                ascsServer->numConnections--;
            }
        }
    }
}

void ascsConnectionDestruct(GattAscsConnection* connection)
{
    int aseIdx;
    for (aseIdx = 0; aseIdx < GATT_ASCS_NUM_ASES_MAX; ++aseIdx)
    {
         ascsAseFreeDynamicData(&connection->ase[aseIdx]);
    }
    free(connection);
}

bool ascsServerSetAseStateAndNotify(const GattAscsServer *ascsServer,
                                    ConnectionId cid,
                                    GattAscsServerAse* ase,
                                    GattAscsServerAseState newState,
                                    GattAscsServerConfigureCodecServerReqInfo* stateSpecificData)
{
    ase->state = newState;

    /* Check to see if we have transitioned to a state where the QOS info is no longer relevant, i.e.
     * transitioned to a state where we MUST receive NEW QOS configuration before attempting to stream again
     */
    switch (ase->state)
    {
    case GATT_ASCS_SERVER_ASE_STATE_IDLE:
    case GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED:
        /* In these states the QOS info (e.g. CIS id / CIG id) are NO LONGER valid: from any of these
         * states, the ASE must first go through 'QOS Configure' to receive NEW QOS info
         * (e.g. new CIS id / CIG id values) before it can attempt streaming
         */
        if (ase->dynamicData)
        {
            free(ase->dynamicData->cachedConfigureQosInfoFromServer);
            ase->dynamicData->cachedConfigureQosInfoFromServer = NULL;
        }
        break;
    default:
        /* In these states the QOS info (e.g. CIS id / CIG id) ARE configured in the ASE and (in
         * particular) the CIG id and CIS id must not clash with CIS id / CIG id values in any other ASEs */
        break;
    };

    if (!ascsAseClientNotify(ascsServer, cid, ase, stateSpecificData))
    {
        return FALSE;
    }

    return TRUE;
}

GattAscsServerAse* ascsServerFindAse(const GattAscsServer *ascsServer, ConnectionId cid, uint8 aseId)
{
    GattAscsConnection* connection = ascsFindConnection(ascsServer, cid);

    if (connection)
    {
        return ascsConnectionFindAse(connection, aseId);
    }
    return NULL;
}

GattAscsServerAse* ascsConnectionFindAse(GattAscsConnection *connection, uint8 aseId)
{
    int i;
    for (i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
    {
        if (connection->ase[i].aseId == aseId){
            return  &connection->ase[i];
        }
    }
    return NULL;
}

/*
 * GattAscsRemoveConfig
 *
 *  1. Find the connection instance (identified by the cid)
 *  2. Store all the connection instance data in the GATT_ASCS_CLIENT_CONFIG_T (so that the connection can be re-instantiated later)
 *  3. Remove the connection instance from the ASCS library
 */
GattAscsClientConfig* GattAscsRemoveConfig(ServiceHandle        serviceHandle,
                        ConnectionId           cid
                        )
{
    int i;
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    GattAscsClientConfig* config = NULL;
    GattAscsConnection* connection;

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }

    connection = ascsFindConnection(ascs, cid);

    if (connection)
    {
        config =(GattAscsClientConfig *)zpnew(GattAscsClientConfig);
        config->aseCharClientCfg = zpmalloc(GATT_ASCS_NUM_ASES_MAX * sizeof(ClientConfig));

        config->aseControlPointCharClientCfg = connection->aseControlPointNotify.clientCfg;
        config->numAses = GATT_ASCS_NUM_ASES_MAX;
        for(i=0; i < GATT_ASCS_NUM_ASES_MAX; i++)
        {
            config->aseCharClientCfg[i] = connection->ase[i].clientCfg;
        }

        ascsRemoveConnection(ascs, connection);
    }

    return config;
}

status_t GattAscsAddConfig(ServiceHandle               serviceHandle,
                           ConnectionId                cid,
                           const GattAscsClientConfig* config)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    GattAscsConnection* connection;
    int i;

    if(ascs == NULL)
    {
        gattAscsServerPanic();
    }

    /* ascsAddConnection() either:
     *      * adds a connection and sets the cid accordingly or
     *      * returns an existing connection that has the specified cid
     */
    connection = ascsAddConnection(ascs, cid);

    if (connection == NULL)
    {
        /* This should only happen if cid == CSR_BT_CONN_ID_INVALID */
        return CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR;
    }

    if (config == NULL)
    {
        /* There is nothing left to do:
         *      * a connection didn't already exist (we'd have returned sooner if one had existed and the config == NULL)
         *      * we successfully created one (we'd have returned sooner if we hadn't created one)
         *      * and there is no configuration to store
         */
        return CSR_BT_GATT_ACCESS_RES_SUCCESS;
    }

    /*
     * A non-NULL config indicates that the lib must configure the connection with the relevant GATT_ASCS_CLIENT_CONFIG_T.
     * The connection instance may (or may not) already exist.
     */

    if (config->numAses != GATT_ASCS_NUM_ASES_MAX)
    {
        GATT_ASCS_SERVER_DEBUG("Invalid numAses in clientCfg, expected %d, received %d\n", GATT_ASCS_NUM_ASES_MAX, config->numAses);
        ascsRemoveConnection(ascs, connection);
        return CSR_BT_GATT_ACCESS_RES_UNSUPPORTED_GROUP_TYPE;
    }

    if (config->aseControlPointCharClientCfg != GATT_ASCS_CLIENT_CONFIG_NOTIFY &&
        config->aseControlPointCharClientCfg != GATT_ASCS_CLIENT_CONFIG_NOT_SET &&
        config->aseControlPointCharClientCfg != GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG)
    {
        GATT_ASCS_SERVER_DEBUG("Invalid Client Configuration ASE CP Characteristic!\n");
        ascsRemoveConnection(ascs, connection);
        return CSR_BT_GATT_ACCESS_RES_INVALID_PDU;/*Only Notify and NO SET*/
    }

    for(i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
    {
        if(config->aseCharClientCfg[i] != GATT_ASCS_CLIENT_CONFIG_NOTIFY &&
            config->aseCharClientCfg[i] != GATT_ASCS_CLIENT_CONFIG_NOT_SET &&
            config->aseCharClientCfg[i] != GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG)
        {
            ascsRemoveConnection(ascs, connection);
            GATT_ASCS_SERVER_DEBUG("Invalid Client Configuration ASE Characteristic!\n");
            return CSR_BT_GATT_ACCESS_RES_INVALID_PDU;/*Only Notify and NO SET*/
        }
    }

    connection->aseControlPointNotify.clientCfg = config->aseControlPointCharClientCfg;

    if (connection->aseControlPointNotify.clientCfg == GATT_ASCS_CLIENT_CONFIG_NOTIFY)
    {
        ascsAseControlPointNotify(ascs, cid);
    }

    for (i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
    {
        connection->ase[i].clientCfg = config->aseCharClientCfg[i];
        if(connection->ase[i].clientCfg == GATT_ASCS_CLIENT_CONFIG_NOTIFY)
        {
            ascsAseClientNotify(ascs, cid, &connection->ase[i], NULL);
        }
    }
    return CSR_BT_GATT_ACCESS_RES_SUCCESS;
}

GattAscsClientConfig* GattAscsServerGetConfig(ServiceHandle srvcHndl, ConnectionId cid)
{
    int i;
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(srvcHndl);
    GattAscsClientConfig* config = NULL;
    GattAscsConnection* connection = NULL;

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }

    connection = ascsFindConnection(ascs, cid);

    if (connection)
    {
        config =(GattAscsClientConfig *)zpnew(GattAscsClientConfig);
        config->aseCharClientCfg = zpmalloc(GATT_ASCS_NUM_ASES_MAX * sizeof(ClientConfig));

        config->aseControlPointCharClientCfg = connection->aseControlPointNotify.clientCfg;
        config->numAses = GATT_ASCS_NUM_ASES_MAX;
        for(i=0; i < GATT_ASCS_NUM_ASES_MAX; i++)
        {
            config->aseCharClientCfg[i] = connection->ase[i].clientCfg;
        }
    }

    return config;
}


static void ascsAseControlPointCharacteristicAddResponseCodeFromBap(GattAscsAseControlPointNotify* aseControlPointNotify,
                                                               uint8 aseId,
                                                               uint8 aseResult,
                                                               uint8 additionalInfo)
{
    AscsAseResponse aseResponse;

    aseResponse.aseId = aseId;
    aseResponse.responseCode = aseResult; /* The ASCS/BAP API aseResult values map directly to ASCS reponseCodes */
    aseResponse.reason = additionalInfo;   /* The ASCS/BAP API additionalInfo values map directly to ASCS reason values*/

#ifdef ASCS_SERVER_DEBUG
    /* ASCS Validation r05 states that the response code 'Unsupported Configuration Parameter Value'
     * 'Shall not be used when the Reason value is 0x05 (Framing)'
     * The framing value specified by the client (in the Configure Qos operation) is validated by ASCS in
     * ascsValidateConfigureQosInfo(), and if the value is not ASCS spec compliant then it is reported
     * to the client as an 'invalid parameter value'.
     * There isn't much that can be done if the Application/Profile then responds to the GATT_ASCS_SERVER_CONFIGURE_QOS_IND
     * with this incompatible combination, but a debug statement could be useful.
     */
    if (aseResult      == GATT_ASCS_ASE_RESULT_UNSUPPORTED_CONFIGURATION_PARAMETER_VALUE &&
        additionalInfo == GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_FRAMING)
    {
        GATT_ASCS_SERVER_DEBUG("ascsAseControlPointCharacteristicAddResponseCodeFromBap: incompatible combination: \'unsupported parameter value\' and \'framing\'");
    }
#endif

    ascsAseControlPointCharacteristicAddAseResponse(aseControlPointNotify, &aseResponse);
}

static void ascsHandleCodecConfiguredRequest(const GattAscsServer *ascsServer, GattAscsServerConfigureCodecReq* request)
{
    GattAscsConnection* connection = ascsFindConnection(ascsServer, request->cid);
    int i;
    const uint8 numAses = MIN(request->numAses, GATT_ASCS_NUM_ASES_MAX);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsConnectionFindAse(connection, request->ase[i].aseId);
        if (ase)
        {
            /* If (before receiving this request) the ASE was in the IDLE state, then
             * no dynamic data will have been allocated */
            if(!ase->dynamicData)
            {
                ase->dynamicData = zpnew(AscsAseDynamicData);
            }
            /* If memory has previously been allocated to store any earlier codec configuration data then
             * we need to free it.
             * If codecConfiguration is NULL it is still safe to call free */
            free(ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration);

            ase->dynamicData->cachedConfigureCodecInfo.codecId = request->ase[i].codecId;

            /*  Note: THERE IS A PROBLEM HERE, introduced as a result of handover optimisation work...
             *  if this request has different values from those
             *  stored in BAP and returned by BapServerUnicastPopulateConfigureCodecResponseData()
             *  then a subsequent Read Characteristic will return the values stored in BAP,
             *  i.e. NOT the values provided in this request and also NOT the values sent in the
             *  Notification to the client (below).
             *  After handover optimisations, a lot of fields in the request are NOT stored in ASCS,
             *  they can be retrieved from BAP when needed (see BapServerUnicastPopulateConfigureCodecResponseData()).
             *  If 'request->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration' points to
             *  valid dynamically allocated memory (i.e. if
             *  request->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfigurationLength != 0)
             *  then the ASCS library takes ownership of (and ultimately frees) this memory.
             */
            ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength = request->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfigurationLength;
            ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration = request->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration;
            ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin = request->ase[i].gattAscsServerConfigureCodecServerInfo->presentationDelayMin;

            if (ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength == 0)
            {
                ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration = NULL;
            }
            /*  The new state is 'codec configured' */
            ascsServerSetAseStateAndNotify(ascsServer,
                                           request->cid,
                                           ase,
                                           GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED,
                                           request->ase[i].gattAscsServerConfigureCodecServerInfo);
        }
        /*
         * The request itself is freed by the profile/app, but the memory pointed to by fields within the response are
         * freed here
         *
         * Note: The ASCS Library takes ownership of and retains the memory pointed to by:
         * request->ase[i].gattAscsServerConfigureCodecServerInfo.codecConfiguration
         */
         free(request->ase[i].gattAscsServerConfigureCodecServerInfo);
    }
}

/***************************************************************************
NAME
    ascsAseHandleCodecConfiguredResponse

DESCRIPTION
    Process the response from the profile/app
*/

static void ascsHandleCodecConfiguredResponse(const GattAscsServer *ascsServer, GattAscsServerConfigureCodecRsp* response)
{
    GattAscsConnection* connection = ascsFindConnection(ascsServer, response->cid);
    int i;
    const uint8 numAses = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

     /* Update the ASE Control Point Notify with the information in the Response from the profile/app */
    for (i = 0; i < numAses; i++)
    {
        if (response->ase[i].gattAscsAseResult.value != GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            ascsAseControlPointCharacteristicAddResponseCodeFromBap(&connection->aseControlPointNotify,
                                                                    response->ase[i].gattAscsAseResult.aseId,
                                                                    response->ase[i].gattAscsAseResult.value,
                                                                    response->ase[i].gattAscsAseResult.additionalInfo);
        }
    }
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotify(ascsServer, response->cid);

    for (i = 0; i < numAses; i++)
    {
        if (response->ase[i].gattAscsAseResult.value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, response->ase[i].gattAscsAseResult.aseId);
            if (ase && ase->dynamicData) /* The codecId, targetLatency and targetPhy should already be stored */
            {
                /* If memory has previously been allocated to store any earlier codec configuration data then
                 * we need to free it.
                 * If codecConfiguration is NULL it is still safe to call free */
                free(ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration);
                /* It is not necessary to set codecConfiguration to NULL or to set
                 * codecConfigurationLength to 0 because these fields are just about to be overwritten
                 * with data from the GattAscsServerConfigureCodecRsp */

                /*  Note: After handover optimisations, a lot of fields in the response are NOT stored in ASCS,
                 *  they can be retrieved from BAP when needed (see BapServerUnicastPopulateConfigureCodecResponseData()).
                 *  If 'response->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration' points to
                 *  valid dynamically allocated memory (i.e. if
                 *  response->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfigurationLength != 0)
                 *  then the ASCS library takes ownership of (and ultimately frees) this memory.
                 */
                ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength = response->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfigurationLength;
                ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration = response->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration;
                ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin = response->ase[i].gattAscsServerConfigureCodecServerInfo->presentationDelayMin;

                if (ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength == 0)
                {
                    ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration = NULL;
                }

                /*  The new state is 'codec configured' */
                ascsServerSetAseStateAndNotify(ascsServer,
                                               response->cid,
                                               ase,
                                               GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED,
                                               NULL);
            }
        }
        /*
         * The response itself is freed by the profile/app, but the memory pointed to by fields within the response are
         * freed here.
         * Note: The ASCS Library takes ownership of and retains the memory pointed to by:
         * response->ase[i].gattAscsServerConfigureCodecServerInfo.codecConfiguration
         */
        free(response->ase[i].gattAscsServerConfigureCodecServerInfo);
    }
}

static uint8* ascsAseConstructQosConfiguredCharacteristicValue(GattAscsServerAse* ase, uint8* characteristicLength)
{
    GattAscsBuffIterator iter;
    uint8* characteristicValue;
    struct {
        /*
         * Fields in the ASE Characteristic : ASCS d09r06
         */
        uint8 aseId;
        uint8 aseState;
        uint8 cigId;
        uint8 cisId;
        uint8 sduInterval[3];
        uint8 framing;
        uint8 phy;
        uint8 maxSdu[2];
        uint8 retransmissionNumber;
        uint8 maxTransportLatency[2];
        uint8 presentationDelay[3];
    }qosConfigureCharValue;

    if ((ase->dynamicData == NULL) ||
        (ase->dynamicData->cachedConfigureQosInfoFromServer == NULL))
    {
        *characteristicLength = 0;
        GATT_ASCS_SERVER_DEBUG("cachedConfigureQosInfoFromServer not initialised");
        return NULL;
    }

    *characteristicLength = sizeof(qosConfigureCharValue);

    characteristicValue = zpmalloc(*characteristicLength);

    ascsBuffIteratorInitialise(&iter, characteristicValue, *characteristicLength);

    /*
     * populate the characteristicValue
     */

    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureQosInfoFromServer->cigId);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureQosInfoFromServer->cisId);
    ascsBuffIteratorWrite24(&iter,ase->dynamicData->cachedConfigureQosInfoFromServer->sduInterval);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureQosInfoFromServer->framing);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureQosInfoFromServer->phy);
    ascsBuffIteratorWrite16(&iter,ase->dynamicData->cachedConfigureQosInfoFromServer->maximumSduSize);
    ascsBuffIteratorWrite8(&iter, ase->dynamicData->cachedConfigureQosInfoFromServer->retransmissionNumber);
    ascsBuffIteratorWrite16(&iter,ase->dynamicData->cachedConfigureQosInfoFromServer->maxTransportLatency);
    ascsBuffIteratorWrite24(&iter,ase->dynamicData->cachedConfigureQosInfoFromServer->presentationDelay);

    if (ascsBuffIteratorErrorDetected(&iter))
    {

        free(characteristicValue);
        characteristicValue = NULL;
        *characteristicLength = 0;
    }
    return characteristicValue;
}

static void ascsHandleConfigureQosRequest(const GattAscsServer *ascsServer, GattAscsServerConfigureQosReq* request)
{
    GattAscsConnection* connection = ascsFindConnection(ascsServer, request->cid);
    int i;
    const uint8 numAses = MIN(request->numAses, GATT_ASCS_NUM_ASES_MAX);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsConnectionFindAse(connection, request->ase[i].aseId);
        if (ase &&
            ase->dynamicData)
        {
            if (ase->dynamicData->cachedConfigureQosInfoFromServer)
            {
                free(ase->dynamicData->cachedConfigureQosInfoFromServer);
            }
            ase->dynamicData->cachedConfigureQosInfoFromServer = request->ase[i].qosInfo;

            ascsServerSetAseStateAndNotify(ascsServer, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED, NULL);
        }
        else
        {
            free(request->ase[i].qosInfo);
        }
    }
}

static void ascsHandleUpdateMetadataRequest(const GattAscsServer *ascsServer,
                                            GattAscsServerUpdateMetadataReq* request)
{
    GattAscsConnection* connection = ascsFindConnection(ascsServer, request->cid);
    int i;
    const uint8 numAses = MIN(request->numAses, GATT_ASCS_NUM_ASES_MAX);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsConnectionFindAse(connection, request->updateMetadataReqInfo[i].aseId);

        if (ase)
        {
            if ((ase->state == GATT_ASCS_SERVER_ASE_STATE_ENABLING) ||
                (ase->state == GATT_ASCS_SERVER_ASE_STATE_STREAMING)||
                (ase->state == GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED))
            {
                /* Free any previously allocated metadata.
                 * If metadata is NULL it is still safe to call free */
                free(ase->dynamicData->metadata);
                ase->dynamicData->metadata = NULL;
                ase->dynamicData->metadataLength = 0;

                ase->dynamicData->metadataLength = request->updateMetadataReqInfo[i].metadataLength;
                ase->dynamicData->metadata = request->updateMetadataReqInfo[i].metadata;

                if(ase->state != GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED)
                {
                    ascsAseClientNotify(ascsServer, request->cid, ase, NULL);
                }
            }
        }
    }
}

static void ascsHandleConfigureQosResponse(const GattAscsServer *ascsServer, GattAscsServerConfigureQosRsp* response)
{
    GattAscsConnection* connection = ascsFindConnection(ascsServer, response->cid);
    int i;
    const uint8 numAses = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    /* Update the ASE Control Point Notify with the information in the Response from the profile/app */
    for (i = 0; i < numAses; i++)
    {
        if (response->ase[i].gattAscsAseResult.value != GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            ascsAseControlPointCharacteristicAddResponseCodeFromBap(&connection->aseControlPointNotify,
                                                                    response->ase[i].gattAscsAseResult.aseId,
                                                                    response->ase[i].gattAscsAseResult.value,
                                                                    response->ase[i].gattAscsAseResult.additionalInfo);
        }
    }
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotify(ascsServer, response->cid);

    for (i = 0; i < numAses; i++)
    {
        if (response->ase[i].gattAscsAseResult.value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, response->ase[i].gattAscsAseResult.aseId);

            if (ase &&
                ase->dynamicData)
            {
                if (ase->dynamicData->cachedConfigureQosInfoFromServer)
                {
                    free(ase->dynamicData->cachedConfigureQosInfoFromServer);
                }
                ase->dynamicData->cachedConfigureQosInfoFromServer = response->ase[i].gattAscsServerConfigureQosRspInfo;

                ascsServerSetAseStateAndNotify(ascsServer, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED, NULL);
            }
            else
            {
                free(response->ase[i].gattAscsServerConfigureQosRspInfo);
            }
        }
        else
        {
            free(response->ase[i].gattAscsServerConfigureQosRspInfo);
        }
    }
}

static void ascsAseControlPointNotifyFromGenericResponse(const GattAscsServer *ascsServer, GattAscsServerGenericRsp* response)
{
    /* Update the ASE Control Point Notify with the information in the Response from the profile/app */
    GattAscsConnection* connection = ascsFindConnection(ascsServer, response->cid);
    int i;

    if (connection)
    {
        const uint8 numAses = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);
        for(i = 0; i < numAses; i++)
        {
            if (response->gattAscsAseResult[i].value != GATT_ASCS_ASE_RESULT_SUCCESS)
            {
                ascsAseControlPointCharacteristicAddResponseCodeFromBap(&connection->aseControlPointNotify,
                                                                        response->gattAscsAseResult[i].aseId,
                                                                        response->gattAscsAseResult[i].value,
                                                                        response->gattAscsAseResult[i].additionalInfo);
            }
        }
    }
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotify(ascsServer, response->cid);
}

static void ascsHandleEnableResponse(const GattAscsServer *ascsServer, GattAscsServerEnableRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    int i;
    const uint8 numAses  = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);

    ascsAseControlPointNotifyFromGenericResponse(ascsServer, response);

    for (i = 0; i < numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascsServer, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                ascsServerSetAseStateAndNotify(ascsServer, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_ENABLING, NULL);
            }
        }
    }
}

static void ascsHandleReceiverReadyRequest(const GattAscsServer *ascsServer, GattAscsServerReceiverReadyReq* request)
{
    const uint8 numAses = MIN(request->numAses, GATT_ASCS_NUM_ASES_MAX);
    int i;

    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascsServer, request->cid, request->aseId[i]);
        if (ase)
        {
            /*
             * TODO:  CHeck if this state change is only meant to happen for 'server is sink' or 'server is source' ASEs
             */
            ascsServerSetAseStateAndNotify(ascsServer, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_STREAMING, NULL);
        }
    }
}

static void ascsHandleReceiverReadyResponse(const GattAscsServer *ascsServer, GattAscsServerReceiverReadyRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    int i;
    const uint8 numAses = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);

    ascsAseControlPointNotifyFromGenericResponse(ascsServer, response);

    for (i = 0; i < numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascsServer, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                /* If the Server is the Audio Source then the server will receive the GATT_ASCS_SERVER_RECEIVER_READY_IND
                 * and respond by calling GattAscsReceiverReadyResponse(). This response is only appropriate when
                 * the server is the audio source.
                 */
                if (GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
                {
                    ascsServerSetAseStateAndNotify(ascsServer, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_STREAMING, NULL);
                }
            }
        }
    }
}

static void ascsHandleUpdateMetadataResponse(const GattAscsServer *ascsServer, GattAscsServerUpdateMetadataRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    int i;
    const uint8 numAses = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);

    ascsAseControlPointNotifyFromGenericResponse(ascsServer, response);

    for (i = 0; i < numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascsServer, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                ascsAseClientNotify(ascsServer, response->cid, ase, NULL);
            }
        }
    }
}
/*
 *  ASCS d09r06: If the server is acting as an Audio Sink for the ASE and the server is ready to stop
 *  consuming audio data transmitted by the client, the server may autonomously initiate
 *  the Receiver Stop Ready operation as defined in Section 5.6 without first sending a
 *  notification of the ASE characteristic value in the Disabling state
 */
static void ascsHandleDisableResponse(const GattAscsServer *ascsServer, GattAscsServerDisableRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    int i;
    const uint8 numAses = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);

    ascsAseControlPointNotifyFromGenericResponse(ascsServer, response);

    for (i = 0; i < numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascsServer, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                if (GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
                {
                    /* The server is now discarding any received audio data, so autonomously perform the
                     * Receiver Stop Ready operation, i.e. transition to QOS configured */
                    ascsServerSetAseStateAndNotify(ascsServer, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED, NULL);
                }
                else if (GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
                {
                    /* Transition to 'disabling' and await the Receiver Stop Ready operation from the client before ceasing transmission */
                    ascsServerSetAseStateAndNotify(ascsServer, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_DISABLING, NULL);
                }
            }
        }
    }
}
/*
 *  ASCS d09r06: If the server is acting as an Audio Sink for the ASE and the server is ready to stop
 *  consuming audio data transmitted by the client, the server may autonomously initiate
 *  the Receiver Stop Ready operation as defined in Section 5.6 without first sending a
 *  notification of the ASE characteristic value in the Disabling state
 */
static void ascsHandleDisableRequest(const GattAscsServer *ascsServer, GattAscsServerDisableReq* request, bool cisLoss)
{
    int i;
    const uint8 numAses = MIN(request->numAses, GATT_ASCS_NUM_ASES_MAX);
    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascsServer, request->cid, request->aseId[i]);
        if (ase)
        {
            if (GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
            {
                /* The server calls the GattAscsServerDisableRequest API after it has stopped consuming audio data
                 * on all ASEs for which it is an audio data sink, so autonomously perform the Receiver Stop
                 * Ready operation, i.e. transition to QOS configured
                 */
                ascsServerSetAseStateAndNotify(ascsServer, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED, NULL);
            }
            else if (GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
            {
                if (cisLoss)
                {
                    /* In the 'CIS loss' case the receipt of a 'Receiver Stop Ready' is implied, so the
                     * ASE transitions immediately to the QOS Configured state.
                     */
                    ascsServerSetAseStateAndNotify(ascsServer, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED, NULL);
                }
                else
                {
                    ascsServerSetAseStateAndNotify(ascsServer, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_DISABLING, NULL);
                }
            }
        }
    }
}

static void ascsHandleReceiverStopReadyResponse(const GattAscsServer *ascsServer, GattAscsServerReceiverStopReadyRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'responseCode' will have been set either in the opCode handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    int i;
    const uint8 numAses = MIN(response->numAses, GATT_ASCS_NUM_ASES_MAX);

    ascsAseControlPointNotifyFromGenericResponse(ascsServer, response);

    for (i = 0; i < numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascsServer, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                if (GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
                {
                    /* The server has stopped transmitting, the ASE can transition to the QOS configured state */
                    ascsServerSetAseStateAndNotify(ascsServer, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED, NULL);
                }
                /*
                else if (ase->direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
                {
                    no action taken for these ASEs (there shouldn't have been any 'server is sink' ases in the RECEIVER_STOP_READY_OP/RECEIVER_STOP_READY_IND)
                }
                */
            }
        }
    }
}

static void ascsHandleReleaseRequest(const GattAscsServer *ascsServer, GattAscsServerReleaseReq* request)
{
    int i;
    const uint8 numAses = MIN(request->numAses, GATT_ASCS_NUM_ASES_MAX);
    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascsServer, request->cid, request->aseId[i]);
        if (ase)
        {
            ascsServerSetAseStateAndNotify(ascsServer, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_RELEASING, NULL);
        }
    }
}

static void ascsHandleReleaseComplete(const GattAscsServer *ascsServer, GattAscsServerReleaseComplete* releaseComplete)
{
    int i;
    const uint8 numAses = MIN(releaseComplete->numAses, GATT_ASCS_NUM_ASES_MAX);
    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascsServer, releaseComplete->cid, releaseComplete->ase[i].aseId);
        if (ase)
        {
            if (releaseComplete->ase[i].cacheCodecConfiguration)
            {
                /* If we have metadata then this needs to be freed here; we are not caching the metadata.
                 * If 'metadata' is NULL it is still safe to call free. */
                free(ase->dynamicData->metadata);
                ase->dynamicData->metadata = NULL;
                ase->dynamicData->metadataLength = 0;

                if (releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo)
                {
                    /*
                     * NOTE: The ASCS library takes ownership of and ultimately frees the memory pointed to by the
                     *       releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo and
                     *       releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration .
                     */

                    /* If memory has previously been allocated to store any earlier codec configuration data then
                     * we need to free it.
                     * If codecConfiguration is NULL it is still safe to call free  */
                    free(ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration);
                    /* It is not necessary to set codecConfiguration to NULL or to set
                     * codecConfigurationLength to 0 because these fields are just about to be overwritten
                     * with data from the GattAscsServerReleaseComplete */

                    /*  Note: THERE IS A PROBLEM HERE, introduced as a result of handover optimisation work...
                     *  if this releaseComplete has different values from those
                     *  stored in BAP and returned by BapServerUnicastPopulateConfigureCodecResponseData()
                     *  then a subsequent Read Characteristic will return the values stored in BAP,
                     *  i.e. NOT the values provided in this releaseComplete and also NOT the values sent in the
                     *  Notification to the client (below).
                     *  After handover optimisations, a lot of fields in the request are NOT stored in ASCS,
                     *  they can be retrieved from BAP when needed (see BapServerUnicastPopulateConfigureCodecResponseData()).
                     *  If 'releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration' points to
                     *  valid dynamically allocated memory (i.e. if
                     *  releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfigurationLength != 0)
                     *  then the ASCS library takes ownership of (and ultimately frees) this memory.
                     */
                    ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength = releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfigurationLength;
                    ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration = releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo->codecConfiguration;
                    ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin = releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo->presentationDelayMin;

                    if (ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength == 0)
                    {
                        ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.codecConfiguration = NULL;
                    }
                    free(releaseComplete->ase[i].gattAscsServerConfigureCodecServerInfo);
                }
                ascsServerSetAseStateAndNotify(ascsServer,
                                               releaseComplete->cid,
                                               ase,
                                               GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED,
                                               NULL);
            }
            else
            {
                ascsAseFreeDynamicData(ase);
                ascsServerSetAseStateAndNotify(ascsServer, releaseComplete->cid, ase, GATT_ASCS_SERVER_ASE_STATE_IDLE, NULL);
            }
        }
    }
}

/* Only one instance of Audio Stream Endpoint Service is supported */
/****************************************************************************/
ServiceHandle GattAscsServerInit(AppTask appTask,
                                    uint16 startHandle,
                                    uint16 endHandle
                                    )
{
    GattAscsServer* ascs;
    CsrBtGattId gattId;

    /* validate the input parameters */
    if (appTask == CSR_SCHED_QID_INVALID)
    {

        GATT_ASCS_SERVER_PANIC("ASCS: Invalid Initialisation parameters");
    }

    /* Allocate memory for the new instance and assign a serviceHandle */
    ascsServiceHandle = ServiceHandleNewInstance((void **)&ascs, sizeof(GattAscsServer));
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    GATT_ASCS_SERVER_SET_GASCS_DEBUG(ascs);

    /* Reset all the service library memory, e.g. numConnections = 0  */
    CsrMemSet(ascs, 0, sizeof(GattAscsServer));



    /* Store application message handler as application messages need to be posted here */
    ascs->appTask = appTask;

    /* Fill in the registration parameters */
    ascs->serviceHandle = ascsServiceHandle;
    ascs->libTask = CSR_BT_ASCS_SERVER_IFACEQUEUE;
    /* Try to register this instance of ASCS library to Gatt profile */
    /* Register with the GATT  */
     gattId = CsrBtGattRegister(ascs->libTask);
    /* verify the result */
     if (gattId == CSR_BT_GATT_INVALID_GATT_ID)
     {
         ServiceHandleFreeInstanceData(ascsServiceHandle);
         ascsServiceHandle = 0;
     }
     else
     {
         ascs->gattId = gattId;
         CsrBtGattConfigModeReqSend(ascs->gattId, CSR_BT_GATT_LONG_WRITE_AS_LIST);
         CsrBtGattFlatDbRegisterHandleRangeReqSend(gattId, startHandle, endHandle);
     }
    return ascsServiceHandle;
}

/****************************************************************************/
void GattAscsServerConfigureCodecRequest(ServiceHandle serviceHandle, GattAscsServerConfigureCodecReq* configureCodecRequest)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (configureCodecRequest->cid == 0)
    {
        return;
    }

    ascsHandleCodecConfiguredRequest(ascs, configureCodecRequest);
}

/****************************************************************************/
void GattAscsServerConfigureCodecResponse(ServiceHandle serviceHandle, GattAscsServerConfigureCodecRsp* configureCodecResponse)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (configureCodecResponse->cid == 0)
    {
        return;
    }

    ascsHandleCodecConfiguredResponse(ascs, configureCodecResponse);

}

/****************************************************************************/
void GattAscsServerConfigureQosRequest(ServiceHandle serviceHandle, GattAscsServerConfigureQosReq* configureQosRequest)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (configureQosRequest->cid == 0)
    {
        return;
    }
    ascsHandleConfigureQosRequest(ascs, configureQosRequest);
}

/****************************************************************************/
void GattAscsServerConfigureQosResponse(ServiceHandle serviceHandle, GattAscsServerConfigureQosRsp* configureQosResponse)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (configureQosResponse->cid == 0)
    {
        return;
    }
    ascsHandleConfigureQosResponse(ascs, configureQosResponse);

}

void GattAscsServerEnableResponse(ServiceHandle serviceHandle, GattAscsServerEnableRsp* enableResponse)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (enableResponse->cid == 0)
    {
        return;
    }
    ascsHandleEnableResponse(ascs, enableResponse);
}

void GattAscsServerUpdateMetadataRequest(ServiceHandle serviceHandle,
                                         GattAscsServerUpdateMetadataReq* updateMetadataReq)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (updateMetadataReq->cid == 0)
    {
        return;
    }
    ascsHandleUpdateMetadataRequest(ascs, updateMetadataReq);
}

void GattAscsReceiverReadyRequest(ServiceHandle serviceHandle, GattAscsServerReceiverReadyReq* receiverReadyRequest)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (receiverReadyRequest->cid == 0)
    {
        return;
    }
    ascsHandleReceiverReadyRequest(ascs, receiverReadyRequest);
}

void GattAscsReceiverReadyResponse(ServiceHandle serviceHandle, GattAscsServerReceiverReadyRsp* receiverReadyResponse)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (receiverReadyResponse->cid == 0)
    {
        return;
    }
    ascsHandleReceiverReadyResponse(ascs, receiverReadyResponse);
}

void GattAscsServerUpdateMetadataResponse(ServiceHandle serviceHandle, GattAscsServerUpdateMetadataRsp* updateMetadataResponse)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (updateMetadataResponse->cid == 0)
    {
        return;
    }
    ascsHandleUpdateMetadataResponse(ascs, updateMetadataResponse);

}

void GattAscsServerDisableResponse(ServiceHandle serviceHandle, GattAscsServerDisableRsp* disableResponse)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (disableResponse->cid == 0)
    {
        return;
    }
    ascsHandleDisableResponse(ascs, disableResponse);

}

void GattAscsServerDisableRequest(ServiceHandle serviceHandle, GattAscsServerDisableReq* disableRequest, bool cisLoss)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (disableRequest->cid == 0)
    {
        return;
    }
    ascsHandleDisableRequest(ascs, disableRequest, cisLoss);
}

void GattAscsReceiverStopReadyResponse(ServiceHandle serviceHandle, GattAscsServerReceiverStopReadyRsp* receiverStopReadyResponse)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (receiverStopReadyResponse->cid == 0)
    {
        return;
    }
    ascsHandleReceiverStopReadyResponse(ascs, receiverStopReadyResponse);
}

void GattAscsServerReleaseCompleteRequest(ServiceHandle serviceHandle, GattAscsServerReleaseComplete* releaseComplete)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (releaseComplete->cid == 0)
    {
        return;
    }
    ascsHandleReleaseComplete(ascs, releaseComplete);
}

void GattAscsServerReleaseRequest(ServiceHandle serviceHandle, GattAscsServerReleaseReq* releaseRequest)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    /* Validate the input parameters */
    if (releaseRequest->cid == 0)
    {
        return;
    }
    ascsHandleReleaseRequest(ascs, releaseRequest);
}


GattAscsServerConfigureCodecInfo* GattAscsReadCodecConfiguration(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    GattAscsServerAse* ase = NULL;

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }

    ase = ascsServerFindAse(ascs, cid, aseId);
    if (ase && ase->dynamicData)
    {
        return &ase->dynamicData->cachedConfigureCodecInfo;
    }
    return NULL;
}

GattAscsAseDirection GattAscsReadAseDirection(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    GattAscsServerAse* ase = NULL;

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }

    ase = ascsServerFindAse(ascs, cid, aseId);
    if (ase)
    {
        return GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase);
    }
    return GATT_ASCS_ASE_DIRECTION_SERVER_UNINITIALISED;
}

GattAscsServerConfigureQosInfo* GattAscsReadQosConfiguration(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    GattAscsServerAse* ase = NULL;

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    ase = ascsServerFindAse(ascs, cid, aseId);

    if (ase &&
        ase->dynamicData)
    {
        return ase->dynamicData->cachedConfigureQosInfoFromServer;
    }
    return NULL;
}


GattAscsServerReleasingAseInfo* gattAscsFindAseIdsByCisIdAndState(ServiceHandle serviceHandle, ConnectionId cid,
                uint8 cisId, GattAscsServerAseState state)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    uint8 count = 0;
    uint8 aseIds[MAX_ASE_ID_PER_CIS] = {0};
    GattAscsServerReleasingAseInfo *aseIdsInfo = NULL;
    if (ascs)
    {
        GattAscsConnection* connection = ascsFindConnection(ascs, cid);

        if (connection)
        {
            int i;
            
            for (i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
            {
                if ((connection->ase[i].state == state) &&
                    connection->ase[i].dynamicData &&
                    connection->ase[i].dynamicData->cachedConfigureQosInfoFromServer &&
                    (connection->ase[i].dynamicData->cachedConfigureQosInfoFromServer->cisId == cisId))
                {
                    if(count < MAX_ASE_ID_PER_CIS)
                        aseIds[count++] = connection->ase[i].aseId;
                }
            }
        }

        if(count)
        {
            aseIdsInfo = zpmalloc(sizeof(GattAscsServerReleasingAseInfo));
            if(aseIdsInfo)
            {
                aseIdsInfo->numAses = (count > MAX_ASE_ID_PER_CIS)? MAX_ASE_ID_PER_CIS : count;
                SynMemCpyS(aseIdsInfo->aseIds, MAX_ASE_ID_PER_CIS, aseIds, aseIdsInfo->numAses);
            }
        }
    }
    else
    {
        gattAscsServerPanic();
    }
    return aseIdsInfo;
}

GattAscsServerReleasingAseInfo* GattAscsReadReleasingAseIdsByCisId(ServiceHandle serviceHandle, ConnectionId cid, uint8 cisId)
{
    return gattAscsFindAseIdsByCisIdAndState(serviceHandle, cid, cisId, GATT_ASCS_SERVER_ASE_STATE_RELEASING);
}

#ifdef INSTALL_ASCS_NOTIFY_CB
void GattAscsServerRegisterCallback( GattAscsAseNtfHook aseNotificationCb)
{
    GattAscsAseNtfCb = aseNotificationCb;
}
#endif /* INSTALL_ASCS_NOTIFY_CB */

uint8* GattAscsServerGetAseData(ServiceHandle serviceHandle, ConnectionId cid, uint8 aseId,  uint8* dataLength)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(serviceHandle);
    GattAscsServerAse* ase = NULL;

    if (ascs == NULL)
    {
        gattAscsServerPanic();
    }
    ase = ascsServerFindAse(ascs, cid, aseId);

    if (ase)
    {
        return ascsAseConstructCharacteristicValue(ase, dataLength, NULL);
    }
    return NULL;
}

void gatt_ascs_server_task_init(void** gash)
{
    *gash = &ascsServiceHandle;
    GATT_ASCS_SERVER_INFO("\nASCS: gatt_ascs_server_task_init\n");
}
#ifdef ENABLE_SHUTDOWN
void gatt_ascs_server_task_deinit(void** gash)
{
    ServiceHandle serviceHandle = *((ServiceHandle*)*gash);

    if (serviceHandle)
    {
        if(ServiceHandleFreeInstanceData(serviceHandle))
        {
            GATT_ASCS_SERVER_INFO("ASCS: gatt_ascs_server_task_deinit\n\n");
        }
        else
        {
            GATT_ASCS_SERVER_PANIC("ASCS: deinit - Unable to free ASCS server instance.\n");
        }
    }
    else
    {
        GATT_ASCS_SERVER_INFO("ASCS: deinit - Invalid Service Handle\n\n");
    }
}
#endif
