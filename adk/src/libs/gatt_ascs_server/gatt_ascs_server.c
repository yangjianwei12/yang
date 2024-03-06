/* Copyright (c) 2021 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <logging.h>
#include "gatt_ascs_server_db.h"
#include "service_handle.h"
#include "gatt_ascs_server_private.h"
#include "gatt_ascs_server_msg_handler.h"

#define ASCS_SERVER_DEBUG
#define ASCS_SERVER_LOG     DEBUG_LOG

#if defined(ASCS_SERVER_DEBUG)
/* Static variable storing the internal of the ASCS Server to allow debugging 
 * in pydbg. The strcuture can be accessed as follow:
 * apps1.fw.env.cus["gatt_ascs_server.c"].localvars["p_gascs"].deref
 * apps1.fw.env.struct("p_gascs")
 **/
static GattAscsServer* p_gascs;
#define GATT_ASCS_SERVER_SET_GASCS_DEBUG(pgascs) (p_gascs = (pgascs))
#else 
#define GATT_ASCS_SERVER_SET_GASCS_DEBUG(pgascs) ((void)0)
#endif /*ASCS_SERVER_DEBUG*/

static uint8* ascsAseConstructQosConfiguredCharacteristicValue(GattAscsServerAse* ase, uint8* characteristic_length);

void ascsAseControlPointCharacteristicReset(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 op_code)
{
    /*
     * Note: Don't memset the entire struct as it will overwrite the client_cfg
     */
    ase_control_point_notify->opCode = op_code;
    ase_control_point_notify->numAses = 0;

    for (int i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
    {
        ase_control_point_notify->aseResponses[i].reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
        ase_control_point_notify->aseResponses[i].responseCode = GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS;
    }
}

void ascsAseControlPointCharacteristicAddAseResponse(GattAscsAseControlPointNotify* ase_control_point_notify, AscsAseResponse* ase_response)
{
    if (ase_control_point_notify->numAses == GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES)
    {
        return; /* The entire op has been aborted already, in ASCS 'validation r03' this can be because the server received an invalid length op or an unrecognised op code */
    }

    /* Find if we already have this ASE response in the ASE CP Characteristic (and it just needs updating) */
    for (int i = 0; i < ase_control_point_notify->numAses; i++)
    {
        if (ase_control_point_notify->aseResponses[i].aseId == ase_response->aseId)

        {
            /* Allow 'success'to be overwritten with an error response code
             * but don't overwrite an existing error response.
             */
            if (ase_control_point_notify->aseResponses[i].responseCode == GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS)
            {
                ase_control_point_notify->aseResponses[i] = *ase_response;
            }
            return;
        }
    }

    /* We don't already have this ASE response, we need to add it */
    if (ase_control_point_notify->numAses < GATT_ASCS_NUM_ASES_MAX)
    {
        ase_control_point_notify->aseResponses[ase_control_point_notify->numAses] = *ase_response;
        ase_control_point_notify->numAses++;
    }
}

void ascsAseControlPointCharacteristicAddSuccessResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_SUCCESS;

    ascsAseControlPointCharacteristicAddAseResponse(ase_control_point_notify, &response);
}

void ascsAseControlPointCharacteristicAddInvalidAseIdResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_ID;

    ascsAseControlPointCharacteristicAddAseResponse(ase_control_point_notify, &response);
}

void ascsAseControlPointCharacteristicAddInvalidLengthResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify)
{
    ase_control_point_notify->numAses = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES; /* special value defined in ASCS 'validation r03' used for invalid length op or unrecognised op code */

    ase_control_point_notify->aseResponses[0].aseId = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_ASE_ID; /* ASCS 'validation r03' use aseId = 0 in response to an invalid length op or unrecognised op code */
    ase_control_point_notify->aseResponses[0].reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    ase_control_point_notify->aseResponses[0].responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_LENGTH_OPERATION;
}

void ascsAseControlPointCharacteristicAddUnsupportedOpcodeResponse(GattAscsAseControlPointNotify* ase_control_point_notify)
{
    ase_control_point_notify->numAses = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES; /* special value defined in ASCS 'validation r03' used for an invalid length op or unrecognised op code */

    ase_control_point_notify->aseResponses[0].aseId = GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_ASE_ID;  /* ASCS 'validation r03' use aseId = 0 in response to an invalid length op or unrecognised op code */
    ase_control_point_notify->aseResponses[0].reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    ase_control_point_notify->aseResponses[0].responseCode = GATT_ASCS_ASE_RESPONSE_CODE_UNSUPPORTED_OPCODE;
}

void ascsAseControlPointCharacteristicAddInvalidTransitionResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                                       uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_ASE_STATE_TRANSITION;

    ascsAseControlPointCharacteristicAddAseResponse(ase_control_point_notify, &response);

}

void ascsAseControlPointCharacteristicAddInvalidParameterValueResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                                           uint8 aseId,
                                                                           uint8 reason_code)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = reason_code;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_INVALID_CONFIGURATION_PARAMETER_VALUE;

    ascsAseControlPointCharacteristicAddAseResponse(ase_control_point_notify, &response);
}

void ascsAseControlPointCharacteristicAddRejectedParameterValueResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                                            uint8 aseId,
                                                                            uint8 reason_code)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = reason_code;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_REJECTED_CONFIGURATION_PARAMETER_VALUE;

    ascsAseControlPointCharacteristicAddAseResponse(ase_control_point_notify, &response);
}

void ascsAseControlPointCharacteristicAddUnspecifiedErrorResponseCode(GattAscsAseControlPointNotify* ase_control_point_notify, uint8 aseId)
{
    AscsAseResponse response;

    response.aseId = aseId;
    response.reason = GATT_ASCS_ASE_REASON_UNSPECIFIED;
    response.responseCode = GATT_ASCS_ASE_RESPONSE_CODE_UNSPECIFIED_ERROR;

    ascsAseControlPointCharacteristicAddAseResponse(ase_control_point_notify, &response);
}


