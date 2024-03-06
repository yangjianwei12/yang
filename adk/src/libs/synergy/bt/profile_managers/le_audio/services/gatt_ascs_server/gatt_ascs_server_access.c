/* Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_access.h"
#include "gatt_ascs_server_db.h"
#include "gatt_ascs_server_debug.h"
#include "gatt_ascs_server_msg_handler.h"
#include "bap_server_lib.h"

#include <stdlib.h>

#define LTV_LENGTH_OFFSET       0
#define LTV_TYPE_OFFSET         1

typedef struct
{
    uint8* ltvStart;
} LTV;

#define LTV_INITIALISE(ltv, buffer)  ((ltv)->ltvStart = (buffer))

#define LTV_NEXT(ltv) ((ltv)->ltvStart + LTV_LEN(ltv))

/* NOTE: The '+ 1' is necessary because the LTV length (the 'L' value stored within the LTV) does not include the
 *       length of the length field itself (i.e. 1 octet) */
#define LTV_LEN(ltv)   ((ltv)->ltvStart[LTV_LENGTH_OFFSET] + 1)

#define LTV_TYPE(ltv)  ((ltv)->ltvStart[LTV_TYPE_OFFSET])

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE           (2)

static void ascsAseControlPointAccess(GattAscsServer *ascsServer, const CsrBtGattAccessInd *accessInd);

static void ascsServerSendConfigChangeIndication(GattAscsServer  *const ascsServer,
    ConnectionId cid,
    bool configChangeComplete)
{
    GattAscsServerConfigChangeInd* message = (GattAscsServerConfigChangeInd*)CsrPmemZalloc((sizeof(GattAscsServerConfigChangeInd)));

    message->id = GATT_ASCS_SERVER_CONFIG_CHANGE_IND;
    message->cid = cid;
    message->configChangeComplete = configChangeComplete;

    AscsServerMessageSend(ascsServer->appTask, message);
}

static bool ascsServerAllClientConfigWritten(GattAscsServer *ascsServer,
    connection_id_t cid)
{
    uint8 i;
    GattAscsConnection* connection;

    connection = ascsFindConnection(ascsServer, cid);

    if (connection == NULL)
    {
        GATT_ASCS_SERVER_PANIC("No connection found\n");
    }


    if(connection->aseControlPointNotify.clientCfg != GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG)
    {
        for (i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
        {
            if (connection->ase[i].clientCfg == GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG)
                return FALSE;
        }
        return TRUE;
    }


    return FALSE;
}

static void ascsServerSetClientConfigWrite(GattAscsServer *ascsServer,
    connection_id_t cid,
    bool clientConfigChanged)
{
    bool configChangeComplete = ascsServerAllClientConfigWritten(ascsServer, cid);

    if (clientConfigChanged)
    {
        /* Inform above layer about CCCD change*/
        ascsServerSendConfigChangeIndication(ascsServer,
                                          cid,
                                          configChangeComplete);
    }
}


static bool ascsServerClientConfigChanged(uint16 clientCfg, uint8 newClientCfg)
{
    /* Check if the client config has changed, to notify above layer */
    if(((uint8)clientCfg) != newClientCfg)
        return TRUE;

    return FALSE;
}


/***************************************************************************
NAME
    ascsStoreConfigureCodecClientInfo

DESCRIPTION
    Stores the relevant data from the Configure Codec operation into the ase structure.
*/
static void ascsStoreConfigureCodecClientInfo(GattAscsConnection* connection, GattAscsServerConfigureCodecInd* message)
{
    int i;
    const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
    for (i = 0; i < numAses; i++)
    {
        GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->gattAscsServerConfigureCodecClientInfo[i].aseId);

        if (ase)
        {
            if(!ase->dynamicData)
            {
                ase->dynamicData = zpnew(AscsAseDynamicData);
            }
            ase->dynamicData->cachedConfigureCodecInfo.codecId  =      message->gattAscsServerConfigureCodecClientInfo[i].codecId;
            ase->dynamicData->cachedConfigureCodecInfo.targetLatency = message->gattAscsServerConfigureCodecClientInfo[i].targetLatency;
            ase->dynamicData->cachedConfigureCodecInfo.targetPhy =     message->gattAscsServerConfigureCodecClientInfo[i].targetPhy;
            /*
             * NOTE: The codecConfiguration and codecConfigurationLength are not stored by this function, they are
             *       stored when GattAscsServerConfigureCodecResponse() is called
             *
            */
        }
    }
}

/* NOTE: This function is called during the handling of a Configure Qos operation.
 *       It is not used to lookup/find any of the ases in the Configure Qos
 *       operation itself; it is used to lookup/find any _other_ ases that are
 *       already using the same direction, cigId and cisId that are specified in the
 *       Configure Qos operation.
 */
static GattAscsServerAse* ascsConnectionFindAseByCisInfo(GattAscsConnection* connection,
                                                              GattAscsAseDirection direction,
                                                              uint8 cigId,
                                                              uint8 cisId)
{
    int i;
    for (i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
    {
        GattAscsServerAse* ase = &connection->ase[i];

        if ( ase &&
            (GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase) == direction) &&
             ase->dynamicData &&
             ase->dynamicData->cachedConfigureQosInfoFromServer &&
            (ase->dynamicData->cachedConfigureQosInfoFromServer->cigId == cigId) &&
            (ase->dynamicData->cachedConfigureQosInfoFromServer->cisId == cisId))
        {
            return ase;
        }
    }
    return NULL;
}

static bool ascsInvalidParameterValueCheck(GattAscsConnection* connection,
                                       uint8 aseId,
                                       uint32 value,
                                       uint32 min,
                                       uint32 max,
                                       uint8 reasonCode)
{
    if ((value >= min) && (value <= max))
    {
        return TRUE;
    }
    else
    {
        ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                              aseId,
                                                                              reasonCode);
        return FALSE;
    }
}

static bool ascsRejectParameterValueCheck(GattAscsConnection* connection,
                                            uint8 aseId,
                                            uint32 value,
                                            uint32 min,
                                            uint32 max,
                                            uint8 reasonCode)
{
    if ((value >= min) && (value <= max))
    {
        return TRUE;
    }
    else
    {
        ascsAseControlPointCharacteristicAddRejectedParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                               aseId,
                                                                               reasonCode);
        return FALSE;
    }
}


/***************************************************************************
NAME
    ascsValidateAseId

DESCRIPTION
    Validates the ASE id by verifying that the id is known to the ASCS server.
*/
static bool ascsConnectionValidateAseId(GattAscsConnection* connection,
                                        uint8 aseId,
                                        GattAscsServerAse** ase)
{
    if (connection == NULL)
        return FALSE;

    *ase = ascsConnectionFindAse(connection, aseId);
    if ( *ase == NULL )
    {
        /* Set the Ase Control Point Characteristic response code to let the client know that this is an invalid ase id  */
        ascsAseControlPointCharacteristicAddInvalidAseIdResponseCode(&connection->aseControlPointNotify,
                                                                     aseId);
        return FALSE;
    }

    return TRUE;
}

