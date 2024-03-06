/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP BapAse interface implementation.
 */

/**
 * \addtogroup BAP
 * @{
 */

#include <string.h>
#include "bap_stream_group.h"
#include "bap_connection.h"
#include "bap_client_list_util_private.h"
#include "bap_ase.h"

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define BAP_ASE_QOS_RETRANSMISSION_EFFORT_MIN          ((uint8) (0x00))
#define BAP_ASE_QOS_RETRANSMISSION_EFFORT_MAX          ((uint8) (0xff))

/*
 * Search for comments about BAP_ASE_QOS_RETRANSMISSION_EFFORT_MIN before changing the compile time assert
 */
COMPILE_TIME_ASSERT((BAP_ASE_QOS_RETRANSMISSION_EFFORT_MIN == 0), RETRANSMISSION_EFFORT_MIN_is_not_zero_it_will_need_to_be_validated_dynamically);

#define BAP_ASE_QOS_SDU_INTERVAL_MIN                   ((uint32)(0x000000FF))
#define BAP_ASE_QOS_SDU_INTERVAL_MAX                   ((uint32)(0x00FFFFFF))

#define BAP_ASE_QOS_MAX_SDU_SIZE_MIN_ALLOWED_VALUE     ((uint16)(0x0000))
#define BAP_ASE_QOS_MAX_SDU_SIZE_MAX_ALLOWED_VALUE     ((uint16)(0x0FFF))

/*
 * Search for comments about BAP_ASE_QOS_MAX_SDU_SIZE_MIN_ALLOWED_VALUE before changing the compile time assert
 */
COMPILE_TIME_ASSERT((BAP_ASE_QOS_MAX_SDU_SIZE_MIN_ALLOWED_VALUE == 0), MAX_SDU_SIZE_MIN_is_not_zero_it_will_need_to_be_validated_dynamically);

#define BAP_ASE_QOS_TRANSPORT_LATENCY_MIN              ((uint16)(0x0000))
#define BAP_ASE_QOS_TRANSPORT_LATENCY_MAX              ((uint16)(0x0FA0))

/*
 * Search for comments about BAP_ASE_QOS_TRANSPORT_LATENCY_MIN before changing the compile time assert
 */
COMPILE_TIME_ASSERT((BAP_ASE_QOS_TRANSPORT_LATENCY_MIN == 0), TRANSPORT_LATENCY_MIN_is_not_zero_it_will_need_to_be_validated_dynamically);

#define BAP_ASE_QOS_PACKING_MIN              ((uint8)(0x00))
#define BAP_ASE_QOS_PACKING_MAX              ((uint8)(0x01))

/*
 * Search for comments about BAP_ASE_QOS_PACKING_MIN before changing the compile time assert
 */
COMPILE_TIME_ASSERT((BAP_ASE_QOS_PACKING_MIN == 0), BAP_ASE_QOS_PACKING_MIN_is_not_zero_it_will_need_to_be_validated_dynamically);

#define BAP_ASE_QOS_FRAMING_MIN              ((uint8)(0x00))
#define BAP_ASE_QOS_FRAMING_MAX              ((uint8)(0x01))

/*
 * Search for comments about BAP_ASE_QOS_FRAMING_MIN before changing the compile time assert
 */
COMPILE_TIME_ASSERT((BAP_ASE_QOS_FRAMING_MIN == 0), BAP_ASE_QOS_FRAMING_MIN_is_not_zero_it_will_need_to_be_validated_dynamically);

/* BAP_ASE_QOS_PACKING_MIN = 0 (verified by a compile time assert)
 * 'packing' is unsigned in all cases in this file, so _must_ be >= 0
 * (and is not checked below)
 */
#define bapAseQosDeserialisePacking(iter, packing)         \
            (*(packing) = buff_iterator_get_octet(iter),        \
          /* return */ (*(packing) <= BAP_ASE_QOS_PACKING_MAX))

