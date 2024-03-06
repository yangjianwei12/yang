/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_access.h"
#include "gatt_ascs_server_db.h"
#include "gatt_ascs_server_msg_handler.h"

#include <stdlib.h>

#define LTV_LENGTH_OFFSET       0
#define LTV_TYPE_OFFSET         1

enum
{
    GATT_ASCS_ASE_CHAR_1_IDX = 0,
    GATT_ASCS_ASE_CHAR_2_IDX = 1,
    GATT_ASCS_ASE_CHAR_3_IDX = 2
};

typedef struct
{
    uint8* ltv_start;
} LTV;

#define LTV_INITIALISE(ltv, buffer)  ((ltv)->ltv_start = (buffer))

#define LTV_NEXT(ltv) ((ltv)->ltv_start + LTV_LEN(ltv))

/* NOTE: The '+ 1' is necessary because the LTV length (the 'L' value stored within the LTV) does not include the
 *       length of the length field itself (i.e. 1 octet) */
#define LTV_LEN(ltv)   ((ltv)->ltv_start[LTV_LENGTH_OFFSET] + 1)

#define LTV_TYPE(ltv)  ((ltv)->ltv_start[LTV_TYPE_OFFSET])

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE           (2)

static void ascsAseControlPointAccess(GattAscsServer *ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

/***************************************************************************
NAME
    ascsStoreConfigureCodecClientInfo

DESCRIPTION
    Stores the relevant data from the Configure Codec operation into the ase structure.
*/
static void ascsStoreConfigureCodecClientInfo(GattAscsConnection* connection, GattAscsServerConfigureCodecInd* message)
{
    for (int i = 0; i < message->numAses; i++)
    {
        GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->gattAscsServerConfigureCodecClientInfo[i].aseId);

        if (ase)
        {
            ase->cachedConfigureCodecInfo.direction =      message->gattAscsServerConfigureCodecClientInfo[i].direction;
            ase->cachedConfigureCodecInfo.codecId  =      message->gattAscsServerConfigureCodecClientInfo[i].codecId;
            ase->cachedConfigureCodecInfo.targetLatency = message->gattAscsServerConfigureCodecClientInfo[i].targetLatency;
            ase->cachedConfigureCodecInfo.targetPhy =     message->gattAscsServerConfigureCodecClientInfo[i].targetPhy;
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
    for (int i = 0; i < connection->numAses; i++)
    {
        GattAscsServerAse* ase = &connection->ase[i];

        if ((ase->cachedConfigureCodecInfo.direction        == direction) &&
            (ase->cachedConfigureQosInfoFromServer.cigId == cigId) &&
            (ase->cachedConfigureQosInfoFromServer.cisId == cisId))
        {
            return ase;
        }
    }
    return NULL;
}

static bool ascsConnectionValidateLTVs(GattAscsConnection* connection, uint8* buffer, uint8 length)
{
    LTV ltv;
    uint8* data_ptr = buffer;

    /* Step over each LTV */
    while ((data_ptr - buffer) < length)
    {
        LTV_INITIALISE(&ltv, data_ptr);

        data_ptr = LTV_NEXT(&ltv);

    }

    /* Verify that the combined length of all stepped over LTVs equals the length of the whole buffer (containing all LTVs) */
    if ((data_ptr - buffer) == length)
    {
        return TRUE;
    }
    else
    {
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    return FALSE;
}

static bool ascsInvalidParameterValueCheck(GattAscsConnection* connection,
                                       uint8 aseId,
                                       uint32 value,
                                       uint32 min,
                                       uint32 max,
                                       uint8 reason_code)
{
    if ((value >= min) && (value <= max))
    {
        return TRUE;
    }
    else
    {
        ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                              aseId,
                                                                              reason_code);
        return FALSE;
    }
}

static bool ascsRejectParameterValueCheck(GattAscsConnection* connection,
                                            uint8 aseId,
                                            uint32 value,
                                            uint32 min,
                                            uint32 max,
                                            uint8 reason_code)
{
    if ((value >= min) && (value <= max))
    {
        return TRUE;
    }
    else
    {
        ascsAseControlPointCharacteristicAddRejectedParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                               aseId,
                                                                               reason_code);
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

static bool ascsConnectionValidateStateTransition(GattAscsConnection* connection, GattAscsServerAse* ase, uint8 op_code)
{
    bool result = FALSE;

    switch (ase->state)
    {
    case GATT_ASCS_SERVER_ASE_STATE_IDLE:
        switch (op_code)
        {
        case GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE:
            result = TRUE;
            break;
        };
        break;
    case GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED:
        switch (op_code)
        {
        case GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE:
        case GATT_ASCS_SERVER_CONFIG_QOS_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;
    case GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED:
        switch (op_code)
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
        switch (op_code)
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
        switch (op_code)
        {
        case GATT_ASCS_SERVER_UPDATE_METADATA_OPCODE:
        case GATT_ASCS_SERVER_DISABLE_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;
    case GATT_ASCS_SERVER_ASE_STATE_DISABLING:
        switch (op_code)
        {
        case GATT_ASCS_SERVER_RECEIVER_STOP_READY_OPCODE:
        case GATT_ASCS_SERVER_RELEASE_OPCODE:
            result = TRUE;
            break;
        };
        break;
    /*
     * The gatt_ascs_server_state_releasing statre accepts the 'released' operation.
     * This is not an op_code that comes from the client and is not handled in this function
     * case gatt_ascs_server_state_releasing:
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
        /* Make sure the direction is valid */
        if ((gattAscsServerConfigureCodecClientInfo->direction != GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK) &&
            (gattAscsServerConfigureCodecClientInfo->direction != GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE))
        {
            /* Set the Ase Control Point Characteristic response code to let the client know that this is an invalid direction */
            ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureCodecClientInfo->aseId,
                                                                                  GATT_ASCS_ASE_REASON_DIRECTION);
            result = FALSE;
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

    if (result)
    {
        result = ascsConnectionValidateLTVs(connection,
                                            gattAscsServerConfigureCodecClientInfo->codecConfiguration,
                                            gattAscsServerConfigureCodecClientInfo->codecConfigurationLength);
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
                                         GattAscsServerConfigureQosInfo* gattAscsServerConfigureQosIndInfo)
{
    bool result = FALSE;
    GattAscsServerAse* ase;

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
                                                gattAscsServerConfigureQosIndInfo->sduInterval,
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
        GattAscsServerAse* clashing_ase = ascsConnectionFindAseByCisInfo(connection,
                                                                              ase->cachedConfigureCodecInfo.direction,
                                                                              gattAscsServerConfigureQosIndInfo->cigId,
                                                                              gattAscsServerConfigureQosIndInfo->cisId);
        if (clashing_ase &&
            clashing_ase->qosInfoValid && /* Is the QOS info initialised and valid in the clashing ASE */
           (clashing_ase != ase)) /* multiple Configure Qos ops for the same ASE (with the same CIS info) are allowed */
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
        if (gattAscsServerConfigureQosIndInfo->framing == GATT_ASCS_FRAMING_UNFRAMED_ISOAL_PDUS &&
            ase->cachedConfigureCodecInfo.infoFromServer.framing == GATT_ASCS_CODEC_CONFIGURED_FRAMING_UNFRAMED_ISOAL_PDUS_NOT_SUPPORTED)
        {
            ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureQosIndInfo->aseId,
                                                                                  GATT_ASCS_ASE_REASON_FRAMING);
            result = FALSE;
        }

        /* Verify that the 'framing' value adheres to the ASCS Validation r05 spec. */
        switch (gattAscsServerConfigureQosIndInfo->framing)
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
        switch (gattAscsServerConfigureQosIndInfo->phy)
        {
        case GATT_ASCS_PHY_1Mbps:
            /* fall through */
        case GATT_ASCS_PHY_2Mbps:
            /* fall through */
        case GATT_ASCS_PHY_LE_CODED_PHY:
            /* Nothing to do: the phy value is valid */
            break;
        default:
            ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(&connection->aseControlPointNotify,
                                                                                  gattAscsServerConfigureQosIndInfo->aseId,
                                                                                  GATT_ASCS_ASE_REASON_PHY);
            result = FALSE;
        };
    }

    /* From ASCS d09r05: the valid range is from 0x0000 to 0x0FFF inclusive */
    if (result)
    {
        result = ascsInvalidParameterValueCheck(connection,
                                                gattAscsServerConfigureQosIndInfo->aseId,
                                                gattAscsServerConfigureQosIndInfo->maximumSduSize,
                                                GATT_ASCS_MAXIMUM_SDU_SIZE_MIN,
                                                GATT_ASCS_MAXIMUM_SDU_SIZE_MAX,
                                                GATT_ASCS_ASE_REASON_MAXIMUM_SDU_SIZE);
    }

    /* The max SDU size selected by the client must be less than or equal to the max value specified in the Configure Codec response */
    if (result)
    {
        if (ase)
        {
            result = ascsRejectParameterValueCheck(connection,
                                                   gattAscsServerConfigureQosIndInfo->aseId,
                                                   gattAscsServerConfigureQosIndInfo->maximumSduSize,
                                                   GATT_ASCS_MAXIMUM_SDU_SIZE_MIN,
                                                   ase->cachedConfigureCodecInfo.infoFromServer.maximumSduSize,
                                                   GATT_ASCS_ASE_REASON_SDU_INTERVAL);
        }
    }
    /* From ASCS d09r05: the valid range is from 0x00 to 0x0F inclusive */
    if (result)
    {
        result = ascsInvalidParameterValueCheck(connection,
                                                gattAscsServerConfigureQosIndInfo->aseId,
                                                gattAscsServerConfigureQosIndInfo->retransmissionNumber,
                                                GATT_ASCS_RETRANSMISSION_NUMBER_MIN,
                                                GATT_ASCS_RETRANSMISSION_NUMBER_MAX,
                                                GATT_ASCS_ASE_REASON_RETRANSMISSION_NUMBER);
    }

    /* From ASCS d09r05: the valid range is from 0x0005 to 0x0FA0 inclusive */
    if (result)
    {
        result = ascsInvalidParameterValueCheck(connection,
                                                gattAscsServerConfigureQosIndInfo->aseId,
                                                gattAscsServerConfigureQosIndInfo->maxTransportLatency,
                                                GATT_ASCS_MAX_TRANSPORT_LATENCY_MIN,
                                                GATT_ASCS_MAX_TRANSPORT_LATENCY_MAX,
                                                GATT_ASCS_ASE_REASON_MAX_TRANSPORT_LATENCY);
    }

    /* NOTE: There are no min/max values specified in ASCS d09r05 (the range is from 0x/0000/0000/ to 0x/FFFF/FFFF/ inclusive)
     *       However, the presentation delay selected by the client must be within the min/max values specified in the
     *       Configure Codec response
     */
    if (result)
    {
        if (ase)
        {
            result = ascsRejectParameterValueCheck(connection,
                                                   gattAscsServerConfigureQosIndInfo->aseId,
                                                   gattAscsServerConfigureQosIndInfo->presentationDelay,
                                                   ase->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin,
                                                   ase->cachedConfigureCodecInfo.infoFromServer.presentationDelayMax,
                                                   GATT_ASCS_ASE_REASON_PRESENTATION_DELAY);
        }
    }

    return result;
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
                                              GattAscsServerUpdateMetadataIndInfo* gattAscsServerUpdateMetadataIndInfo)
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
static void ascsSendConfigureCodecInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsBuffIteratorReadOnly iter;
    uint8 access_ind_numAses;

    GattAscsConnection* connection = ascsFindConnection(ascs_server, access_ind->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    ascsBuffIteratorInitialiseReadOnly(&iter, &access_ind->value[0], access_ind->size_value);
    ascsBuffIteratorSkipOctetsReadOnly(&iter, 1 /* size of opcode */); /* skip over the configure_codec op code */

    access_ind_numAses = ascsBuffIteratorGet8(&iter);

    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerConfigureCodecInd,
        (access_ind_numAses * sizeof(GattAscsServerConfigureCodecClientInfo)));

    message->cid = access_ind->cid;
    message->numAses = 0;

    for (int i = 0;
        (i < access_ind_numAses) && (! ascsBuffIteratorErrorDetected(&iter));
         i++)
    {
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].aseId = ascsBuffIteratorGet8(&iter);;
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].direction = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].targetLatency = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].targetPhy = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecId.codingFormat = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecId.companyId = ascsBuffIteratorGet16(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecId.vendorSpecificCodecId = ascsBuffIteratorGet16(&iter);

        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfigurationLength = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfiguration =
            ascsBuffIteratorGetMultipleOctets(&iter, message->gattAscsServerConfigureCodecClientInfo[message->numAses].codecConfigurationLength);

        if (ascsConnectionValidateConfigureCodecInfo(connection,
                                        &message->gattAscsServerConfigureCodecClientInfo[message->numAses]))
        {
            /* Keep the ASE data in the codec_config_ind (increment the numAses and start decoding the data for the next ASE in the access_ind) */
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

    if (ascsBuffIteratorErrorDetected(&iter)) /* This op is an invalid length */
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if ((ascsBuffIteratorErrorDetected(&iter)) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */
    {
        /* NOTE: message->numAses might not be 0 */
        for (int i = 0; i < message->numAses; i++)
            free(message->gattAscsServerConfigureCodecClientInfo[i].codecConfiguration);

        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call */
        /* The ASE CP Characteristic are set in response_code and reason have been set above */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);

        free(message);
    }
    else
    {
        /* Store some of the ase data - this is only done after the entire message has been validated to be error free */
        ascsStoreConfigureCodecClientInfo(connection, message);

        /* Send the IND to the profile/app */
        MessageSend(
                ascs_server->appTask,
                GATT_ASCS_SERVER_CONFIGURE_CODEC_IND,
                message
                );
    }
}

/***************************************************************************
NAME
    ascsSendConfigureQosInd

DESCRIPTION
    Sends the Configure Qos indication to the profile/app.
*/
static void ascsSendConfigureQosInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsBuffIteratorReadOnly iter;
    uint8 access_ind_numAses;
    GattAscsConnection* connection = ascsFindConnection(ascs_server, access_ind->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    ascsBuffIteratorInitialiseReadOnly(&iter, &access_ind->value[0], access_ind->size_value);
    ascsBuffIteratorSkipOctetsReadOnly(&iter, 1 /* size of opcode */); /* skip over the configure_qos op code */

    access_ind_numAses = ascsBuffIteratorGet8(&iter);

    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerConfigureQosInd,
        (access_ind_numAses * sizeof(GattAscsServerConfigureQosIndAse)));

    message->cid = access_ind->cid;
    message->numAses = 0;

    for (int i = 0;
         (i < access_ind_numAses) && (! ascsBuffIteratorErrorDetected(&iter));
         i++)
    {
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo =
            (GattAscsServerConfigureQosInfo *)PanicNull(
                calloc(1, sizeof(GattAscsServerConfigureQosInfo)));
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->aseId = ascsBuffIteratorGet8(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->cigId = ascsBuffIteratorGet8(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->cisId = ascsBuffIteratorGet8(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->sduInterval = ascsBuffIteratorGet24(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->framing = ascsBuffIteratorGet8(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->phy = ascsBuffIteratorGet8(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->maximumSduSize = ascsBuffIteratorGet16(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->retransmissionNumber = ascsBuffIteratorGet8(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->maxTransportLatency = ascsBuffIteratorGet16(&iter);
        message->ase[message->numAses].gattAscsServerConfigureQosIndInfo->presentationDelay = ascsBuffIteratorGet24(&iter);

        if (ascsValidateConfigureQosInfo(connection,
                                         message->ase[message->numAses].gattAscsServerConfigureQosIndInfo))
        {
            /* Keep the ASE data in the message (increment the numAses and start decoding the data for the next ASE in the access_ind) */
            message->numAses++;
        }
        else
        {
            /* The information for this ASE is not valid - discard it and continue processing any other ASEs in the ACCESS_IND */
            free(message->ase[message->numAses].gattAscsServerConfigureQosIndInfo);
            /* Avoid any possibility of double free */
            message->ase[message->numAses].gattAscsServerConfigureQosIndInfo = NULL;
        }
    }

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if ((ascsBuffIteratorErrorDetected(&iter)) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */

    {
        /* There is nothing useful to send to the profile/app, so just free the message */
        for (int i = 0; i < message->numAses; i++)
        {
            free(message->ase[i].gattAscsServerConfigureQosIndInfo);
        }
        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic are set in response_code and reason have been set above */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);

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
        for (int i = 0; i < message->numAses; i++)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->ase[i].gattAscsServerConfigureQosIndInfo->aseId);
            if (ase)
            {
                ase->cachedConfigureQosInfoFromServer.cigId = message->cid, message->ase[i].gattAscsServerConfigureQosIndInfo->cigId;
                ase->cachedConfigureQosInfoFromServer.cisId = message->cid, message->ase[i].gattAscsServerConfigureQosIndInfo->cisId;
                ase->qosInfoValid = TRUE;
            }
        }
        /* Send the Configure QOS Indication to the profile/app */
        MessageSend(ascs_server->appTask,
                    GATT_ASCS_SERVER_CONFIGURE_QOS_IND,
                    message);
    }
}

/***************************************************************************
NAME
    ascsSendEnableInd

DESCRIPTION
    Sends the Enable indication to the profile/app.
*/
static void ascsSendEnableInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsBuffIteratorReadOnly iter;
    uint8 access_ind_numAses;
    GattAscsConnection* connection = ascsFindConnection(ascs_server, access_ind->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    ascsBuffIteratorInitialiseReadOnly(&iter, &access_ind->value[0], access_ind->size_value);
    ascsBuffIteratorSkipOctetsReadOnly(&iter, 1 /* size of opcode */); /* skip over the configure_qos op code */

    access_ind_numAses = ascsBuffIteratorGet8(&iter);

    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerEnableInd,
        (access_ind_numAses * sizeof(GattAscsServerEnableIndInfo)));

    message->cid = access_ind->cid;
    message->numAses = 0;

    for (int i = 0;
         (i < access_ind_numAses) && (! ascsBuffIteratorErrorDetected(&iter));
         i++)
    {
        message->gattAscsServerEnableIndInfo[message->numAses].aseId = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerEnableIndInfo[message->numAses].metadataLength = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerEnableIndInfo[message->numAses].metadata =
            ascsBuffIteratorGetMultipleOctets(&iter, message->gattAscsServerEnableIndInfo[message->numAses].metadataLength);

        if (ascsConnectionValidateEnableIndInfo(connection,
                                                  &message->gattAscsServerEnableIndInfo[message->numAses]))
        {
            /* Keep the ASE data in the message (increment the numAses and start decoding the data for the next ASE in the access_ind) */
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

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if ((ascsBuffIteratorErrorDetected(&iter)) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */

    {
        /* NOTE: message->numAses might not be 0 */
        for (int i = 0; i < message->numAses; i++)
        {
            free(message->gattAscsServerEnableIndInfo[i].metadata);
        }
        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic are set in response_code and reason have been set above */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);

        /* There is nothing useful to send to the profile/app, so just free the message */
        free(message);
    }
    else
    {
        /* Only update the ASE's internal data after the complete ASCS operation has been validated (i.e. when we reach this branch) */
        for (int i = 0; i < message->numAses; i++)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->gattAscsServerEnableIndInfo[i].aseId);
            if (ase)
            {
                ase->metadataLength = message->gattAscsServerEnableIndInfo[i].metadataLength;
                /* the server retains ownership of this memory; the API specifies that the profile/app will not free it*/
                ase->metadata = message->gattAscsServerEnableIndInfo[i].metadata;
                /*
                 * Set the cisId and cigId in the Enable Ind
                 */
                message->gattAscsServerEnableIndInfo[i].cisId = ase->cachedConfigureQosInfoFromServer.cisId;
                message->gattAscsServerEnableIndInfo[i].cigId = ase->cachedConfigureQosInfoFromServer.cigId;
            }
        }
        MessageSend(
                ascs_server->appTask,
                GATT_ASCS_SERVER_ENABLE_IND,
                message
                );
    }
}

/***************************************************************************
NAME
    ascsConstructGenericInd

DESCRIPTION
    Constructs a generic indication message for indications that have a common format, e.g. Disable Ind, Release Ind.
*/
static GattAscsServerGenericInd* ascsConstructGenericInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsBuffIteratorReadOnly iter;
    uint8 access_ind_numAses;
    uint8 access_ind_op_code;
    GattAscsConnection* connection = ascsFindConnection(ascs_server, access_ind->cid);

    if (connection == NULL)
    {
        return NULL;
    }

    ascsBuffIteratorInitialiseReadOnly(&iter, &access_ind->value[0], access_ind->size_value);
    access_ind_op_code = ascsBuffIteratorGet8(&iter);

    access_ind_numAses = ascsBuffIteratorGet8(&iter);

    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerGenericInd,
        (access_ind_numAses * sizeof(uint8)));

    message->cid = access_ind->cid;
    message->numAses = 0;

    for (int i = 0;
         (i < access_ind_numAses) && (! ascsBuffIteratorErrorDetected(&iter));
         i++)
    {
        GattAscsServerAse* ase;
        message->aseId[message->numAses] = ascsBuffIteratorGet8(&iter);
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
            if ((ase) && ascsConnectionValidateStateTransition(connection, ase, access_ind_op_code))
            {
                /* Keep the ASE data in the message (increment the numAses and start decoding the data for the next ASE in the access_ind) */
                message->numAses++;
            }
        }
    }

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if(ascsBuffIteratorErrorDetected(&iter) ||
       (message->numAses == 0))
    {
        free(message);
        message = NULL;
    }

    return message;
}

/***************************************************************************
NAME
    ascsSendReceiverReadyInd

DESCRIPTION
    Sends the Receiver Ready indication to the profile/app.
*/
static void ascsSendReceiverReadyInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsServerReceiverReadyInd* message = (GattAscsServerReceiverReadyInd*)ascsConstructGenericInd(ascs_server, access_ind);

    if (message)
    {
        MessageSend(
                ascs_server->appTask,
                GATT_ASCS_SERVER_RECEIVER_READY_IND,
                message
                );
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'response_code' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);
    }
}

/***************************************************************************
NAME
    ascsSendUpdateMetadataInd

DESCRIPTION
    Sends the Update Metadata indication to the profile/app.
*/
static void ascsSendUpdateMetadataInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsBuffIteratorReadOnly iter;
    uint8 access_ind_numAses;

    GattAscsConnection* connection = ascsFindConnection(ascs_server, access_ind->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id*/
    }

    ascsBuffIteratorInitialiseReadOnly(&iter, &access_ind->value[0], access_ind->size_value);
    ascsBuffIteratorSkipOctetsReadOnly(&iter, 1 /* size of opcode */); /* skip over the configure_qos op code */

    access_ind_numAses = ascsBuffIteratorGet8(&iter);

    MAKE_ASCS_SERVER_FLEX_MESSAGE(GattAscsServerUpdateMetadataInd,
        (access_ind_numAses * sizeof(GattAscsServerUpdateMetadataIndInfo)));

    message->cid = access_ind->cid;
    message->numAses = 0;

    for (int i = 0;
         (i < access_ind_numAses) && (! ascsBuffIteratorErrorDetected(&iter));
         i++)
    {
        message->gattAscsServerUpdateMetadataIndInfo[message->numAses].aseId = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerUpdateMetadataIndInfo[message->numAses].metadataLength = ascsBuffIteratorGet8(&iter);
        message->gattAscsServerUpdateMetadataIndInfo[message->numAses].metadata =
            ascsBuffIteratorGetMultipleOctets(&iter, message->gattAscsServerUpdateMetadataIndInfo[message->numAses].metadataLength);

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

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        /* A invalid length response code 'overrides' any previously set response codes */
        ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(&connection->aseControlPointNotify);
    }

    if ((ascsBuffIteratorErrorDetected(&iter)) ||  /* This op is an invalid length */
        (message->numAses == 0)) /* This indication doesn't have any valid ASEs */
    {
        /* NOTE: message->numAses might not be 0 */
        for (int i = 0; i < message->numAses; i++)
        {
            free(message->gattAscsServerUpdateMetadataIndInfo[i].metadata);
        }
        /* The Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic are set in response_code and reason have been set above */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);

        /* There is nothing useful to send to the profile/app, so just free the message */
        free(message);
    }
    else
    {
        /* only update the ASEs internal data after the ASCS operation has been validated */
        for (int i = 0; i < message->numAses; i++)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->gattAscsServerUpdateMetadataIndInfo[i].aseId);
            if (ase)
            {
                ase->metadataLength = message->gattAscsServerUpdateMetadataIndInfo[i].metadataLength;
                /* The ase will already have metadata from the Enable Ind operation, so free it before storing the new metadata */
                free(ase->metadata);
                /* the server retains ownership of this memory; the API specifies that the profile/app will not free it*/
                ase->metadata = message->gattAscsServerUpdateMetadataIndInfo[i].metadata;
            }
        }
        MessageSend(
                ascs_server->appTask,
                GATT_ASCS_SERVER_UPDATE_METADATA_IND,
                message
                );
    }
}