static bool ascsConnectionValidateStateTransition(GattAscsConnection* connection, GattAscsServerAse* ase, uint8 opCode)
{
    bool result = FALSE;

    switch (ase->state)
    {
    case GATT_ASCS_SERVER_ASE_STATE_IDLE:
        switch (opCode)
        {
        case GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE:
            result = TRUE;
            break;
        };
        break;
    case GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED:
        switch (opCode)
        {
        case GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE:
        case GATT_ASCS_SERVER_CONFIG_QOS_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;
    case GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED:
        switch (opCode)
        {
        case GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE:
        case GATT_ASCS_SERVER_CONFIG_QOS_OPCODE:
        case GATT_ASCS_SERVER_ENABLE_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;

    case GATT_ASCS_SERVER_ASE_STATE_ENABLING:
        switch (opCode)
        {
        case GATT_ASCS_SERVER_UPDATE_METADATA_OPCODE:
        case GATT_ASCS_SERVER_RECEIVER_START_READY_OPCODE:
        case GATT_ASCS_SERVER_DISABLE_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;
    case GATT_ASCS_SERVER_ASE_STATE_STREAMING:
        switch (opCode)
        {
        case GATT_ASCS_SERVER_UPDATE_METADATA_OPCODE:
        case GATT_ASCS_SERVER_DISABLE_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;
    case GATT_ASCS_SERVER_ASE_STATE_DISABLING:
        switch (opCode)
        {
        case GATT_ASCS_SERVER_RECEIVER_STOP_READY_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;
    /*
     * The GATT_ASCS_SERVER_STATE_RELEASING state accepts the 'released' operation.
     * This is not an opCode that comes from the client and is not handled in this function
     * case GATT_ASCS_SERVER_STATE_RELEASING:
     * break;
     */
    default:
        {
            break;
        }
    };

    if ( ! result)
    {
        /* Set the Ase Control Point Characteristic response code to let the client know that this is an invalid state transition */
        ascsAseControlPointCharacteristicAddInvalidTransitionResponseCode(&connection->aseControlPointNotify,
                                                                          ase->aseId);

    }
    return result;
}

/***************************************************************************
NAME
    ascsValidateConfigureCodecInfo

DESCRIPTION
    Validates the fields in the configure codec operation received from the client.
*/
static bool ascsConnectionValidateConfigureCodecInfo(GattAscsConnection* connection,
                                           GattAscsServerConfigureCodecClientInfo* gattAscsServerConfigureCodecClientInfo)
{
    GattAscsServerAse* ase;
    bool result;

    /*
     * Make the initial assumption that the ASE CP response should indicate 'success',
     * this will be overwritten if an error is found later during processing of this configure qos operation
     */
    ascsAseControlPointCharacteristicAddSuccessResponseCode(&connection->aseControlPointNotify,
                                                            gattAscsServerConfigureCodecClientInfo->aseId);

    /* Make sure the ASE id is known to this ASCS server */
    result = ascsConnectionValidateAseId(connection,
                                           gattAscsServerConfigureCodecClientInfo->aseId,
                                           &ase);

    if (result)
    {
        /* Make sure the codec id is valid */
        if (gattAscsServerConfigureCodecClientInfo->codecId.codingFormat != GATT_ASCS_CODEC_VENDOR_SPECIFIC_CODING_FORMAT)
        {
            /* If the coding format is NOT vendor specific then the vendor specific company id and
             * vendor specific codec id shall both be 0x0000
             */
            if ((gattAscsServerConfigureCodecClientInfo->codecId.companyId != GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_COMPANY_ID)
                 ||
                (gattAscsServerConfigureCodecClientInfo->codecId.vendorSpecificCodecId != GATT_ASCS_CODEC_NON_VENDOR_SPECIFIC_CODEC_ID))
            {
                ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                      gattAscsServerConfigureCodecClientInfo->aseId,
                                                                                      GATT_ASCS_ASE_REASON_CODEC_ID);
                result = FALSE;
            }

        }
    }

    if (result)
    {
        if ((gattAscsServerConfigureCodecClientInfo->targetLatency < GATT_ASCS_TARGET_LATENCY_TARGET_LOWER_LATENCY) ||
            (gattAscsServerConfigureCodecClientInfo->targetLatency > GATT_ASCS_TARGET_LATENCY_TARGET_HIGHER_RELIABILITY))
        {
            /* Set the Ase Control Point Characteristic response code to let the client know something is wrong - need to update spec to
               allow more specific error code */
            ascsAseControlPointCharacteristicAddUnspecifiedErrorResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureCodecClientInfo->aseId);
            result = FALSE;
        }
    }

    if (result)
    {
        if ((gattAscsServerConfigureCodecClientInfo->targetPhy < GATT_ASCS_TARGET_PHY_LE_1M_PHY) ||
            (gattAscsServerConfigureCodecClientInfo->targetPhy > GATT_ASCS_TARGET_PHY_LE_CODED_PHY))
        {
            /* Set the Ase Control Point Characteristic response code to let the client know something is wrong - need to update spec to
               allow more specific error code */
            ascsAseControlPointCharacteristicAddUnspecifiedErrorResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureCodecClientInfo->aseId);
            result = FALSE;
        }
    }
    /* NOTE: From the ASCS d09r05 spec there doesn't seem to be any feasible way of validating the codecId parameter */

    if (result && ase)
    {
        result = ascsConnectionValidateStateTransition(connection, ase, GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE);
    }

    return result;
}


/***************************************************************************
NAME
    ascsValidateConfigureQosInfo

DESCRIPTION
    Validates the fields in the configure QOS operation received from the client.
*/
static bool ascsValidateConfigureQosInfo(GattAscsConnection* connection,
                                         GattAscsServerConfigureQosInfoWithAseId* gattAscsServerConfigureQosIndInfo)
{
    bool result = FALSE;
    GattAscsServerAse* ase;
    GattAscsServerConfigureCodecServerReqInfo ascsServerConfigureCodecServerInfo = {0};

    /*
     * Make the initial assumption that the ASE CP response should indicate 'success',
     * this will be overwritten if an error is found later during processing of this configure qos operation
     */
    ascsAseControlPointCharacteristicAddSuccessResponseCode(&connection->aseControlPointNotify,
                                                            gattAscsServerConfigureQosIndInfo->aseId);

    result = ascsConnectionValidateAseId(connection,
                                           gattAscsServerConfigureQosIndInfo->aseId,
                                           &ase);

    if (result && ase)
    {
        result = ascsConnectionValidateStateTransition(connection, ase, GATT_ASCS_SERVER_CONFIG_QOS_OPCODE);
    }

    /* From ASCS d09r05: the valid range is from 0x000000FF to 0x00FFFFFF inclusive */
    if (result)
    {
        result = ascsInvalidParameterValueCheck(connection,
                                                gattAscsServerConfigureQosIndInfo->aseId,
                                                gattAscsServerConfigureQosIndInfo->qosInfo->sduInterval,
                                                GATT_ASCS_SDU_INTERVAL_MIN,
                                                GATT_ASCS_SDU_INTERVAL_MAX,
                                                GATT_ASCS_ASE_REASON_SDU_INTERVAL);
    }

    if (result && ase)
    {
        /* No two ases can use the same CIS (identified by: connection, direction, cigId and cisId).
         * If an ase is already using a given CIS, then a Configure Qos operation requesting for
         * another ase to use that same CIS must be rejected with an 'invalid parameter' response.
         */
        GattAscsServerAse* clashingAse = ascsConnectionFindAseByCisInfo(connection,
                                                                              GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase),
                                                                              gattAscsServerConfigureQosIndInfo->qosInfo->cigId,
                                                                              gattAscsServerConfigureQosIndInfo->qosInfo->cisId);
        if ( clashingAse &&       /* is there a clashing ASE? */
            (clashingAse != ase)) /* multiple Configure Qos ops for the same ASE (with the same CIS info) are allowed */
        {
            /* In these states the CIS id / CIG id ARE configured in the ASE and must not clash with
             * CIS id / CIG id values in any other ASEs */
            result = FALSE;
            ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                  ase->aseId,
                                                                                  GATT_ASCS_ASE_REASON_INVALID_ASE_CIS_MAPPING);
        }
    }

    if (result && ase)
    {
        BapServerUnicastPopulateConfigureCodecData(connection->cid, ase->aseId, &ascsServerConfigureCodecServerInfo);

        /* Verify that the 'framing' value we received from the client in the configure qos op
         * is consistent with the 'framing' value that we previously sent to the client in
         * the codec configured notification.
         */
        /*
         * Note: The ASCS Validation r05 states that we shall not use 'unsupported parameter value'
         *       to reject the framing parameter, so we use 'invalid parameter value' if the framing value
         *       sent by the client doesn't correspond to the value we sent in the codec configured
         *       notification (even if the value itself is valid according to the spec)
         */
        if (gattAscsServerConfigureQosIndInfo->qosInfo->framing == GATT_ASCS_FRAMING_UNFRAMED_ISOAL_PDUS &&
            ascsServerConfigureCodecServerInfo.framing == GATT_ASCS_CODEC_CONFIGURED_FRAMING_UNFRAMED_ISOAL_PDUS_NOT_SUPPORTED)
        {
            ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureQosIndInfo->aseId,
                                                                                  GATT_ASCS_ASE_REASON_FRAMING);
            result = FALSE;
        }

        /* Verify that the 'framing' value adheres to the ASCS Validation r05 spec. */
        switch (gattAscsServerConfigureQosIndInfo->qosInfo->framing)
        {
        case GATT_ASCS_FRAMING_UNFRAMED_ISOAL_PDUS:
            /* fall through */
        case GATT_ASCS_FRAMING_FRAMED_ISOAL_PDUS:
            /* Nothing to do: the framing value is valid */
            break;
        default:
            ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureQosIndInfo->aseId,
                                                                                  GATT_ASCS_ASE_REASON_FRAMING);
            result = FALSE;
        };
    }

    if (result)
    {
        if ((gattAscsServerConfigureQosIndInfo->qosInfo->phy < GATT_ASCS_PHY_1M_BITS_PER_SECOND) ||
            (gattAscsServerConfigureQosIndInfo->qosInfo->phy > (GATT_ASCS_PHY_LE_CODED_PHY |
             GATT_ASCS_PHY_2M_BITS_PER_SECOND | GATT_ASCS_PHY_1M_BITS_PER_SECOND)))
        {
            ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureQosIndInfo->aseId,
                                                                                  GATT_ASCS_ASE_REASON_PHY);
            result = FALSE;
        }
    }

    /* From ASCS d09r05: the valid range is from 0x0000 to 0x0FFF inclusive */
    if (result)
    {
        result = ascsInvalidParameterValueCheck(connection,
                                                gattAscsServerConfigureQosIndInfo->aseId,
                                                gattAscsServerConfigureQosIndInfo->qosInfo->maximumSduSize,
                                                GATT_ASCS_MAXIMUM_SDU_SIZE_MIN,
                                                GATT_ASCS_MAXIMUM_SDU_SIZE_MAX,
                                                GATT_ASCS_ASE_REASON_MAXIMUM_SDU_SIZE);
    }

    /* From ASCS d09r05: the valid range is from 0x00 to 0x0F inclusive */
    if (result)
    {
        result = ascsInvalidParameterValueCheck(connection,
                                                gattAscsServerConfigureQosIndInfo->aseId,
                                                gattAscsServerConfigureQosIndInfo->qosInfo->retransmissionNumber,
                                                GATT_ASCS_RETRANSMISSION_NUMBER_MIN,
                                                GATT_ASCS_RETRANSMISSION_NUMBER_MAX,
                                                GATT_ASCS_ASE_REASON_RETRANSMISSION_NUMBER);
    }

    /* From ASCS d09r05: the valid range is from 0x0005 to 0x0FA0 inclusive */
    if (result)
    {
        result = ascsInvalidParameterValueCheck(connection,
                                                gattAscsServerConfigureQosIndInfo->aseId,
                                                gattAscsServerConfigureQosIndInfo->qosInfo->maxTransportLatency,
                                                GATT_ASCS_MAX_TRANSPORT_LATENCY_MIN,
                                                GATT_ASCS_MAX_TRANSPORT_LATENCY_MAX,
                                                GATT_ASCS_ASE_REASON_MAX_TRANSPORT_LATENCY);
    }

    if (result && ase)
    {
        uint32 pDelayMin = ase->dynamicData->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin;
        /* GMAP special handling for Source PD value greater than BAP PD Max value */
        uint32 pDelayMax = (pDelayMin > ascsServerConfigureCodecServerInfo.presentationDelayMax)? pDelayMin : ascsServerConfigureCodecServerInfo.presentationDelayMax;
        result = ascsRejectParameterValueCheck(connection,
                                               gattAscsServerConfigureQosIndInfo->aseId,
                                               gattAscsServerConfigureQosIndInfo->qosInfo->presentationDelay,
                                               pDelayMin,
                                               pDelayMax,
                                               GATT_ASCS_ASE_REASON_PRESENTATION_DELAY);
    }

    return result;
}