/* BAP_ASE_QOS_FRAMING_MIN = 0 (verified by a compile time assert)
 * 'framing' is unsigned in all cases in this file, so _must_ be >= 0
 * (and is not checked below)
 */
#define bapAseQosDeserialiseFraming(iter, framing)         \
            (*(framing) = buff_iterator_get_octet(iter),        \
          /* return */ (*(framing) <= BAP_ASE_QOS_FRAMING_MAX))

/* temporary phy hack for new orleans */
#define bapAseQosDeserialisePhy(iter, phy)         \
            (*(phy) = buff_iterator_get_octet(iter),        \
          /* return */ (*(phy) <= 8))

/* BAP_ASE_QOS_RETRANSMISSION_EFFORT_MIN = 0 (verified by a compile time assert)
 * 'transmission_effort' is unsigned in all cases in this file, so _must_ be >= 0
 * (and is not checked below)
 */
#define bapAseQosDeserialiseRetransmissionEffort(iter, retransmissionEffort) \
            (*(retransmissionEffort) = buff_iterator_get_octet(iter),              \
          /* return */ (*(retransmissionEffort) <= BAP_ASE_QOS_RETRANSMISSION_EFFORT_MAX))

static void bapAseQosInitialise(BapAseQos * const aseQos);

static bool bapAseQosDeserialiseSduInterval(BUFF_ITERATOR* iter ,
                                            uint32 * sduIntervalMicrosecs);

static bool bapAseQosDeserialiseMaxSduSize(BUFF_ITERATOR* iter,
                                           uint16 * maxSduSize);

static bool bapAseQosDeserialiseTransportLatency(BUFF_ITERATOR* iter,
                                                 uint16 * transportLatencyMilliseconds);

static void bapAseVDelete(BapAse * const ase);

/*! \brief RTTI information for the BapAse structure.
 */
type_name_declare_and_initialise_const_rtti_variable(BapAse,  'A','s','A','s')

const BapAseVTable ase_vtable =
{
    bapAseVHandleAscsMsg,
    bapAseVDelete
};

static void bapAseQosInitialise(BapAseQos * const aseQos)
{
    memset(&aseQos->data[0], 0, MAX_QOS_LEN);
}

static bool bapAseQosDeserialiseSduInterval(BUFF_ITERATOR* iter ,
                                            uint32 * sduIntervalMicrosecs)
{
    *sduIntervalMicrosecs  = buff_iterator_get_octet(iter);
    *sduIntervalMicrosecs += buff_iterator_get_octet(iter) << 0x08;
    *sduIntervalMicrosecs += buff_iterator_get_octet(iter) << 0x10;

    return ((*sduIntervalMicrosecs >= BAP_ASE_QOS_SDU_INTERVAL_MIN) && (*sduIntervalMicrosecs <= BAP_ASE_QOS_SDU_INTERVAL_MAX));
}

static bool bapAseQosDeserialiseMaxSduSize(BUFF_ITERATOR* iter,
                                           uint16 * maxSduSize)
{
    *maxSduSize  = buff_iterator_get_octet(iter);
    *maxSduSize += (uint16)buff_iterator_get_octet(iter) << 0x08;

    /* BAP_ASE_QOS_MAX_SDU_SIZE_MIN_ALLOWED_VALUE = 0 (verified by a compile time assert)
     * max_sdu_size is unsigned, so _must_ be >= 0, so the dynamic check below is commented out
     */
    return (/*(*max_sdu_size >= BAP_ASE_QOS_MAX_SDU_SIZE_MIN_ALLOWED_VALUE) &&*/
            (*maxSduSize <= BAP_ASE_QOS_MAX_SDU_SIZE_MAX_ALLOWED_VALUE));
}