/***************************************************************************
NAME
    ascsSendDisableInd

DESCRIPTION
    Sends the Disable indication to the profile/app.
*/
static void ascsSendDisableInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsServerDisableInd* message = (GattAscsServerDisableInd*)ascsConstructGenericInd(ascs_server, access_ind);

    if (message)
    {
        MessageSend(
                ascs_server->appTask,
                GATT_ASCS_SERVER_DISABLE_IND,
                message
                );
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'response_code' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);
    }
}

/***************************************************************************
NAME
    ascsSendReceiverStopReadyInd

DESCRIPTION
    Sends the Receiver Stop Ready indication to the profile/app.
*/
static void ascsSendReceiverStopReadyInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsServerReceiverStopReadyInd* message = (GattAscsServerReceiverStopReadyInd*)ascsConstructGenericInd(ascs_server, access_ind);

    if (message)
    {
        MessageSend(
                ascs_server->appTask,
                GATT_ASCS_SERVER_RECEIVER_STOP_READY_IND,
                message
                );
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'response_code' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);
    }
}

/***************************************************************************
NAME
    ascsSendReleaseInd

DESCRIPTION
    Sends the Release indication to the profile/app.
*/
static GattAscsServerReleaseInd* ascsSendReleaseInd(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsServerReleaseInd* message = (GattAscsServerReleaseInd*)ascsConstructGenericInd(ascs_server, access_ind);

    if (message)
    {
        MessageSend(
                ascs_server->appTask,
                GATT_ASCS_SERVER_RELEASE_IND,
                message
                );
    }
    else
    {
        /* Something went wrong and the Ind could not be constructed, we will therefore not get a response from the profile/app
         * and will therefore not be sending an ASE CP Notify in that response call. */
        /* The ASE CP Characteristic 'response_code' and 'result' are set in ascsConstructGenericInd */
        ascsAseControlPointNotify(ascs_server, access_ind->cid);
    }

    return message;
}