static bool ascsConnectionValidateLTVs(GattAscsConnection* connection, uint8* buffer, uint8 length)
{
    LTV ltv;
    uint8* dataPtr = buffer;

    /* Step over each LTV */
    while ((dataPtr - buffer) < length)
    {
        LTV_INITIALISE(&ltv, dataPtr);

        dataPtr = LTV_NEXT(&ltv);

    }

    /* Verify that the combined length of all stepped over LTVs equals the length of the whole buffer (containing all LTVs) */
    if ((dataPtr - buffer) == length)
    {
        return TRUE;
    }
    else
    {
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    return FALSE;
}

/***************************************************************************
NAME
    ascsValidateEnableIndInfo

DESCRIPTION
    Validates the fields in the Enable operation received from the client.
*/
static bool ascsConnectionValidateEnableIndInfo(GattAscsConnection* connection,
                                                GattAscsServerEnableIndInfo* gattAscsServerEnableIndInfo)
{
    GattAscsServerAse* ase;
    bool result;

    /*
     * Make the initial assumption that the ASE CP response should indicate 'success',
     * this will be overwritten if an error is found later during processing of this configure qos operation
     */
    ascsAseControlPointCharacteristicAddSuccessResponseCode(&connection->aseControlPointNotify,
                                                            gattAscsServerEnableIndInfo->aseId);

    result =  ascsConnectionValidateAseId(connection, gattAscsServerEnableIndInfo->aseId, &ase);

    if (result && ase)
    {
        result = ascsConnectionValidateStateTransition(connection, ase, GATT_ASCS_SERVER_ENABLE_OPCODE);
    }

    if (result)
    {
        result = ascsConnectionValidateLTVs(connection,
                                              gattAscsServerEnableIndInfo->metadata,
                                              gattAscsServerEnableIndInfo->metadataLength);
    }

    return result;
}

/***************************************************************************
NAME
    ascsValidateUpdateMetadataIndInfo

DESCRIPTION
    Validates the fields in the Update Metadata operation received from the client.
*/
static bool ascsValidateUpdateMetadataIndInfo(GattAscsConnection* connection,
                                              GattAscsServerUpdateMetadataInfo* gattAscsServerUpdateMetadataIndInfo)
{
    GattAscsServerAse* ase;
    bool result;
    /*
     * Make the initial assumption that the ASE CP response should indicate 'success',
     * this will be overwritten if an error is found later during processing of this configure qos operation
     */
    ascsAseControlPointCharacteristicAddSuccessResponseCode(&connection->aseControlPointNotify,
                                                            gattAscsServerUpdateMetadataIndInfo->aseId);

    result = ascsConnectionValidateAseId(connection, gattAscsServerUpdateMetadataIndInfo->aseId, &ase);

    if (result && ase)
    {
        result = ascsConnectionValidateStateTransition(connection, ase, GATT_ASCS_SERVER_UPDATE_METADATA_OPCODE);
    }

    if (result)
    {
        result = ascsConnectionValidateLTVs(connection,
                                              gattAscsServerUpdateMetadataIndInfo->metadata,
                                              gattAscsServerUpdateMetadataIndInfo->metadataLength);
    }

    return result;
}

/***************************************************************************
NAME
    ascsSendConfigureCodecInd

DESCRIPTION
    Sends the Configure Codec indication to the profile/app.
*/
static void ascsSendConfigureCodecInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    int i;
    AccessIndIterator iter;
    uint8 accessIndNumAses;

    GattAscsConnection* connection = ascsFindConnection(ascsServer, accessInd->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    accessIndIteratorInitialise(&iter, accessInd);
    accessIndIteratorRead8(&iter); /* skip over the configureCodec op code */

    accessIndNumAses = accessIndIteratorRead8(&iter);
    if ((accessIndNumAses == 0) || (accessIndNumAses > GATT_ASCS_NUM_ASES_MAX))
    {
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
        /* The Ind will not be constructed, due to invalid number of ases
            requested by the client.
           The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
        return;
    }
    else
    {
    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerConfigureCodecInd,
                                  accessIndNumAses, sizeof(GattAscsServerConfigureCodecClientInfo));

    message->cid = accessInd->cid;
    message->numAses = 0;

    for (i = 0;
        (i < accessIndNumAses) && (iter.error == FALSE);
         i++)
    {
        GattAscsServerAse* ase;
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].aseId = accessIndIteratorRead8(&iter);
        ase = ascsConnectionFindAse(connection, message->gattAscsServerConfigureCodecClientInfo[message->numAses].aseId);
        if ( ase != NULL )
        {
            /* NULL ase is handled by ascsConnectionValidateConfigureCodecInfo */
            message->gattAscsServerConfigureCodecClientInfo[message->numAses].direction = GATT_ASCS_SERVER_GET_ASE_DIRECTION(ase);
        }
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].targetLatency = accessIndIteratorRead8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].targetPhy = accessIndIteratorRead8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecId.codingFormat = accessIndIteratorRead8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecId.companyId = accessIndIteratorRead16(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecId.vendorSpecificCodecId = accessIndIteratorRead16(&iter);

        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfigurationLength = accessIndIteratorRead8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfiguration =
            accessIndIteratorReadMultipleOctets(&iter, message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfigurationLength);

        if (ascsConnectionValidateConfigureCodecInfo(connection,
                                        &message->gattAscsServerConfigureCodecClientInfo[message->numAses]))
        {
            /* Keep the ASE data in the codecConfigInd (increment the numAses and start decoding the data for the next ASE in the accessInd) */
            message->numAses++;
        }
        else
        {
            /* We had to parse this ASE (to be in a position to successfully parse any subsequent ASEs), but the ASE
             * is not valid, so we are discarding it, but we still need to free any allocated codec specific configuration memory */
            free(message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfiguration);
            /* Avoid any possibility of double free */
            message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfiguration = NULL;
        }
    }

    if (iter.error == TRUE) /* This op is an invalid length */
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if (( iter.error == TRUE) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */
    {
        /* NOTE: message->numAses might not be 0 */
        const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
        for (i = 0; i < numAses; i++)
            free(message->gattAscsServerConfigureCodecClientInfo[i].codecConfiguration);

        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call */
        /* The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);

        free(message);
    }
    else
    {
        /* Store some of the ase data - this is only done after the entire message has been validated to be error free */
        ascsStoreConfigureCodecClientInfo(connection, message);

        /* Send the IND to the profile/app */
        message->id = GATT_ASCS_SERVER_CONFIGURE_CODEC_IND;
        AscsServerMessageSend(ascsServer->appTask,
                message
                );
    }
    }
}