uint8* ascsAseControlPointConstructCharacteristicValue(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                       uint8* characteristic_length)
{
    GattAscsBuffIterator iter;
    uint8* characteristic_value;
    uint8 num_ase_responses;

    /* Check to see if the GATT_ASCS_ASE_CONTROL_POINT_NOTIFY_T is capable of constructing a Notify */
    if (ase_control_point_notify->numAses == 0)
    {
        *characteristic_length = 0;
        return NULL;
    }

    *characteristic_length = sizeof(struct
                                   {
                                      /*
                                       * Fields in the ASE Control Point Characteristic : ASCS d09r06
                                       */
                                       uint8 op_code;
                                       uint8 numAses;
                                  /*     ASCS_ASE_RESPONSE_T ase_responses[];        * variable length */
                                   });

    /*
     * The ASCS 'Validation r03' spec uses the 'numAses' field AND the response code to convey essentially the same information: the op has been aborted
     * This means the numAses field does NOT represent the number of ASE responses in the ase_control_point_notify for an invalid length op or
     * invalid op code, instead, for these cases the number of ASE entries is actually 1, but the numAses field is set to 0xFF
     */
    num_ase_responses = (ase_control_point_notify->numAses == GATT_ASE_CONTROL_POINT_NOTIFY_OPERATION_ABORTED_NUM_ASES)? 1 : ase_control_point_notify->numAses;

    *characteristic_length += num_ase_responses * sizeof(AscsAseResponse);

    characteristic_value = PanicUnlessMalloc(*characteristic_length);

    ascsBuffIteratorInitialise(&iter, characteristic_value, *characteristic_length);

    /*
     * populate the characteristic_value
     */

    ascsBuffIteratorWrite8(&iter, ase_control_point_notify->opCode);
    ascsBuffIteratorWrite8(&iter, ase_control_point_notify->numAses);
    for (int i = 0; i < num_ase_responses; i++)
    {
        ascsBuffIteratorWrite8(&iter, ase_control_point_notify->aseResponses[i].aseId);
        ascsBuffIteratorWrite8(&iter, ase_control_point_notify->aseResponses[i].responseCode);
        ascsBuffIteratorWrite8(&iter, ase_control_point_notify->aseResponses[i].reason);
    }

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristic_value);
        characteristic_value = NULL;
        *characteristic_length = 0;
    }

    return characteristic_value;
}

static bool ascsAseClientNotify(const GattAscsServer *ascs_server, ConnectionId cid, GattAscsServerAse* ase)
{
    uint8 characteristic_length;
    uint8* characteristic_value;
    uint16 ase_characteristic = 0;

    if(ase->clientCfg != GATT_ASCS_CLIENT_CONFIG_NOTIFY)
    {
        /* Notify is not enabled by the client. Server does not need to send
            any notifications to the client */
        return TRUE;
    }

    characteristic_value = ascsAseConstructCharacteristicValue(ase, &characteristic_length);

    if (characteristic_value == NULL)
    {
        return FALSE;
    }

    /*TODO: As we support more ASEs this switch will grow*/
    switch(ase->aseId)
    {
        case 1:
        {
            ase_characteristic = (uint16) HANDLE_ASCS_ASE_CHAR_1;
        }
        break;
#ifdef HANDLE_ASCS_ASE_CHAR_2
        case 2:
        {
            ase_characteristic = (uint16) HANDLE_ASCS_ASE_CHAR_2;
        }
        break;
#endif
#ifdef HANDLE_ASCS_ASE_CHAR_3
        case 3:
        {
            ase_characteristic = (uint16) HANDLE_ASCS_ASE_CHAR_3;
        }
        break;
#endif
        default:
        {
            /*this should never happen*/
            GATT_ASCS_SERVER_DEBUG_PANIC(("ASE ID does not match the numer of"
                                            "characteristics supported\n"));
        }
        break;
    }
        /* Send notification to GATT Manager */
    GattManagerRemoteClientNotify((Task)&ascs_server->libTask,
                                    cid,
                                    ase_characteristic,
                                    characteristic_length,
                                    characteristic_value);


    free(characteristic_value);
    return TRUE;
}

static uint8* ascsAseConstructIdleAndReleasingCharacteristicValue(GattAscsServerAse* ase, uint8* characteristic_length)
{
    GattAscsBuffIterator iter;
    uint8* characteristic_value;

    *characteristic_length = sizeof(struct
                                   {
                                    /*
                                    * Fields in the ASE Characteristic : ASCS d09r06 Table 4.2 (idle and release have no
                                    * 'additional parameters')
                                    */
                                       uint8 aseId;
                                       uint8 ase_state;
                                   });

    characteristic_value = PanicUnlessMalloc(*characteristic_length);

    ascsBuffIteratorInitialise(&iter, characteristic_value, *characteristic_length);

    /*  populate the characteristic_value */
    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristic_value);
        characteristic_value = NULL;
        *characteristic_length = 0;
    }

    return characteristic_value;
}

static uint8* ascsAseConstructCodecConfiguredCharacteristicValue(GattAscsServerAse* ase, uint8* characteristic_length)
{
    GattAscsBuffIterator iter;
    uint8* characteristic_value;

    *characteristic_length = sizeof(struct
                                   {
                                            /*
                                             * Fields in the ASE Characteristic : ASCS d09r06
                                             */
                                       uint8 aseId;
                                       uint8 ase_state;
                                       uint8 direction;
                                       uint8 framing;
                                       uint8 preferred_phy;
                                       uint8 preferred_retransmissionNumber;
                                       uint8 maxTransportLatency[2];
                                       uint8 presentationDelayMin[3];
                                       uint8 presentationDelayMax[3];
                                       uint8 codecId[5];
                                       uint8 codec_specific_configuration_length;
                                       /* codec_specific_configuration    variable length */
                                   });

    *characteristic_length += ase->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength;

    characteristic_value = PanicUnlessMalloc(*characteristic_length);

    ascsBuffIteratorInitialise(&iter, characteristic_value, *characteristic_length);

    /*  populate the characteristic_value */

    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);
    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureCodecInfo.direction);
    ascsBuffIteratorWrite8(&iter,  ase->cachedConfigureCodecInfo.infoFromServer.framing);
    ascsBuffIteratorWrite8(&iter,  ase->cachedConfigureCodecInfo.infoFromServer.phyPreference);
    ascsBuffIteratorWrite8(&iter,  ase->cachedConfigureCodecInfo.infoFromServer.retransmissionNumber);
    ascsBuffIteratorWrite16(&iter, ase->cachedConfigureCodecInfo.infoFromServer.transportLatencyMax);
    ascsBuffIteratorWrite24(&iter, ase->cachedConfigureCodecInfo.infoFromServer.presentationDelayMin);
    ascsBuffIteratorWrite24(&iter, ase->cachedConfigureCodecInfo.infoFromServer.presentationDelayMax);

    ascsBuffIteratorWrite8(&iter,  ase->cachedConfigureCodecInfo.codecId.codingFormat);
    ascsBuffIteratorWrite16(&iter, ase->cachedConfigureCodecInfo.codecId.companyId);
    ascsBuffIteratorWrite16(&iter, ase->cachedConfigureCodecInfo.codecId.vendorSpecificCodecId);

    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength);
    ascsBuffIteratorWriteMultipleOctets(&iter,
                                        ase->cachedConfigureCodecInfo.infoFromServer.codecConfiguration,
                                        ase->cachedConfigureCodecInfo.infoFromServer.codecConfigurationLength);

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristic_value);
        characteristic_value = NULL;
        *characteristic_length = 0;
    }

    return characteristic_value;
}