static bool bapAseQosDeserialiseTransportLatency(BUFF_ITERATOR* iter,
                                                 uint16 * transportLatencyMilliseconds)
{
    *transportLatencyMilliseconds  = buff_iterator_get_octet(iter);
    *transportLatencyMilliseconds += buff_iterator_get_octet(iter) << 0x08;

    /* BAP_ASE_QOS_TRANSPORT_LATENCY_MIN = 0 (verified by a compile time assert)
     * transport_latency is unsigned, so _must_ be >= 0, so the dynamic check below
     * is commented out
     */
    return (/*(*transportLatencyMilliseconds >= BAP_ASE_QOS_TRANSPORT_LATENCY_MIN) && */
            (*transportLatencyMilliseconds <= BAP_ASE_QOS_TRANSPORT_LATENCY_MAX));
}

bool bapAseQosDeserialise(BapAseQos* aseQos,
                          uint32* sduIntervalMicrosecs,
                          uint8*  packing,
                          uint8*  framing,
                          uint8*  phy,
                          uint16* maxSduSize,
                          uint8*  retransmissionEffort,
                          uint16* transportLatencyMilliseconds)
{
    BUFF_ITERATOR iter;
    (void)packing;

    buff_iterator_initialise(&iter, &aseQos->data[0]);

    return bapAseQosDeserialiseSduInterval(&iter, sduIntervalMicrosecs) &&
           bapAseQosDeserialiseFraming(&iter, framing) &&
           bapAseQosDeserialisePhy(&iter, phy) &&
           bapAseQosDeserialiseMaxSduSize(&iter, maxSduSize) &&
           bapAseQosDeserialiseRetransmissionEffort(&iter, retransmissionEffort) &&
           bapAseQosDeserialiseTransportLatency(&iter, transportLatencyMilliseconds);
}

bool bapAseQosDeserialiseServerQos(uint8* aseQos,
                                   uint32* sduIntervalMin,
                                   uint32* sduIntervalMax,
                                   uint8*  framing,
                                   uint8*  phy,
                                   uint16* maxSduSize,
                                   uint8*  retransmissionEffort,
                                   uint16* transportLatencyMilliseconds)
{
    BUFF_ITERATOR iter;

    buff_iterator_initialise(&iter, aseQos);

    return bapAseQosDeserialiseSduInterval(&iter, sduIntervalMin) &&
           bapAseQosDeserialiseSduInterval(&iter, sduIntervalMax) &&
           bapAseQosDeserialiseFraming(&iter, framing) &&
           bapAseQosDeserialisePhy(&iter, phy) &&
           bapAseQosDeserialiseMaxSduSize(&iter, maxSduSize) &&
           bapAseQosDeserialiseRetransmissionEffort(&iter, retransmissionEffort) &&
           bapAseQosDeserialiseTransportLatency(&iter, transportLatencyMilliseconds);
}

bool bapAseQosSerialise(BapAseQos* aseQos,
                        uint32 sduIntervalMicrosecs,
                        uint8  framing,
                        uint8  phy,
                        uint16 maxSduSize,
                        uint8  retransmissionEffort,
                        uint16 transportLatencyMilliseconds)
{

    BUFF_ITERATOR iter;
    buff_iterator_initialise(&iter, &aseQos->data[0]);

    buff_iterator_get_octet(&iter) = (sduIntervalMicrosecs >>  0) & 0x0000FF;
    buff_iterator_get_octet(&iter) = (sduIntervalMicrosecs >>  8) & 0x0000FF;
    buff_iterator_get_octet(&iter) = (sduIntervalMicrosecs >> 16) & 0x0000FF;
    buff_iterator_get_octet(&iter) = framing;
    buff_iterator_get_octet(&iter) = phy;
    buff_iterator_get_octet(&iter) = (maxSduSize >>  0) & 0x00FF;
    buff_iterator_get_octet(&iter) = (maxSduSize >>  8) & 0x00FF;
    buff_iterator_get_octet(&iter) = retransmissionEffort;
    buff_iterator_get_octet(&iter) = (transportLatencyMilliseconds >>  0) & 0x00FF;
    buff_iterator_get_octet(&iter) = (transportLatencyMilliseconds >>  8) & 0x00FF;

    return FALSE;
}