/***************************************************************************
NAME
    ascsSendConfigureQosInd

DESCRIPTION
    Sends the Configure Qos indication to the profile/app.
*/
static void ascsSendConfigureQosInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    int i;
    AccessIndIterator iter;
    uint8 accessIndNumAses;
    GattAscsConnection* connection = ascsFindConnection(ascsServer, accessInd->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    accessIndIteratorInitialise(&iter, accessInd);
    accessIndIteratorRead8(&iter); /* skip over the configureQos op code */

    accessIndNumAses = accessIndIteratorRead8(&iter);
    if ((accessIndNumAses == 0) || (accessIndNumAses > GATT_ASCS_NUM_ASES_MAX))
    {
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
        /* The Ind will not be constructed, due to invalid number of ases
            requested by the client.
           The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
        return;
    }
    else
    {
    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerConfigureQosInd,
                                  accessIndNumAses, sizeof(GattAscsServerConfigureQosInfoWithAseId));

    message->cid = accessInd->cid;
    message->numAses = 0;

    for (i = 0;
         (i < accessIndNumAses) && (iter.error == FALSE);
         i++)
    {
        message->ase[message->numAses].qosInfo = zpnew(GattAscsServerConfigureQosInfo);
        message->ase[message->numAses].aseId = accessIndIteratorRead8(&iter);
        message->ase[message->numAses].qosInfo->cigId = accessIndIteratorRead8(&iter);
        message->ase[message->numAses].qosInfo->cisId = accessIndIteratorRead8(&iter);
        message->ase[message->numAses].qosInfo->sduInterval = accessIndIteratorRead24(&iter);
        message->ase[message->numAses].qosInfo->framing = accessIndIteratorRead8(&iter);
        message->ase[message->numAses].qosInfo->phy = accessIndIteratorRead8(&iter);
        message->ase[message->numAses].qosInfo->maximumSduSize = accessIndIteratorRead16(&iter);
        message->ase[message->numAses].qosInfo->retransmissionNumber = accessIndIteratorRead8(&iter);
        message->ase[message->numAses].qosInfo->maxTransportLatency = accessIndIteratorRead16(&iter);
        message->ase[message->numAses].qosInfo->presentationDelay = accessIndIteratorRead24(&iter);

        if (ascsValidateConfigureQosInfo(connection,
                                         &message->ase[message->numAses]))
        {
            /* Keep the ASE data in the message (increment the numAses and start decoding the data for the next ASE in the accessInd) */
            message->numAses++;
        }
        else
        {
            /* The information for this ASE is not valid - discard it and continue processing any other ASEs in the ACCESS_IND */
            free(message->ase[message->numAses].qosInfo);
            /* Avoid any possibility of double free */
            message->ase[message->numAses].qosInfo = NULL;
        }
    }

    if (iter.error == TRUE)
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if ((iter.error == TRUE) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */

    {
        /* There is nothing useful to send to the profile/app, so just free the message */
        const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
        for (i = 0; i < numAses; i++)
        {
            free(message->ase[i].qosInfo);
        }
        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);

        free(message);
    }
    else
    {
        /* Update the ASE's internal data now that the ASCS operation has been validated */
        /*
         * TODO: Check behaviour when the following for loop is removed; from code inspection it appears the
         * cigId and cisId are set when the other (server supplied) info is set in ascsHandleConfigureQosResponse().
         * It also appears that these values are not accessed until after the call to ascsHandleConfigureQosResponse()
         * has returned.
         */
        const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
        for (i = 0; i < numAses; i++)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->ase[i].aseId);
            if (ase &&
                ase->dynamicData)
            {
                if (ase->dynamicData->cachedConfigureQosInfoFromServer)
                {
                    free(ase->dynamicData->cachedConfigureQosInfoFromServer);
                }
                ase->dynamicData->cachedConfigureQosInfoFromServer = zpnew(GattAscsServerConfigureQosInfo);

                ase->dynamicData->cachedConfigureQosInfoFromServer->cigId = message->ase[i].qosInfo->cigId;
                ase->dynamicData->cachedConfigureQosInfoFromServer->cisId = message->ase[i].qosInfo->cisId;
            }
        }
        /* Send the Configure QOS Indication to the profile/app */
        message->id = GATT_ASCS_SERVER_CONFIGURE_QOS_IND;
        AscsServerMessageSend(ascsServer->appTask,
                    message);
    }
    }
}