static uint8* ascsAseConstructGenericCharacteristicValue(GattAscsServerAse* ase, uint8* characteristic_length)
{
    GattAscsBuffIterator iter;
    uint8* characteristic_value;

    *characteristic_length = sizeof(struct
                                   {
                                      /*
                                       * Fields in the ASE Characteristic : ASCS d09r06
                                       */
                                       uint8 aseId;
                                       uint8 ase_state;
                                       uint8 cigId;
                                       uint8 cisId;
                                       uint8 metadataLength;
                                    /*  uint8 metadata[];  variable length */
                                   });

    *characteristic_length += ase->metadataLength;

    characteristic_value = PanicUnlessMalloc(*characteristic_length);

    ascsBuffIteratorInitialise(&iter, characteristic_value, *characteristic_length);

    /*
     * populate the characteristic_value
     */

    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);
    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureQosInfoFromServer.cigId);
    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureQosInfoFromServer.cisId);
    ascsBuffIteratorWrite8(&iter, ase->metadataLength);

    ascsBuffIteratorWriteMultipleOctets(&iter, ase->metadata, ase->metadataLength);

    if (ascsBuffIteratorErrorDetected(&iter))
    {
        free(characteristic_value);
        characteristic_value = NULL;
        *characteristic_length = 0;
    }

    return characteristic_value;
}

uint8* ascsAseConstructCharacteristicValue(GattAscsServerAse* ase, uint8* characteristic_length)
{
    uint8* characteristic_value = NULL;

    switch(ase->state)
    {
    case GATT_ASCS_SERVER_ASE_STATE_IDLE:
        /* Fall through to construct characteristic with no 'Additional ASE parameters' */
    case GATT_ASCS_SERVER_ASE_STATE_RELEASING:
        characteristic_value = ascsAseConstructIdleAndReleasingCharacteristicValue(ase, characteristic_length);
        break;
    case GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED:
        characteristic_value = ascsAseConstructCodecConfiguredCharacteristicValue(ase, characteristic_length);
        break;
    case GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED:
        characteristic_value = ascsAseConstructQosConfiguredCharacteristicValue(ase, characteristic_length);
        break;
    case GATT_ASCS_SERVER_ASE_STATE_ENABLING:
        /* Fall through to construct generic characteristic */
    case GATT_ASCS_SERVER_ASE_STATE_STREAMING:
        /* Fall through to construct generic characteristic */
    case GATT_ASCS_SERVER_ASE_STATE_DISABLING:
        characteristic_value = ascsAseConstructGenericCharacteristicValue(ase, characteristic_length);
        break;

    };

    return characteristic_value;
}

static void ascsBuffIteratorInitialiseCommon(GattAscsBuffIteratorCommon* common, uint16 size)
{
    common->error = FALSE;
    common->size = size;
}

void ascsBuffIteratorInitialise(GattAscsBuffIterator* iter, uint8* buffer, uint16 size)
{
    iter->dataStart = buffer;
    iter->data = buffer;
    ascsBuffIteratorInitialiseCommon(&iter->common, size);
}

void ascsBuffIteratorInitialiseReadOnly(GattAscsBuffIteratorReadOnly* iter, const uint8* buffer, uint16 size)
{
    iter->dataStart = buffer;
    iter->data = buffer;
    ascsBuffIteratorInitialiseCommon(&iter->common, size);
}


uint8 ascsBuffIteratorGet8(GattAscsBuffIteratorReadOnly* iter)
{
    if ((iter->data - iter->dataStart) < iter->common.size)
        return *iter->data++;
    else
        iter->common.error = TRUE;

    return 0;
}

bool ascsBuffIteratorWrite8(GattAscsBuffIterator* iter, uint8 value)
{
    if ((iter->data - iter->dataStart) < iter->common.size)
        *iter->data++ = value;
    else
        iter->common.error = TRUE;

    return ( ! iter->common.error);
}

uint16 ascsBuffIteratorGet16(GattAscsBuffIteratorReadOnly* iter)
{
    uint16 value;
    value  = ascsBuffIteratorGet8(iter);
    value += ascsBuffIteratorGet8(iter) << 0x08;
    return value;
}

bool ascsBuffIteratorWrite16(GattAscsBuffIterator* iter, uint16 value)
{
    bool result;

    result = ascsBuffIteratorWrite8(iter, value & 0x00FF);
    if (result)
        result = ascsBuffIteratorWrite8(iter, (value >> 0x08) & 0x00FF);

    return result;
}