/***************************************************************************
NAME
    ascsAseCharacteristicAccess

DESCRIPTION
    Deals with access of the HANDLE_ASCS_ASE_CHAR_x handles.
*/
static void ascsAseCharacteristicAccess(GattAscsServer* ascs_server,
                                        const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind,
                                        GattAscsServerAse* ase)
{
    /* we only permit reads */
    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        GATT_ASCS_SERVER_DEBUG_INFO(("ASCS:GATTMAN:SERVER:ACCESS_IND: Write not permitted Handle:0x%x\n",
                access_ind->handle));
        GattManagerServerAccessResponse(&ascs_server->libTask, access_ind->cid,
                access_ind->handle, gatt_status_write_not_permitted, 0, NULL);
        return;
    }

    /* handle read access only */
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        uint8 characteristic_size;
        uint8* characteristic_value;

        characteristic_value = ascsAseConstructCharacteristicValue(ase, &characteristic_size);

        if (characteristic_value)
        {
            GattManagerServerAccessResponse(&ascs_server->libTask,
                                            access_ind->cid,
                                            access_ind->handle,
                                            gatt_status_success,
                                            characteristic_size,
                                            characteristic_value);
            free(characteristic_value);
        }
        else
        {
            GattManagerServerAccessResponse(&ascs_server->libTask,
                                            access_ind->cid,
                                            access_ind->handle,
                                            gatt_status_unlikely_error,
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
        Task task,
        uint16 cid,
        uint16 handle,
        uint16 client_config
        )
{
    uint8 config_data[GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE];

    if (task == NULL || cid == 0)
    {
        GATT_ASCS_SERVER_DEBUG_PANIC((
                    "ASCS: Null instance!\n"
                    ));
    }

    /* BASED on the spec this can only have one of the following three values:.
           GATT_ASCS_CLIENT_CONFIG_NOT_SET  0, 
           GATT_ASCS_CLIENT_CONFIG_NOTIFY   1, 
           GATT_ASCS_CLIENT_CONFIG_INDICATE 2 
           NOTE:the only reason we use index 1 is for compliance in case of 
           spec change.
    */
    config_data[0] = (uint8)(client_config & 0xFF);
    config_data[1] = (uint8)(client_config >> 8);

    sendAscsServerAccessRsp(
            task,
            cid,
            handle,
            gatt_status_success,
            GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE,
            config_data
            );
}

/***************************************************************************
NAME
    ascsHandleWriteClientConfigAccess

DESCRIPTION
    Writes the client config.
*/
static bool ascsHandleWriteClientConfigAccess(
        Task task,
        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
        uint16 *client_config
        )
{

    if (task == NULL || access_ind->cid == 0)
    {
        GATT_ASCS_SERVER_DEBUG_PANIC((
                    "ASCS: Null instance!\n"
                    ));
    }


    if (access_ind->size_value != GATT_ASCS_CLIENT_CONFIG_VALUE_SIZE)
    {
        sendAscsServerAccessRsp(task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_invalid_length,
                                0,
                                NULL);
        
        return FALSE;
    }

    switch (access_ind->value[0])
    {
    case GATT_ASCS_CLIENT_CONFIG_NOTIFY:
    case GATT_ASCS_CLIENT_CONFIG_NOT_SET:
    {
        /*GATT spec only uses 3 values for the client config: 0,1,2*/
        (*client_config) = (uint16) access_ind->value[0];
    }
    break;
    default:
    {
        /* ONLY Notify is allowed (INDICATE falls here)*/
        sendAscsServerAccessRsp(task,
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_cccd_improper_config,
                                0,
                                NULL);
    }
    return FALSE;
    }

    /* If not GATT_ASCS_CLIENT_CONFIG_NOTIFY send a generic respons without 
        changing the client config */
    sendAscsServerAccessRsp(task,
                            access_ind->cid,
                            access_ind->handle,
                            gatt_status_success,
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
static void ascsHandleClientConfigAccess(GattAscsServer *ascs_server,
                                        const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind,
                                        uint16* client_cfg)
{
    if (access_ind->flags & ATT_ACCESS_READ)
    {
        ascsHandleReadClientConfigAccess(
                    (Task) &ascs_server->libTask,
                    access_ind->cid,
                    access_ind->handle,
                    *client_cfg
                    );
    }
    else if (access_ind->flags & ATT_ACCESS_WRITE)
    {
            ascsHandleWriteClientConfigAccess(
                        (Task) &ascs_server->libTask,
                        access_ind,
                        client_cfg
                        );
    }
    else
    {
        sendAscsServerAccessRsp((Task) &(ascs_server->libTask),
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_success,
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
static void ascsAseControlPointAccess(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    if (access_ind->flags & ATT_ACCESS_WRITE)
    {
        GattAscsConnection* connection = ascsFindConnection(ascs_server, access_ind->cid);

        if (connection == NULL)
        {
            GATT_ASCS_SERVER_DEBUG_INFO(("There is no CONNECTION_T instance associated with the access_ind->cid value\n"));
            return; /* This should never happen */
        }

        uint8 opcode = access_ind->value[0];   /* All ASE Control Point Access Reads start with: opcode (followed by numAses) */

        sendAscsServerAccessRsp((Task) &(ascs_server->libTask),
                                access_ind->cid,
                                access_ind->handle,
                                gatt_status_success,
                                0,
                                NULL);

        ascsAseControlPointCharacteristicReset(&connection->aseControlPointNotify, opcode);


        switch(opcode)
        {
            case GATT_ASCS_SERVER_CONFIG_CODEC_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Configure Codec operation\n"));
                ascsSendConfigureCodecInd(ascs_server, access_ind);
            }
            break;

            case GATT_ASCS_SERVER_CONFIG_QOS_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Configure Qos operation\n"));
                ascsSendConfigureQosInd(ascs_server, access_ind);
            }
            break;

            case GATT_ASCS_SERVER_ENABLE_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Enable operation\n"));
                ascsSendEnableInd(ascs_server, access_ind);
            }
            break;

            case GATT_ASCS_SERVER_RECEIVER_START_READY_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Receiver Start Ready operation\n"));
                ascsSendReceiverReadyInd(ascs_server, access_ind);
            }
            break;

            case GATT_ASCS_SERVER_UPDATE_METADATA_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Update Metadata operation\n"));
                ascsSendUpdateMetadataInd(ascs_server, access_ind);
            }
            break;

            case GATT_ASCS_SERVER_DISABLE_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Disable operation\n"));
                ascsSendDisableInd(ascs_server, access_ind);
            }
            break;

            case GATT_ASCS_SERVER_RECEIVER_STOP_READY_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Receiver Stop Ready operation\n"));
                ascsSendReceiverStopReadyInd(ascs_server, access_ind);
            }
            break;

            case GATT_ASCS_SERVER_RELEASE_OPCODE:
            {
                GATT_ASCS_SERVER_DEBUG_INFO(("Release operation\n"));

                {
                    GattAscsServerReleaseInd* message = ascsSendReleaseInd(ascs_server, access_ind);

                    if (message)
                    {
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
                         * NOTE: The profile/app calls GattAscsServerReleaseComplete() after receiving the
                         *       RELEASE_IND and after the underlying CIS has been cleared, this call transitions
                         *       the ASE(s) from the GATT_ASCS_SERVER_ASE_STATE_RELEASING state to the
                         *       GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED state.
                         */
                        ascsAseControlPointNotify(ascs_server, access_ind->cid);

                        for (int i = 0; i < message->numAses; i++)
                        {
                            GattAscsServerAse* ase = ascsConnectionFindAse(connection, message->aseId[i]);
                            if (ase)
                            {
                                ascsServerSetAseStateAndNotify(ascs_server, message->cid, ase, GATT_ASCS_SERVER_ASE_STATE_RELEASING);
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
                ascsAseControlPointNotify(ascs_server, access_ind->cid);
            }
            break;
        }

        /* NOTE: The ASE Control Point Notification is sent when the Response is received from the profile/app */
    }
    else
    {
        sendAscsServerAccessRsp(
                        (Task) &ascs_server->libTask,
                        access_ind->cid,
                        access_ind->handle,
                        gatt_status_request_not_supported,
                        0,
                        NULL);
    }
}

/***************************************************************************/

void ascsAseControlPointNotify(const GattAscsServer* ascs_server, ConnectionId cid)
{
    uint8 characteristic_length;
    uint8* characteristic_value;
    GattAscsConnection* connection = ascsFindConnection(ascs_server, cid);

    if (connection)
    {
        characteristic_value = ascsAseControlPointConstructCharacteristicValue(&connection->aseControlPointNotify,
                                                                               &characteristic_length);

        if((connection->aseControlPointNotify.clientCfg != GATT_ASCS_CLIENT_CONFIG_NOTIFY) ||
            (characteristic_value == NULL))
        {
            free(characteristic_value);
            /* Notify is not enabled by the client. Server does not need to send
                any notifications to the client */
            return;
        }

        GattManagerRemoteClientNotify((Task)&ascs_server->libTask,
                                      cid,
                                      HANDLE_ASCS_ASE_CONTROL_POINT_CHAR,
                                      characteristic_length,
                                      characteristic_value);
    }
}

void handleAscsAccess(GattAscsServer* ascs_server, const GATT_MANAGER_SERVER_ACCESS_IND_T* access_ind)
{
    GattAscsConnection* connection = ascsFindConnection(ascs_server, access_ind->cid);

    if (connection == NULL)
    {
        return; /* We cannot process an access ind for a connection we do not recognise */
    }

    switch (access_ind->handle)
    {       
        case HANDLE_ASCS_ASE_CONTROL_POINT_CHAR:
        {
            ascsAseControlPointAccess(ascs_server, access_ind);
        }
        break;

        case HANDLE_ASCS_ASE_CONTROL_POINT_CHAR_CLIENT_CONFIG:
        {
            ascsHandleClientConfigAccess(ascs_server,
                                         access_ind,
                                         &connection->aseControlPointNotify.clientCfg);
        }
        break;
#ifdef HANDLE_ASCS_ASE_CHAR_1
        case HANDLE_ASCS_ASE_CHAR_1:
        {
            ascsAseCharacteristicAccess(ascs_server,
                                        access_ind,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_1_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_1:
        {
            ascsHandleClientConfigAccess(ascs_server,
                                         access_ind,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_1_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_2
        case HANDLE_ASCS_ASE_CHAR_2:
        {
            ascsAseCharacteristicAccess(ascs_server,
                                        access_ind,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_2_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_2:
        {
            ascsHandleClientConfigAccess(ascs_server,
                                         access_ind,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_2_IDX].clientCfg);
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_3
        case HANDLE_ASCS_ASE_CHAR_3:
        {
            ascsAseCharacteristicAccess(ascs_server,
                                        access_ind,
                                        &connection->ase[GATT_ASCS_ASE_CHAR_3_IDX]);
        }
        break;

        case HANDLE_ASE_CHAR_CLIENT_CONFIG_3:
        {
            ascsHandleClientConfigAccess(ascs_server,
                                         access_ind,
                                         &connection->ase[GATT_ASCS_ASE_CHAR_3_IDX].clientCfg);
        }
        break;
#endif
        default:
        {
            /* Respond to invalid handles */
           sendAscsServerAccessRsp(
                    (Task) &ascs_server->libTask,
                    access_ind->cid,
                    access_ind->handle,
                    gatt_status_invalid_handle,
                    0,
                    NULL);
        }
        break;
    }
}