/***************************************************************************
NAME
    ascsSendEnableInd

DESCRIPTION
    Sends the Enable indication to the profile/app.
*/
static void ascsSendEnableInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    int i;
    AccessIndIterator iter;
    uint8 accessIndNumAses;
    GattAscsConnection* connection = ascsFindConnection(ascsServer, accessInd->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    accessIndIteratorInitialise(&iter, accessInd);
    accessIndIteratorRead8(&iter); /* skip over the configureQos op code */

    accessIndNumAses = accessIndIteratorRead8(&iter);
    if ((accessIndNumAses == 0) || (accessIndNumAses > GATT_ASCS_NUM_ASES_MAX))
    {
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
        /* The Ind will not be constructed, due to invalid number of ases
            requested by the client.
           The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
        return;
    }
    else
    {
    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerEnableInd,
                                  accessIndNumAses, sizeof(GattAscsServerEnableIndInfo));

    message->cid = accessInd->cid;
    message->numAses = 0;

    for (i = 0;
         (i < accessIndNumAses) && (iter.error == FALSE);
         i++)
    {
        message->gattAscsServerEnableIndInfo[message->numAses].aseId = accessIndIteratorRead8(&iter);
        message->gattAscsServerEnableIndInfo[message->numAses].metadataLength = accessIndIteratorRead8(&iter);
        message->gattAscsServerEnableIndInfo[message->numAses].metadata =
            accessIndIteratorReadMultipleOctets(&iter, message->gattAscsServerEnableIndInfo[message->numAses].metadataLength);

        if (ascsConnectionValidateEnableIndInfo(connection,
                                                  &message->gattAscsServerEnableIndInfo[message->numAses]))
        {
            /* Keep the ASE data in the message (increment the numAses and start decoding the data for the next ASE in the accessInd) */
            message->numAses++;
        }
        else
        {
            /* We had to parse this ASE (to be in a position to successfully parse any subsequent ASEs), but the ASE
             * is not valid, so we are discarding it, but we still need to free any allocated metadata memory */
            free(message->gattAscsServerEnableIndInfo[message->numAses].metadata);
            /* Avoid any possibility of double free */
            message->gattAscsServerEnableIndInfo[message->numAses].metadata = NULL;
        }
    }

    if (iter.error == TRUE)
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if ((iter.error == TRUE) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */

    {
        /* NOTE: message->numAses might not be 0 */
        const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
        for (i = 0; i < numAses; i++)
        {
            free(message->gattAscsServerEnableIndInfo[i].metadata);
        }
        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);

        /* There is nothing useful to send to the profile/app, so just free the message */
        free(message);
    }
    else
    {
        /* Only update the ASE's internal data after the complete ASCS operation has been validated (i.e. when we reach this branch) */
        const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
        for (i = 0; i < numAses; i++)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->gattAscsServerEnableIndInfo[i].aseId);
            if (ase &&
                ase->dynamicData &&
                ase->dynamicData->cachedConfigureQosInfoFromServer)
            {
                /* If we have metadata then this needs to be freed here */
                if (ase->dynamicData->metadataLength != 0)
                {
                    free(ase->dynamicData->metadata);
                    ase->dynamicData->metadata = NULL;
                    ase->dynamicData->metadataLength = 0;
                }
                ase->dynamicData->metadataLength = message->gattAscsServerEnableIndInfo[i].metadataLength;
                /* the server retains ownership of this memory; the API specifies that the profile/app will not free it*/
                ase->dynamicData->metadata = message->gattAscsServerEnableIndInfo[i].metadata;
                /*
                 * Set the cisId and cigId in the Enable Ind
                 */
                message->gattAscsServerEnableIndInfo[i].cisId = ase->dynamicData->cachedConfigureQosInfoFromServer->cisId;
                message->gattAscsServerEnableIndInfo[i].cigId = ase->dynamicData->cachedConfigureQosInfoFromServer->cigId;
            }
            else
            {
                GATT_ASCS_SERVER_ERROR("EnableInd: Ase not found or dynamic data not initialised, aseId=%d\n",
                                       message->gattAscsServerEnableIndInfo[i].aseId);
            }
        }
        message->id = GATT_ASCS_SERVER_ENABLE_IND;
        AscsServerMessageSend(
                ascsServer->appTask,
                message);
    }
    }
}