uint32 ascsBuffIteratorGet24(GattAscsBuffIteratorReadOnly* iter)
{
    uint32 value;
    value  = ascsBuffIteratorGet8(iter);
    value += ascsBuffIteratorGet8(iter) << 0x08;
    value += ascsBuffIteratorGet8(iter) << 0x10;
    return value;
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

uint8* ascsBuffIteratorGetMultipleOctets(GattAscsBuffIteratorReadOnly* iter, uint8 num_octets)
{
    uint8* dest = NULL;
    if (! ascsBuffIteratorErrorDetected(iter)) /* don't call functions like ascsBuffIteratorSkipOctetsReadOnly() if there's already an overflow */
    {
        const uint8* src = iter->data;

        ascsBuffIteratorSkipOctetsReadOnly(iter, num_octets);
        if (! ascsBuffIteratorErrorDetected(iter))
        {
            dest = PanicUnlessMalloc(num_octets);

            memcpy(dest, src, num_octets);
        }
    }
    return dest;
}

bool ascsBuffIteratorWriteMultipleOctets(GattAscsBuffIterator* iter, uint8* src, uint8 num_octets)
{
    uint8* dest = iter->data;

    ascsBuffIteratorSkipOctets(iter, num_octets);
    if (! ascsBuffIteratorErrorDetected(iter))
    {
        memcpy(dest, src, num_octets);
    }
    return ( ! iter->common.error );
}

void ascsBuffIteratorSkipOctets(GattAscsBuffIterator* iter, uint8 num_octets)
{
    iter->data += num_octets;
    if ((iter->data - iter->dataStart) <= iter->common.size)
    {
        /* no error: iter->data still points within the bounds of the buffer */
    }
    else
        iter->common.error = TRUE;
}

void ascsBuffIteratorSkipOctetsReadOnly(GattAscsBuffIteratorReadOnly* iter, uint8 num_octets)
{
    iter->data += num_octets;
    if ((iter->data - iter->dataStart) <= iter->common.size)
    {
        /* no error: iter->data still points within the bounds of the buffer */
    }
    else
        iter->common.error = TRUE;
}

static void ascsServerAseInitialise(GattAscsServerAse* ase, uint8 aseId)
{
    memset(ase, 0, sizeof(GattAscsServerAse));

    /* Fields that need to be initialised to none zero values should be initialised here */

    ase->aseId = aseId;

    /* commented (to save code space) because these initial values are zero
    ase->state = GATT_ASCS_SERVER_ASE_STATE_IDLE;
    ase->direction = GATT_ASCS_ASE_DIRECTION_SERVER_UNINITIALISED;
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

static void ascsInitialiseConnection(GattAscsConnection* connection, ConnectionId cid)
{
    connection->cid = cid;
    /*TODO: Connection->numAses is currently NOT used.*/
    connection->numAses = GATT_ASCS_NUM_ASES_MAX;
    for (int i = 0; i < GATT_ASCS_NUM_ASES_MAX; i++)
    {
        /* NOTE: Don't use aseId = 0. In ASCS 'Validation R03' an aseId of 0 is reserved for an invalid length operation or unrecognised an op code.
         *       Use aseId values: 1, 2, 3, ... */
        ascsServerAseInitialise(&connection->ase[i], i + 1);
    }
}

GattAscsConnection* ascsFindConnection(const GattAscsServer *ascs_server, ConnectionId cid)
{
    for (int i = 0; i < ascs_server->numConnections; i++)
    {
        if (ascs_server->connection[i]->cid == cid)
            return ascs_server->connection[i];
    }
    return NULL;
}

static GattAscsConnection* ascsAddConnection(GattAscsServer *ascs_server, ConnectionId cid)
{
    GattAscsConnection* connection;

    if(cid == 0)
        return NULL;

    connection = ascsFindConnection(ascs_server, cid);

    if (connection == NULL)
    {
        if (ascs_server->numConnections < GATT_ASCS_NUM_CONNECTIONS_MAX)
        {
            connection = (GattAscsConnection *)PanicNull( calloc(1, sizeof(GattAscsConnection)));

            ascsInitialiseConnection(connection, cid);

            ascs_server->connection[ascs_server->numConnections++] = connection;
        }
    }

    return connection;
}

static void ascsRemoveConnection(GattAscsServer *ascs_server, GattAscsConnection* connection)
{
    bool move_connection_to_lower_index = FALSE;
    for (int i = 0; i < ascs_server->numConnections; i++)
    {
        if (move_connection_to_lower_index)
            ascs_server->connection[i-1] = ascs_server->connection[i];

        if (ascs_server->connection[i] == connection)
        {
            /* ascsConnectionDestruct(connection); if the connection ever contains dynamically allocated memory we'll need a destruct function */
            free(connection);

            move_connection_to_lower_index = TRUE;
        }
    }
    ascs_server->numConnections--;
}

bool ascsServerSetAseStateAndNotify(const GattAscsServer *ascs_server,
                                    ConnectionId cid,
                                    GattAscsServerAse* ase,
                                    GattAscsServerAseState new_state)
{
    ase->state = new_state;

    /* Check to see if we have transitioned to a state where the QOS info is no longer relevant, i.e.
     * transitioned to a state where we MUST receive NEW QOS configuration before attempting to stream again
     */
    switch (ase->state)
    {
    case GATT_ASCS_SERVER_ASE_STATE_IDLE:
    case GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED:
    case GATT_ASCS_SERVER_ASE_STATE_RELEASING:
        /* In these states the QOS info (e.g. CIS id / CIG id) are NO LONGER valid: from any of these
         * states, the ASE must first go through 'QOS Configure' to receive NEW QOS info
         * (e.g. new CIS id / CIG id values) before it can attempt streaming
         */
        ase->qosInfoValid = FALSE;
        break;
    default:
        /* In these states the QOS info (e.g. CIS id / CIG id) ARE configured in the ASE and (in
         * particular) the CIG id and CIS id must not clash with CIS id / CIG id values in any other ASEs */
        break;
    };

    if (!ascsAseClientNotify(ascs_server, cid, ase))
    {
        return FALSE;
    }

    return TRUE;
}

GattAscsServerAse* ascsServerFindAse(const GattAscsServer *ascs_server, ConnectionId cid, uint8 aseId)
{
    GattAscsConnection* connection = ascsFindConnection(ascs_server, cid);

    if (connection)
    {
        return ascsConnectionFindAse(connection, aseId);
    }
    return NULL;
}

GattAscsServerAse* ascsConnectionFindAse(GattAscsConnection *connection, uint8 aseId)
{
    for (int i = 0; i < connection->numAses; i++)
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
GattAscsClientConfig* GattAscsRemoveConfig(
                        ServiceHandle        service_handle,
                        ConnectionId         cid
                        )
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    GattAscsClientConfig* config = NULL;
    GattAscsConnection* connection;

    if (ascs == NULL)
    {
        return NULL;
    }

    connection = ascsFindConnection(ascs, cid);

    if (connection)
    {
        config =(GattAscsClientConfig *)PanicNull(
                                                calloc(1, sizeof(GattAscsClientConfig) +
                                                        connection->numAses * sizeof(ClientConfig)));

        config->aseControlPointCharClientCfg = connection->aseControlPointNotify.clientCfg;
        config->numAses = connection->numAses;
        for(int i=0; i < connection->numAses; i++)
        {
            config->aseCharClientCfg[i] = connection->ase[i].clientCfg;
        }

        ascsRemoveConnection(ascs, connection);
    }

    return config;
}

gatt_status_t GattAscsAddConfig(
            ServiceHandle                    service_handle,
            ConnectionId                     cid,
            const GattAscsClientConfig*      config
            )
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    GattAscsConnection* connection;

    if(ascs == NULL)
    {
        return gatt_status_invalid_handle;
    }

    /*
     * A NULL config indicates that the lib must create a NEW connection instance
     * with the specified cid: make sure that a connection with this cid doesn't already exist
     */
    if ((config == NULL) && (ascsFindConnection(ascs, cid)))
    {
        /* with (config == NULL) the connection instance should not already exist */
        return gatt_status_value_not_allowed;
    }

    /* ascsAddConnection() either:
     *      * adds a connection and sets the cid accordingly or
     *      * returns an existing connection that has the specified cid
     */
    connection = ascsAddConnection(ascs, cid);

    if (config == NULL)
    {
        /* There is nothing left to do:
         *      * a connection didn't already exist (we'd have returned sooner if one had existed and the config == NULL)
         *      * we successfully created one (we'd have returned sooner if we hadn't created one)
         *      * and there is no configuration to store
         */
        return gatt_status_success;
    }

    /*
     * A non-NULL config indicates that the lib must configure the connection with the relevant GATT_ASCS_CLIENT_CONFIG_T.
     * The connection instance may (or may not) already exist.
     */

    if (config->numAses != connection->numAses)
    {
        ascsRemoveConnection(ascs, connection);
        return gatt_result_invalid_params;
    }

    if (config->aseControlPointCharClientCfg != GATT_ASCS_CLIENT_CONFIG_NOTIFY &&
        config->aseControlPointCharClientCfg != GATT_ASCS_CLIENT_CONFIG_NOT_SET)
    {
        GATT_ASCS_SERVER_DEBUG_INFO(("Invalid Client Configuration ASE CP Characteristic!\n"));
        ascsRemoveConnection(ascs, connection);
        return gatt_status_value_not_allowed;/*Only Notify and NO SET*/
    }

    for(int i = 0; i < config->numAses; i++)
    {
        if(config->aseCharClientCfg[i] != GATT_ASCS_CLIENT_CONFIG_NOTIFY &&
            config->aseCharClientCfg[i] != GATT_ASCS_CLIENT_CONFIG_NOT_SET)
        {
            ascsRemoveConnection(ascs, connection);
            GATT_ASCS_SERVER_DEBUG_INFO(("Invalid Client Configuration ASE Characteristic!\n"));
            return gatt_status_value_not_allowed;/*Only Notify and NO SET*/
        }
    }

    connection->aseControlPointNotify.clientCfg = config->aseControlPointCharClientCfg;

    if (connection->aseControlPointNotify.clientCfg == GATT_ASCS_CLIENT_CONFIG_NOTIFY)
    {
        ascsAseControlPointNotify(ascs, cid);
    }

    for (int i = 0; i < connection->numAses; i++)
    {
        connection->ase[i].clientCfg = config->aseCharClientCfg[i];
        if(connection->ase[i].clientCfg == GATT_ASCS_CLIENT_CONFIG_NOTIFY)
        {
            ascsAseClientNotify(ascs, cid, &connection->ase[i]);
        }
    }

    return gatt_status_success;
}