void bapAseInitialise(BapAse * const ase,
                      uint8 id,
                      uint16 contextType,
                      BapCodecIdInfo codecId,
                      struct BapCis* const cis,
                      BapServerDirection serverDirection,
                      BapConnection * const connection,
                      BapClientPacRecord * const pacRecord,
                      BapAseVTable const * const vtable)
{

    ase->id = id;
    ase->connection = connection;
    ase->contextType= contextType;
    ase->codecId = codecId;
    ase->cis = cis;
    ase->pacRecord = pacRecord;
    ase->serverDirection = serverDirection;
    ase->presentationDelayMin = 0;
    ase->presentationDelayMax = 0;
    ase->pacRecord = pacRecord;
    ase->streamGroup = NULL;

    ase->errorCode = BAP_RESULT_SUCCESS;
    ase->aseStateOnServer = BAP_ASE_STATE_IDLE;

    bapAseQosInitialise(&ase->qos);

    /*ase->codec_configuration.codec_specific_parameters = NULL;*/

    memset(&ase->codecConfiguration, 0, sizeof(BapCodecConfiguration));
    memset(&ase->isocConfig, 0, sizeof(BapIsocConfig));

    if (vtable == NULL)
    {
        ase->vtable = &ase_vtable;
    }
    else
    {
        ase->vtable = vtable;
    }

    type_name_initialise_rtti_member_variable(BapAse, ase);

    /* Safest to put this at the end after all member variables have been initialised (in case they
     * are used from inside bapCisAddAse() )*/
    if (cis)
    {
        bapCisAddAse(cis, ase);
    }
}

void bapAseGetAseInfo(BapAse* const ase, BapAseInfo* aseInfo)
{
    aseInfo->aseId = ase->id;
    aseInfo->cisId = ase->cis->cisId;
    aseInfo->aseState = ase->aseStateOnServer;
    aseInfo->errorCode = ase->errorCode;
}
/*
 * default 'empty' assc msg handler function
 */
void bapAseVHandleAscsMsg(BapAse* ase, uint8 opcode, AscsMsg* msg)
{
    (void) ase;
    (void)msg;
    (void)opcode;
}


bool bapAseSetQos(BapAse * const ase, uint8* qos)
{
    BUFF_ITERATOR iter;
    uint32 sduIntervalMicrosecs;
    uint8  packing;
    uint8  framing;
    uint8  phy;
    uint16 maxSduSize;
    uint8  retransmissionEffort;
    uint16 transportLatencyMilliseconds;

    buff_iterator_initialise(&iter, &ase->qos.data[0]);

    memcpy(&ase->qos.data[0], qos, MAX_QOS_LEN);

    /*
     * The deserialised values are discarded (it's more memory efficient to store the
     * qos octets 'as is'), BUT the deserialiser does validate the contents from a
     * protocol perspective
     * TODO: is there a way these values can be validated by our ISOC implementation?
     */
    return bapAseQosDeserialise(&ase->qos,
                                 &sduIntervalMicrosecs,
                                 &packing,
                                 &framing,
                                 &phy,
                                 &maxSduSize,
                                 &retransmissionEffort,
                                 &transportLatencyMilliseconds);
}

static void bapAseVDelete(BapAse * const ase)
{
    (void) ase;
    /*
     * Must be overridden to ensure the correct pointer is deleted
     */
}

void bapAseDestroy(BapAse * const ase)
{
    /*
     * The stream group is not the owner of the ase, but it does use
     * references to ases and so does need to know when an ase has
     * been destroyed
     */
    if (ase->streamGroup)
    {
        bapStreamGroupAseDestroyedInd(ase->streamGroup,
                                      ase);
    }

    ase->pacRecord = NULL;

    /* Keeping it for vendor defined codec id 
    if (ase->codec_configuration.codec_specific_parameters != NULL)
    {
        CsrPmemFree(ase->codec_configuration.codec_specific_parameters);
    }*/
    /*
     * Dynamic memory for this ase MUST be freed elsewhere, e.g. in the
     * derived class 'delete' function.
     */
}
#endif /* INSTALL_LEA_UNICAST_CLIENT */
/**@}*/