/***************************************************************************
NAME
    ascsConstructGenericInd

DESCRIPTION
    Constructs a generic indication message for indications that have a common format, e.g. Disable Ind, Release Ind.
*/
static GattAscsServerGenericInd* ascsConstructGenericInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    int i;
    AccessIndIterator iter;
    uint8 accessIndNumAses;
    uint8 accessIndOpCode;
    GattAscsConnection* connection = ascsFindConnection(ascsServer, accessInd->cid);

    if (connection == NULL)
    {
        return NULL;
    }

    accessIndIteratorInitialise(&iter, accessInd);
    accessIndOpCode = accessIndIteratorRead8(&iter);

    accessIndNumAses = accessIndIteratorRead8(&iter);
    if ((accessIndNumAses == 0) || (accessIndNumAses > GATT_ASCS_NUM_ASES_MAX))
    {
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
        /* The Ind will not be constructed, due to invalid number of ases
            requested by the client.
           The ASE CP Characteristic are set in responseCode and reason have been set above
           For ascsConstructGenericInd() the Notify is sent by the calling function (when this function returns NULL). */
        return NULL;
    }
    else
    {
    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerGenericInd,
                                  accessIndNumAses, sizeof(uint8));

    message->cid = accessInd->cid;
    message->numAses = 0;

    for (i = 0;
         (i < accessIndNumAses) && (iter.error == FALSE);
         i++)
    {
        GattAscsServerAse* ase;
        message->aseId[message->numAses] = accessIndIteratorRead8(&iter);
        /*
         * Make the initial assumption that the ASE CP response should indicate 'success',
         * this will be overwritten if an error is found later during processing of this configure qos operation
         */
        ascsAseControlPointCharacteristicAddSuccessResponseCode(&connection->aseControlPointNotify,
                                                                message->aseId[message->numAses]);

        if (ascsConnectionValidateAseId(connection,
                                        message->aseId[message->numAses],
                                        &ase)) /* if (the ASE id is valid) */
        {
            if ((ase) && ascsConnectionValidateStateTransition(connection, ase, accessIndOpCode))
            {
                /* Keep the ASE data in the message (increment the numAses and start decoding the data for the next ASE in the accessInd) */
                message->numAses++;
            }
        }
    }

    if (iter.error == TRUE)
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if((iter.error == TRUE) ||
       (message->numAses == 0))
    {
        free(message);
        message = NULL;
    }
    return message;
    }
}

/***************************************************************************
NAME
    ascsSendReceiverReadyInd

DESCRIPTION
    Sends the Receiver Ready indication to the profile/app.
*/
static void ascsSendReceiverReadyInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    GattAscsServerReceiverReadyInd* message = (GattAscsServerReceiverReadyInd*)ascsConstructGenericInd(ascsServer, accessInd);

    if (message)
    {
        message->id =  GATT_ASCS_SERVER_RECEIVER_READY_IND;
        AscsServerMessageSend(
                ascsServer->appTask,
                message
                );
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'responseCode' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
    }
}

/***************************************************************************
NAME
    ascsSendUpdateMetadataInd

DESCRIPTION
    Sends the Update Metadata indication to the profile/app.
*/
static void ascsSendUpdateMetadataInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    int i;
    AccessIndIterator iter;
    uint8 accessIndNumAses;

    GattAscsConnection* connection = ascsFindConnection(ascsServer, accessInd->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id*/
    }

    accessIndIteratorInitialise(&iter, accessInd);
    accessIndIteratorRead8(&iter); /* skip over the configureQos op code */

    accessIndNumAses = accessIndIteratorRead8(&iter);
    if ((accessIndNumAses == 0) || (accessIndNumAses > GATT_ASCS_NUM_ASES_MAX))
    {
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
        /* The Ind will not be constructed, due to invalid number of ases
            requested by the client.
           The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
        return;
    }
    else
    {
    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerUpdateMetadataInd,
                                  accessIndNumAses, sizeof(GattAscsServerUpdateMetadataInfo));

    message->cid = accessInd->cid;
    message->numAses = 0;

    for (i = 0;
         (i < accessIndNumAses) && (iter.error == FALSE);
         i++)
    {
        message->gattAscsServerUpdateMetadataIndInfo[message->numAses].aseId = accessIndIteratorRead8(&iter);
        message->gattAscsServerUpdateMetadataIndInfo[message->numAses].metadataLength = accessIndIteratorRead8(&iter);
        message->gattAscsServerUpdateMetadataIndInfo[message->numAses].metadata =
            accessIndIteratorReadMultipleOctets(&iter, message->gattAscsServerUpdateMetadataIndInfo[message->numAses].metadataLength);

        if (ascsValidateUpdateMetadataIndInfo(connection,
                                              &message->gattAscsServerUpdateMetadataIndInfo[message->numAses]))
        {
            message->numAses++;
        }
        else
        {
            free(message->gattAscsServerUpdateMetadataIndInfo[message->numAses].metadata);
        }
    }

    if (iter.error == TRUE)
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if ((iter.error == TRUE) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */
    {
        /* NOTE: message->numAses might not be 0 */
        const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
        for (i = 0; i < numAses; i++)
        {
            free(message->gattAscsServerUpdateMetadataIndInfo[i].metadata);
        }
        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic are set in responseCode and reason have been set above */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);

        /* There is nothing useful to send to the profile/app, so just free the message */
        free(message);
    }
    else
    {
        /* only update the ASEs internal data after the ASCS operation has been validated */
        const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
        for (i = 0; i < numAses; i++)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->gattAscsServerUpdateMetadataIndInfo[i].aseId);
            if (ase)
            {
                ase->dynamicData->metadataLength = message->gattAscsServerUpdateMetadataIndInfo[i].metadataLength;
                /* The ase will already have metadata from the Enable Ind operation, so free it before storing the new metadata */
                free(ase->dynamicData->metadata);
                /* the server retains ownership of this memory; the API specifies that the profile/app will not free it*/
                ase->dynamicData->metadata = message->gattAscsServerUpdateMetadataIndInfo[i].metadata;
            }
        }
        message->id = GATT_ASCS_SERVER_UPDATE_METADATA_IND;
        AscsServerMessageSend(ascsServer->appTask,message);
    }
    }
}

/***************************************************************************
NAME
    ascsSendDisableInd

DESCRIPTION
    Sends the Disable indication to the profile/app.
*/
static void ascsSendDisableInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    GattAscsServerDisableInd* message = (GattAscsServerDisableInd*)ascsConstructGenericInd(ascsServer, accessInd);

    if (message)
    {
        message->id = GATT_ASCS_SERVER_DISABLE_IND;
        AscsServerMessageSend(
                ascsServer->appTask,
                message);
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'responseCode' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
    }
}

/***************************************************************************
NAME
    ascsSendReceiverStopReadyInd

DESCRIPTION
    Sends the Receiver Stop Ready indication to the profile/app.
*/
static void ascsSendReceiverStopReadyInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    GattAscsServerReceiverStopReadyInd* message = (GattAscsServerReceiverStopReadyInd*)ascsConstructGenericInd(ascsServer, accessInd);

    if (message)
    {
        message->id = GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND;
        AscsServerMessageSend(
                ascsServer->appTask,
                message);
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'responseCode' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
    }
}

/***************************************************************************
NAME
    ascsSendReleaseInd

DESCRIPTION
    Sends the Release indication to the profile/app.
*/
static GattAscsServerReleaseInd* ascsSendReleaseInd(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    GattAscsServerReleaseInd* message = (GattAscsServerReleaseInd*)ascsConstructGenericInd(ascsServer, accessInd);

    if (message)
    {
        message->id = GATT_ASCS_SERVER_RELEASE_IND;
        AscsServerMessageSend(
                ascsServer->appTask,
                message
                );
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'responseCode' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascsServer, accessInd->cid);
    }

    return message;
}