static void ascsAseControlPointCharacteristicAddResponseCodeFromBap(GattAscsAseControlPointNotify* ase_control_point_notify,
                                                               uint8 aseId,
                                                               uint8 ase_result,
                                                               uint8 additional_info)
{
    AscsAseResponse ase_response;

    ase_response.aseId = aseId;
    ase_response.responseCode = ase_result; /* The ASCS/BAP API ase_result values map directly to ASCS reponse_codes */
    ase_response.reason = additional_info;   /* The ASCS/BAP API additional_info values map directly to ASCS reason values*/

#ifdef ASCS_SERVER_DEBUG
    /* ASCS Validation r05 states that the response code 'Unsupported Configuration Parameter Value'
     * 'Shall not be used when the Reason value is 0x05 (Framing)'
     * The framing value specified by the client (in the Configure Qos operation) is validated by ASCS in
     * ascsValidateConfigureQosInfo(), and if the value is not ASCS spec compliant then it is reported
     * to the client as an 'invalid parameter value'.
     * There isn't much that can be done if the Application/Profile then responds to the GATT_ASCS_SERVER_CONFIGURE_QOS_IND
     * with this incompatible combination, but a debug statement could be useful.
     */
    if (ase_result      == GATT_ASCS_ASE_RESULT_UNSUPPORTED_CONFIGURATION_PARAMETER_VALUE &&
        additional_info == GATT_ASCS_ASE_RESULT_ADDITIONAL_INFO_PARAMETER_FRAMING)
    {
        ASCS_SERVER_LOG("ascsAseControlPointCharacteristicAddResponseCodeFromBap: incompatible combination: \'unsupported parameter value\' and \'framing\'");
    }
#endif

    ascsAseControlPointCharacteristicAddAseResponse(ase_control_point_notify, &ase_response);
}

static void ascsHandleCodecConfiguredRequest(const GattAscsServer *ascs_server, GattAscsServerConfigureCodecReq* request)
{
    GattAscsConnection* connection = ascsFindConnection(ascs_server, request->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    for (int i = 0; i < request->numAses; i++)
    {
        GattAscsServerAse* ase = ascsConnectionFindAse(connection, request->ase[i].aseId);
        if (ase) /* The codecId and direction should already be stored */
        {
            ase->cachedConfigureCodecInfo.infoFromServer = *request->ase[i].gattAscsServerConfigureCodecServerInfo;

            /*  The new state is 'codec configured' */
            ascsServerSetAseStateAndNotify(ascs_server, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED);
        }
        /*
         * The request itself is freed by the profile/app, but the memory pointed to by fields within the response are
         * freed here
         */
    }
}

/***************************************************************************
NAME
    ascsAseHandleCodecConfiguredResponse

DESCRIPTION
    Process the response from the profile/app
*/

static void ascsHandleCodecConfiguredResponse(const GattAscsServer *ascs_server, GattAscsServerConfigureCodecRsp* response)
{
    GattAscsConnection* connection = ascsFindConnection(ascs_server, response->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

     /* Update the ASE Control Point Notify with the information in the Response from the profile/app */
    for (int i = 0; i < response->numAses; i++)
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
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotify(ascs_server, response->cid);

    for (int i = 0; i < response->numAses; i++)
    {
        if (response->ase[i].gattAscsAseResult.value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, response->ase[i].gattAscsAseResult.aseId);
            if (ase) /* The codecId and direction should already be stored */
            {
                ase->cachedConfigureCodecInfo.infoFromServer = *response->ase[i].gattAscsServerConfigureCodecServerInfo;

                /*  The new state is 'codec configured' */
                ascsServerSetAseStateAndNotify(ascs_server, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED);
            }
        }
        /*
         * The response itself is freed by the profile/app, but the memory pointed to by fields within the response are
         * freed here
         */
        /*
         * TODO: multiple ASEs can point to the same codec_config data, so (when multiple ASEs are supported) this
         * free statement must protect against double freeing.
         */
        free(response->ase[i].gattAscsServerConfigureCodecServerInfo);
    }
}

static uint8* ascsAseConstructQosConfiguredCharacteristicValue(GattAscsServerAse* ase, uint8* characteristic_length)
{
    GattAscsBuffIterator iter;
    uint8* characteristic_value;

    *characteristic_length = sizeof(struct
                                          {
                                             /*
                                              * Fields in the ASE Characteristic : ASCS d09r06
                                              */
                                              uint8 aseId;
                                              uint8 ase_state;
                                              uint8 cigId;
                                              uint8 cisId;
                                              uint8 sduInterval[3];
                                              uint8 framing;
                                              uint8 phy;
                                              uint8 max_sdu[2];
                                              uint8 retransmissionNumber;
                                              uint8 maxTransportLatency[2];
                                              uint8 presentationDelay[3];
                                          });

    characteristic_value = PanicUnlessMalloc(*characteristic_length);

    ascsBuffIteratorInitialise(&iter, characteristic_value, *characteristic_length);

    /*
     * populate the characteristic_value
     */

    ascsBuffIteratorWrite8(&iter, ase->aseId);
    ascsBuffIteratorWrite8(&iter, ase->state);
    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureQosInfoFromServer.cigId);
    ascsBuffIteratorWrite8(&iter,ase->cachedConfigureQosInfoFromServer.cisId);
    ascsBuffIteratorWrite24(&iter,ase->cachedConfigureQosInfoFromServer.sduInterval);
    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureQosInfoFromServer.framing);
    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureQosInfoFromServer.phy);
    ascsBuffIteratorWrite16(&iter,ase->cachedConfigureQosInfoFromServer.maximumSduSize);
    ascsBuffIteratorWrite8(&iter, ase->cachedConfigureQosInfoFromServer.retransmissionNumber);
    ascsBuffIteratorWrite16(&iter,ase->cachedConfigureQosInfoFromServer.maxTransportLatency);
    ascsBuffIteratorWrite24(&iter,ase->cachedConfigureQosInfoFromServer.presentationDelay);

    if (ascsBuffIteratorErrorDetected(&iter))
    {

        free(characteristic_value);
        characteristic_value = NULL;
        *characteristic_length = 0;
    }
    return characteristic_value;
}

static void ascsHandleConfigureQosRequest(const GattAscsServer *ascs_server, GattAscsServerConfigureQosReq* request)
{
    GattAscsConnection* connection = ascsFindConnection(ascs_server, request->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    for (int i = 0; i < request->numAses; i++)
    {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, request->ase[i].aseId);
            if (ase)
            {
                ase->cachedConfigureQosInfoFromServer = request->ase[i];

                ascsServerSetAseStateAndNotify(ascs_server, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED);
            }
    }
    /*free(response);*/
}

static void ascsHandleConfigureQosResponse(const GattAscsServer *ascs_server, GattAscsServerConfigureQosRsp* response)
{
    GattAscsConnection* connection = ascsFindConnection(ascs_server, response->cid);

    if (connection == NULL)
    {
        return; /* We don't know about this connection id */
    }

    /* Update the ASE Control Point Notify with the information in the Response from the profile/app */
    for (int i = 0; i < response->numAses; i++)
    {
        if (response->ase[i].gattAscsAseResultValue != GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            ascsAseControlPointCharacteristicAddResponseCodeFromBap(&connection->aseControlPointNotify,
                                                                    response->ase[i].gattAscsServerConfigureQosRspInfo->aseId,
                                                                    response->ase[i].gattAscsAseResultValue,
                                                                    response->ase[i].additionalResultInfo);
        }
    }
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotify(ascs_server, response->cid);

    for (int i = 0; i < response->numAses; i++)
    {
        if (response->ase[i].gattAscsAseResultValue == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsConnectionFindAse(connection, response->ase[i].gattAscsServerConfigureQosRspInfo->aseId);
            if (ase)
            {
                ase->cachedConfigureQosInfoFromServer = *response->ase[i].gattAscsServerConfigureQosRspInfo;

                ascsServerSetAseStateAndNotify(ascs_server, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED);
            }
        }
        free(response->ase[i].gattAscsServerConfigureQosRspInfo);
    }
    /*free(response);*/
}

static void ascsAseControlPointNotifyFromGenericResponse(const GattAscsServer *ascs_server, GattAscsServerGenericRsp* response)
{
    /* Update the ASE Control Point Notify with the information in the Response from the profile/app */
    GattAscsConnection* connection = ascsFindConnection(ascs_server, response->cid);

    if (connection)
    {
        for (int i = 0; i < response->numAses; i++)
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
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotify(ascs_server, response->cid);
}

static void ascsHandleEnableResponse(const GattAscsServer *ascs_server, GattAscsServerEnableRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotifyFromGenericResponse(ascs_server, response);

    for (int i = 0; i < response->numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascs_server, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                ascsServerSetAseStateAndNotify(ascs_server, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_ENABLING);
            }
        }
    }
}