/***************************************************************************
NAME
    ascsAseCharacteristicAccess

DESCRIPTION
    Deals with access of the HANDLE_ASCS_ASE_CHAR_x handles.
*/
static void ascsAseCharacteristicAccess(GattAscsServer* ascsServer,
                                        const CsrBtGattAccessInd* accessInd,
                                        GattAscsServerAse* ase)
{
    /* we only permit reads */
    if (accessInd->flags & ATT_ACCESS_WRITE)
    {
        CsrBtGattDbWriteAccessResSend(ascsServer->gattId,
                                      accessInd->cid,
                                      accessInd->handle,
                                      CSR_BT_GATT_ACCESS_RES_WRITE_NOT_PERMITTED);
        return;
    }

    /* handle read access only */
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        uint8 characteristicSize;
        uint8* characteristicValue;

        characteristicValue = ascsAseConstructCharacteristicValue(ase, &characteristicSize, NULL);

        if (characteristicValue)
        {
            CsrBtGattDbReadAccessResSend(ascsServer->gattId,
                                         accessInd->cid,
                                         accessInd->handle,
                                         CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                         characteristicSize,
                                         characteristicValue);
        }
        else
        {
            CsrBtGattDbReadAccessResSend(ascsServer->gattId,
                                         accessInd->cid,
                                         accessInd->handle,
                                         CSR_BT_GATT_ACCESS_RES_UNLIKELY_ERROR,
                                         0,
                                         NULL);
        }
    }
}

/***************************************************************************
NAME
    ascsHandleReadClientConfigAccess

DESCRIPTION
    Reads the client config.
*/
static void ascsHandleReadClientConfigAccess(
        CsrBtGattId gattId,
        ConnectionId cid,
        uint16 handle,
        uint16 clientConfig
        )
{
    uint8 configData[GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE];

    if (gattId == CSR_BT_GATT_INVALID_GATT_ID || cid == CSR_BT_CONN_ID_INVALID)
    {
        gattAscsServerPanic();
    }

     /* Default value of clientConfig is set as 0xFFFF. If client has not written
        any CCCD then we need to replace 0xFFFF with 0x0 (Disable) while
        responding. Default value is changed from 0 to 0xFFFF because of
        CCCD values getting lost if the device panics without these values are
        passed to application.
      */
    if(clientConfig != GATT_ASCS_SERVER_INVALID_CLIENT_CONFIG)
    {
        /* BASED on the spec this can only have one of the following three values:.
           GATT_ASCS_CLIENT_CONFIG_NOT_SET  0,
           GATT_ASCS_CLIENT_CONFIG_NOTIFY   1,
           GATT_ASCS_CLIENT_CONFIG_INDICATE 2
           NOTE:the only reason we use index 1 is for compliance in case of
           spec change.
        */
        configData[0] = (uint8)(clientConfig & 0xFF);
        configData[1] = (uint8)(clientConfig >> 8);
    }
    else
    {
        configData[0] = 0;
        configData[1] = 0;
    }

    sendAscsServerReadAccessRsp(
            gattId,
            cid,
            handle,
            CSR_BT_GATT_ACCESS_RES_SUCCESS,
            GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE,
            configData
            );
}

/***************************************************************************
NAME
    ascsHandleWriteClientConfigAccess

DESCRIPTION
    Writes the client config.
*/
static bool ascsHandleWriteClientConfigAccess(
        GattAscsServer *ascsServer,
        const CsrBtGattAccessInd *accessInd,
        uint16 *clientConfig
        )
{
    bool clientConfigChanged = FALSE;

    if (ascsServer->gattId== CSR_BT_GATT_INVALID_GATT_ID || accessInd->cid == CSR_BT_CONN_ID_INVALID)
    {
        gattAscsServerPanic();
    }


    if (accessInd->writeUnit[0].valueLength != GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE)
    {
        sendAscsServerWriteAccessRsp(ascsServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_INVALID_LENGTH,
                                0,
                                NULL);

        return FALSE;
    }

    switch (accessInd->writeUnit[0].value[0])
    {
    case GATT_ASCS_CLIENT_CONFIG_NOTIFY:
    case GATT_ASCS_CLIENT_CONFIG_NOT_SET:
    {
        clientConfigChanged = ascsServerClientConfigChanged(
                             *clientConfig,
                             accessInd->writeUnit[0].value[0]);

        /*GATT spec only uses 3 values for the client config: 0,1,2*/
        (*clientConfig) = (uint16) accessInd->writeUnit[0].value[0];

        /* Inform application for client write operation */
        ascsServerSetClientConfigWrite(ascsServer,
                                   accessInd->cid,
                              clientConfigChanged);
    }
    break;
    default:
    {
        /* ONLY Notify is allowed (INDICATE falls here)*/
        sendAscsServerWriteAccessRsp(ascsServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_CLIENT_CONFIG_IMPROPERLY_CONF,
                                0,
                                NULL);
    }
    return FALSE;
    }

    /* If not GATT_ASCS_CLIENT_CONFIG_NOTIFY send a generic respons without
        changing the client config */
    sendAscsServerWriteAccessRsp(ascsServer->gattId,
                            accessInd->cid,
                            accessInd->handle,
                            CSR_BT_GATT_ACCESS_RES_SUCCESS,
                            0,
                            NULL);

    return TRUE;
}

/***************************************************************************
NAME
    ascsHandleClientConfigAccess

DESCRIPTION
    Deals with access of the HANDLE_XXX_CLIENT_CONFIG_<number> handles.
*/
static void ascsHandleClientConfigAccess(GattAscsServer *ascsServer,
                                         const CsrBtGattAccessInd *accessInd,
                                         uint16* clientCfg)
{
    if (accessInd->flags & ATT_ACCESS_READ)
    {
        ascsHandleReadClientConfigAccess(
                    ascsServer->gattId,
                    accessInd->cid,
                    accessInd->handle,
                    *clientCfg
                    );
    }
    else if (accessInd->flags & ATT_ACCESS_WRITE)
    {
        ascsHandleWriteClientConfigAccess(
                        ascsServer,
                        accessInd,
                        clientCfg
                        );
    }
    else
    {
        sendAscsServerReadAccessRsp(ascsServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_SUCCESS,
                                0,
                                NULL);
    }
}