static void ascsHandleReceiverReadyRequest(const GattAscsServer *ascs_server, GattAscsServerReceiverReadyReq* request)
{
    for (int i = 0; i < request->numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascs_server, request->cid, request->aseId[i]);
        if (ase)
        {
            /*
             * TODO:  CHeck if this state change is only meant to happen for 'server is sink' or 'server is source' ASEs
             */
            ascsServerSetAseStateAndNotify(ascs_server, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_STREAMING);
        }
    }
}

static void ascsHandleReceiverReadyResponse(const GattAscsServer *ascs_server, GattAscsServerReceiverReadyRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotifyFromGenericResponse(ascs_server, response);

    for (int i = 0; i < response->numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascs_server, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                /* If the Server is the Audio Source then the server will receive the GATT_ASCS_SERVER_RECEIVER_READY_IND
                 * and respond by calling GattAscsReceiverReadyResponse(). This response is only appropriate when
                 * the server is the audio source.
                 */
                if (ase->cachedConfigureCodecInfo.direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
                {
                    ascsServerSetAseStateAndNotify(ascs_server, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_STREAMING);
                }
            }
        }
    }
}

static void ascsHandleUpdateMetadataResponse(const GattAscsServer *ascs_server, GattAscsServerUpdateMetadataRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotifyFromGenericResponse(ascs_server, response);

    for (int i = 0; i < response->numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascs_server, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                ascsAseClientNotify(ascs_server, response->cid, ase);
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
static void ascsHandleDisableResponse(const GattAscsServer *ascs_server, GattAscsServerDisableRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotifyFromGenericResponse(ascs_server, response);

    for (int i = 0; i < response->numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascs_server, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                if (ase->cachedConfigureCodecInfo.direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
                {
                    /* The server is now discarding any received audio data, so autonomously perform the
                     * Receiver Stop Ready operation, i.e. transition to QOS configured */
                    ascsServerSetAseStateAndNotify(ascs_server, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED);
                }
                else if (ase->cachedConfigureCodecInfo.direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
                {
                    /* Transition to 'disabling' and await the Receiver Stop Ready operation from the client before ceasing transmission */
                    ascsServerSetAseStateAndNotify(ascs_server, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_DISABLING);
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
static void ascsHandleDisableRequest(const GattAscsServer *ascs_server, GattAscsServerDisableReq* request)
{
    for (int i = 0; i < request->numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascs_server, request->cid, request->aseId[i]);
        if (ase)
        {
            if (ase->cachedConfigureCodecInfo.direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SINK)
            {
                /* The server calls the GattAscsServerDisableRequest API after it has stopped consuming audio data
                 * on all ASEs for which it is an audio data sink, so autonomously perform the Receiver Stop
                 * Ready operation, i.e. transition to QOS configured
                 */
                ascsServerSetAseStateAndNotify(ascs_server, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED);
            }
            else if (ase->cachedConfigureCodecInfo.direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
            {
                ascsServerSetAseStateAndNotify(ascs_server, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_DISABLING);
            }
        }
    }
}

static void ascsHandleReceiverStopReadyResponse(const GattAscsServer *ascs_server, GattAscsServerReceiverStopReadyRsp* response)
{
    /* Send the ASE Control Point Notify -
     * The 'reason' and 'response_code' will have been set either in the op_code handler functions
     * when decoding the ACCESS_IND or in the loop above.
     */
    ascsAseControlPointNotifyFromGenericResponse(ascs_server, response);

    for (int i = 0; i < response->numAses; i++)
    {
        if (response->gattAscsAseResult[i].value == GATT_ASCS_ASE_RESULT_SUCCESS)
        {
            GattAscsServerAse* ase = ascsServerFindAse(ascs_server, response->cid, response->gattAscsAseResult[i].aseId);
            if (ase)
            {
                if (ase->cachedConfigureCodecInfo.direction == GATT_ASCS_ASE_DIRECTION_SERVER_IS_AUDIO_SOURCE)
                {
                    /* The server has stopped transmitting, the ASE can transition to the QOS configured state */
                    ascsServerSetAseStateAndNotify(ascs_server, response->cid, ase, GATT_ASCS_SERVER_ASE_STATE_QOS_CONFIGURED);
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

static void ascsHandleReleaseRequest(const GattAscsServer *ascs_server, GattAscsServerReleaseReq* request)
{
    for (int i = 0; i < request->numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascs_server, request->cid, request->aseId[i]);
        if (ase)
        {
            ascsServerSetAseStateAndNotify(ascs_server, request->cid, ase, GATT_ASCS_SERVER_ASE_STATE_RELEASING);
            /*
             * The Release Request takes the ASE to the 'releasing' state. The ASE cannot enter the Qos Configured
             * again until a new Configure Qos operation is completed, providing new Qos configuration data.
             * The Qos configuration data currently stored by the ASE is no longer valid and must not
             * inadvertently be used in a subsequent call to (for example) ascsConnectionFindAseByCisInfo().
             */
            memset(&ase->cachedConfigureQosInfoFromServer, 0, sizeof(GattAscsServerConfigureQosInfo));
        }
    }
}

static void ascsHandleReleaseComplete(const GattAscsServer *ascs_server, GattAscsServerReleaseComplete* release_complete)
{
    for (int i = 0; i < release_complete->numAses; i++)
    {
        GattAscsServerAse* ase = ascsServerFindAse(ascs_server, release_complete->cid, release_complete->ase[i].aseId);
        if (ase)
        {
            if (release_complete->ase[i].cacheCodecConfiguration)
            {
                if (release_complete->ase[i].gattAscsServerConfigureCodecServerInfo)
                {
                    /*
                     * NOTE: The profile/app is responsible for freeing the memory pointed to by the
                     *       release_complete->ase[i].gattAscsServerConfigureCodecServerInfo.
                     */
                    ase->cachedConfigureCodecInfo.infoFromServer = *release_complete->ase[i].gattAscsServerConfigureCodecServerInfo;
                }
                ascsServerSetAseStateAndNotify(ascs_server, release_complete->cid, ase, GATT_ASCS_SERVER_ASE_STATE_CODEC_CONFIGURED);
            }
            else
            {
                free(ase->cachedConfigureCodecInfo.infoFromServer.codecConfiguration);
                ascsServerSetAseStateAndNotify(ascs_server, release_complete->cid, ase, GATT_ASCS_SERVER_ASE_STATE_IDLE);
            }
        }
    }
}

/* Only one instance of Audio Stream Endpoint Service is supported */
/****************************************************************************/
ServiceHandle GattAscsServerInit(Task appTask,
                                 uint16 start_handle,
                                 uint16 end_handle
                                 )
{   
    /*Registration parameters for ASCS library to GATT manager  */
    gatt_manager_server_registration_params_t reg_params;
    ServiceHandle service_handle;
    GattAscsServer* ascs;

    /* validate the input parameters */
    if (appTask == NULL)
    {

        GATT_ASCS_SERVER_PANIC(("ASCS: Invalid Initialisation parameters"));
    }

    /* Allocate memory for the new instance and assign a service_handle */
    service_handle = ServiceHandleNewInstance((void **)&ascs, sizeof(GattAscsServer));
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Unable to allocate an ASCS Service Instance"));
    }
    GATT_ASCS_SERVER_SET_GASCS_DEBUG(ascs);    

    /* Reset all the service library memory, e.g. num_connections = 0  */
    memset(ascs, 0, sizeof(GattAscsServer));


    /* Set up the library task handler for external messages
     * ASCS library receives gatt manager messages here
     */
    ascs->libTask.handler = ascsServerMsgHandler;

    /* Store application message handler as application messages need to be posted here */
    ascs->appTask = appTask;

    /* Fill in the registration parameters */
    reg_params.start_handle = start_handle;
    reg_params.end_handle   = end_handle;
    reg_params.task = &ascs->libTask;
    /* Try to register this instance of ASCS library to Gatt manager */
    if (GattManagerRegisterServer(&reg_params) != gatt_manager_status_success)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: GattManagerRegisterServer failed \n\n\n"));
    }
    return service_handle;
}

/****************************************************************************/
void GattAscsServerConfigureCodecRequest(ServiceHandle service_handle, GattAscsServerConfigureCodecReq* configure_codec_request)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (configure_codec_request->cid == 0)
    {
        return;
    }

    ascsHandleCodecConfiguredRequest(ascs, configure_codec_request);
}

/****************************************************************************/
void GattAscsServerConfigureCodecResponse(ServiceHandle service_handle, GattAscsServerConfigureCodecRsp* configure_codec_response)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (configure_codec_response->cid == 0)
    {
        return;
    }

    ascsHandleCodecConfiguredResponse(ascs, configure_codec_response);

}

/****************************************************************************/
void GattAscsServerConfigureQosRequest(ServiceHandle service_handle, GattAscsServerConfigureQosReq* configure_qos_request)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (configure_qos_request->cid == 0)
    {
        return;
    }
    ascsHandleConfigureQosRequest(ascs, configure_qos_request);
}

/****************************************************************************/
void GattAscsServerConfigureQosResponse(ServiceHandle service_handle, GattAscsServerConfigureQosRsp* configure_qos_response)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (configure_qos_response->cid == 0)
    {
        return;
    }
    ascsHandleConfigureQosResponse(ascs, configure_qos_response);

}

void GattAscsServerEnableResponse(ServiceHandle service_handle, GattAscsServerEnableRsp* enable_response)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (enable_response->cid == 0)
    {
        return;
    }
    ascsHandleEnableResponse(ascs, enable_response);
}

void GattAscsReceiverReadyRequest(ServiceHandle service_handle, GattAscsServerReceiverReadyReq* receiver_ready_request)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (receiver_ready_request->cid == 0)
    {
        return;
    }
    ascsHandleReceiverReadyRequest(ascs, receiver_ready_request);
}

void GattAscsReceiverReadyResponse(ServiceHandle service_handle, GattAscsServerReceiverReadyRsp* receiver_ready_response)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (receiver_ready_response->cid == 0)
    {
        return;
    }
    ascsHandleReceiverReadyResponse(ascs, receiver_ready_response);
}

void GattAscsServerUpdateMetadataResponse(ServiceHandle service_handle, GattAscsServerUpdateMetadataRsp* update_metadata_response)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (update_metadata_response->cid == 0)
    {
        return;
    }
    ascsHandleUpdateMetadataResponse(ascs, update_metadata_response);

}

void GattAscsServerDisableResponse(ServiceHandle service_handle, GattAscsServerDisableRsp* disable_response)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (disable_response->cid == 0)
    {
        return;
    }
    ascsHandleDisableResponse(ascs, disable_response);

}