/***************************************************************************
NAME
    ascsAseControlPointAccess

DESCRIPTION
    Deals with the access of the HANDLE_ASCS_ASE_CONTROL_POINT_CHAR handle.
*/
static void ascsAseControlPointAccess(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    int i;
    if (accessInd->flags & ATT_ACCESS_WRITE)
    {
        GattAscsConnection* connection = ascsFindConnection(ascsServer, accessInd->cid);
        uint8 opcode;

        if (connection == NULL)
        {
            GATT_ASCS_SERVER_ERROR("There is no CONNECTION_T instance associated with the accessInd->cid value\n");
            return; /* This should never happen */
        }

        sendAscsServerWriteAccessRsp(ascsServer->gattId,
                               accessInd->cid,
                               accessInd->handle,
                               CSR_BT_GATT_ACCESS_RES_SUCCESS,
                               0,
                               NULL);

        if (accessInd->writeUnit == NULL)
        {
            return;
        }

        opcode = accessInd->writeUnit[0].value[0];  /* All ASE Control Point Access Reads start with: opcode (followed by numAses) */

        ascsAseControlPointCharacteristicReset(&connection->aseControlPointNotify, opcode);

        GATT_ASCS_SERVER_INFO("Control Point Access Opcode 0x%x",opcode);

        switch(opcode)
        {
            case GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE:
            {
                ascsSendConfigureCodecInd(ascsServer, accessInd);
            }
            break;

            case GATT_ASCS_SERVER_CONFIG_QOS_OPCODE:
            {
                ascsSendConfigureQosInd(ascsServer, accessInd);
            }
            break;

            case GATT_ASCS_SERVER_ENABLE_OPCODE:
            {
                ascsSendEnableInd(ascsServer, accessInd);
            }
            break;

            case GATT_ASCS_SERVER_RECEIVER_START_READY_OPCODE:
            {
                ascsSendReceiverReadyInd(ascsServer, accessInd);
            }
            break;

            case GATT_ASCS_SERVER_UPDATE_METADATA_OPCODE:
            {
                ascsSendUpdateMetadataInd(ascsServer, accessInd);
            }
            break;

            case GATT_ASCS_SERVER_DISABLE_OPCODE:
            {
                ascsSendDisableInd(ascsServer, accessInd);
            }
            break;

            case GATT_ASCS_SERVER_RECEIVER_STOP_READY_OPCODE:
            {
                ascsSendReceiverStopReadyInd(ascsServer, accessInd);
            }
            break;

            case GATT_ASCS_SERVER_RELEASE_OPCODE:
            {
                {
                    GattAscsServerReleaseInd* message = ascsSendReleaseInd(ascsServer, accessInd);

                    if (message)
                    {
                         const uint8 numAses = MIN(message->numAses, GATT_ASCS_NUM_ASES_MAX);
                        /*
                         * The ASE state changes to GATT_ASCS_SERVER_ASE_STATE_RELEASING without requiring a response
                         * from the profile/app - the code in this 'if' statement performs the actions that _would_
                         * have been done by an ascsHandleReleaseResponse function (if it were required).
                         *
                         * NOTE: The RELEASE_IND must be sent to the profile/app _before_ calling ascsServerAseStateNotify
                         *       (below) because ascsSendReleaseInd checks that GATT_ASCS_SERVER_RELEASE_OPCODE is
                         *       valid in the current ASE state, but ascsServerAseStateNotify _changes_ the ASE
                         *       state to GATT_ASCS_SERVER_ASE_STATE_RELEASING.
                         *
                         * NOTE: The profile/app is in a different context, so the handling of the RELEASE_IND
                         *       will always be after the ASE CP Notify and the ASE Notify(s) have been sent.
                         *
                         * NOTE: The profile/app calls GattAscsServerReleaseCompleteRequest() after receiving the
                         *       RELEASE_IND and after the underlying CIS has been cleared, this call transitions
                         *       the ASE(s) from the GATT_ASCS_SERVER_ASE_STATE_RELEASING state to the
                         *       GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED state.
                         */
                        ascsAseControlPointNotify(ascsServer, accessInd->cid);

                        for (i = 0; i < numAses; i++)
                        {
                            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->aseId[i]);
                            if (ase)
                            {
                                ascsServerSetAseStateAndNotify(ascsServer, message->cid, ase, GATT_ASCS_SERVER_ASE_STATE_RELEASING, NULL);
                            }
                        }
                    }
                }
            }
            break;

            default:
            {
                /* We don't recognise this opcode, set the ASE Control Point Notify response code accordingly */
                ascsAseControlPointCharacteristicAddUnsupportedOpcodeResponse(&connection->aseControlPointNotify);
                /* Send the ASE CP Notify */
                ascsAseControlPointNotify(ascsServer, accessInd->cid);
            }
            break;
        }

        /* NOTE: The ASE Control Point Notification is sent when the Response is received from the profile/app */
    }
    else
    {
        sendAscsServerReadAccessRsp(ascsServer->gattId,
                                accessInd->cid,
                                accessInd->handle,
                                CSR_BT_GATT_ACCESS_RES_REQUEST_NOT_SUPPORTED,
                                0,
                                NULL);
    }
}

/***************************************************************************/

void ascsAseControlPointNotify(const GattAscsServer* ascsServer, ConnectionId cid)
{
    uint8 characteristicLength;
    uint8* characteristicValue;
    GattAscsConnection* connection = ascsFindConnection(ascsServer, cid);

    if (connection)
    {
        characteristicValue = ascsAseControlPointConstructCharacteristicValue(&connection->aseControlPointNotify,
                                                                               &characteristicLength);

        if((connection->aseControlPointNotify.clientCfg != GATT_ASCS_CLIENT_CONFIG_NOTIFY) ||
            (characteristicValue == NULL))
        {
            free(characteristicValue);
            /* Notify is not enabled by the client. Server does not need to send
                any notifications to the client */
            return;
        }

        CsrBtGattNotificationEventReqSend(ascsServer->gattId,
                                          cid,
                                          HANDLE_ASCS_ASE_CONTROL_POINT_CHAR,
                                          characteristicLength,
                                          characteristicValue);
    }
}

void handleAscsAccess(GattAscsServer* ascsServer, const CsrBtGattAccessInd* accessInd)
{
    GattAscsConnection* connection = ascsFindConnection(ascsServer, accessInd->cid);

    if (connection == NULL)
    {
        return; /* We cannot process an access ind for a connection we do not recognise */
    }

    switch (accessInd->handle)
    {
        case HANDLE_ASCS_ASE_CONTROL_POINT_CHAR:
        {
            ascsAseControlPointAccess(ascsServer, accessInd);
        }
        break;

        case HANDLE_ASCS_ASE_CONTROL_POINT_CHAR_CLIENT_CONFIG:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->aseControlPointNotify.clientCfg);
        }
        break;
#ifdef HANDLE_ASCS_ASE_CHAR_1
        case HANDLE_ASCS_ASE_CHAR_1:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_1_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_1:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_1_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_2
        case HANDLE_ASCS_ASE_CHAR_2:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_2_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_2:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_2_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_3
        case HANDLE_ASCS_ASE_CHAR_3:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_3_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_3:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_3_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_4
        case HANDLE_ASCS_ASE_CHAR_4:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_4_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_4:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_4_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_5
        case HANDLE_ASCS_ASE_CHAR_5:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_5_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_5:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_5_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_6
        case HANDLE_ASCS_ASE_CHAR_6:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_6_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_6:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_6_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_7
        case HANDLE_ASCS_ASE_CHAR_7:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_7_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_7:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_7_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_8
        case HANDLE_ASCS_ASE_CHAR_8:
        {
            ascsAseCharacteristicAccess(ascsServer,
                                        accessInd,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_8_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_8:
        {
            ascsHandleClientConfigAccess(ascsServer,
                                         accessInd,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_8_IDX].clientCfg);
        }
        break;
#endif
        default:
        {
            /* Respond to invalid handles */
           sendAscsServerReadAccessRsp(
                    ascsServer->gattId,
                    accessInd->cid,
                    accessInd->handle,
                    CSR_BT_GATT_ACCESS_RES_INVALID_HANDLE,
                    0,
                    NULL);
        }
        break;
    }
}

void gattAscsServerPanic(void)
{
    GATT_ASCS_SERVER_PANIC("ASCS: Null instance\n");
}