void GattAscsServerDisableRequest(ServiceHandle service_handle, GattAscsServerDisableReq* disable_request)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (disable_request->cid == 0)
    {
        return;
    }
    ascsHandleDisableRequest(ascs, disable_request);
}

void GattAscsReceiverStopReadyResponse(ServiceHandle service_handle, GattAscsServerReceiverStopReadyRsp* receiver_stop_ready_response)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (receiver_stop_ready_response->cid == 0)
    {
        return;
    }
    ascsHandleReceiverStopReadyResponse(ascs, receiver_stop_ready_response);
}

void GattAscsServerReleaseCompleteRequest(ServiceHandle service_handle, GattAscsServerReleaseComplete* release_complete)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (release_complete->cid == 0)
    {
        return;
    }
    ascsHandleReleaseComplete(ascs, release_complete);
}

void GattAscsServerReleaseRequest(ServiceHandle service_handle, GattAscsServerReleaseReq* release_request)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);
    /* Validate this instance */
    if (ascs == NULL)
    {
        GATT_ASCS_SERVER_PANIC(("ASCS: Null instance"));
    }
    /* Validate the input parameters */
    if (release_request->cid == 0)
    {
        return;
    }
    ascsHandleReleaseRequest(ascs, release_request);
}


GattAscsServerConfigureCodecInfo* GattAscsReadCodecConfiguration(ServiceHandle service_handle, ConnectionId cid, uint8 aseId)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);

    GattAscsServerAse* ase = ascsServerFindAse(ascs, cid, aseId);

    if (ase)
    {
        return &ase->cachedConfigureCodecInfo;
    }
    return NULL;
}

GattAscsServerConfigureQosInfo* GattAscsReadQosConfiguration(ServiceHandle service_handle, ConnectionId cid, uint8 aseId)
{
    GattAscsServer *ascs = (GattAscsServer*)ServiceHandleGetInstanceData(service_handle);

    GattAscsServerAse* ase = ascsServerFindAse(ascs, cid, aseId);

    if (ase)
    {
        return &ase->cachedConfigureQosInfoFromServer;
    }
    return NULL;
}

