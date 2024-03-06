/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#ifndef BAP_ASE_H
#define BAP_ASE_H

#include "bap_client_type_name.h"
#include "bap_client_list_element.h"
#include "bap_client_lib.h"
#include "bap_pac_record.h"
#include "bap_ascs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_UNICAST_CLIENT
#define BAP_ASE_PRESENTATION_DELAY_MIN_MICROSECONDS 0x00000000
#define BAP_ASE_PRESENTATION_DELAY_MAX_MICROSECONDS 0xFFFFFFFFu

#define BAP_ASE_START_ID       0x01
#define BAP_ASE_END_ID         0x02

typedef struct
{
    uint8 data[MAX_QOS_LEN];
} BapAseQos;

/*
 * Forward decl
 */
struct BapAseVTable;
struct BapConnection;

/*! \brief BapAse struct.
 */
typedef struct BapAse
{
    struct BapConnection*       connection;
    uint8                     id;
    uint16                    contextType;
    BapCodecIdInfo              codecId;
    struct BapCis*              cis;
    BapServerDirection          serverDirection;
    BapCodecConfiguration       codecConfiguration;
    BapIsocConfig               isocConfig;
    uint32                    presentationDelayMin;
    uint32                    presentationDelayMax;
    BapAseQos                   qos;                   /*!< tx qos (if local_direction is BAP_SERVER_DIRECTION_SOURCE)
                                                            rx qos (if local_direction is BAP_SERVER_DIRECTION_SINK) */
    struct BapStreamGroup*      streamGroup;
    BapClientPacRecord*         pacRecord;
    BapResult                   errorCode;
    BapAseState                 aseStateOnServer;
    const struct BapAseVTable*  vtable;
    type_name_declare_rtti_member_variable
} BapAse;

typedef struct BapAseVTable
{
    void (*handleAscsMsg)(BapAse* const, uint8 opcode, AscsMsg* const);
    void (*deleteInstance)(BapAse * const);
}BapAseVTable;

void bapAseInitialise(BapAse * const ase,
                      uint8 id,
                      uint16 contextType,
                      BapCodecIdInfo codecId,
                      struct BapCis* const cis,
                      BapServerDirection serverDirection,
                      struct BapConnection * const connection,
                      BapClientPacRecord * const pacRecord,
                      BapAseVTable const * const vtable);

void bapAseGetAseInfo(BapAse* const ase, BapAseInfo* aseInfo);

void bapAseVHandleAscsMsg(BapAse* ase,uint8 opcode, AscsMsg* msg);

bool bapAseSetQos(BapAse * const ase, uint8* qos);

bool bapAseQosDeserialise(BapAseQos* aseQos,
                          uint32* sduIntervalMicrosecs,
                          uint8*  packing,
                          uint8*  framing,
                          uint8*  phy,
                          uint16* maxSduSize,
                          uint8*  retransmissionEffort,
                          uint16* transportLatencyMilliseconds);

bool bapAseQosDeserialiseServerQos(uint8* ase_qos,
                                   uint32* sduIntervalMin,
                                   uint32* sduIntervalMax,
                                   uint8*  framing,
                                   uint8*  phy,
                                   uint16* maxSduSize,
                                   uint8*  retransmissionEffort,
                                   uint16* transportLatencyMilliseconds);

bool bapAseQosSerialise(BapAseQos* aseQos,
                        uint32 sduIntervalMicrosecs,
                        uint8  framing,
                        uint8  phy,
                        uint16 maxSduSize,
                        uint8  retransmissionEffort,
                        uint16 transportLatencyMilliseconds);
/*
 * This is following a pattern that appears in previous code in the company:
 * the 'destroy' function implements all the code needed to cleanup an
 * ase, it can include freeing memory of internally allocated
 * components and notifying users that this instance is being destroyed (so
 * they don't try to use it any more), but this function doesn't free the
 * memory of the ase itself; that would be done by calling the
 * (virtual) delete function instead. The delete is virtual so that the
 * memory for the right instance can be freed (the memory for the derived
 * class, e.g. the bap_client_ase etc.)
 */
void bapAseDestroy(BapAse * const ase);

#define bapAseHandleAscsMsg(ase,opcode, msg) \
    ((ase)->vtable->handleAscsMsg((ase), (opcode), (msg)))

#define bapAseDelete(ase) \
    ((ase)->vtable->deleteInstance(ase))

#define bapAseGetId(ase) ((ase)->id)

#define bapAseGetDirection(ase) ((ase)->server_direction)

/*
 * An BapAse cannot be constructed without passing in an BapCis* (so dereferencing without a check is okay)
 */
#define bapAseGetCigId(ase)               ((ase)->cis->cigId)

#define bapAseGetCisId(ase)               ((ase)->cis->cisId)

#define bapAseGetCisHandle(ase)           ((ase)->cis->cisHandle)

#define bapAseSetErrorCode(ase, errCode)  ((ase)->errorCode = (errCode))

#define bapAseSetPresentationDelay(ase, pd) ((ase)->presentationDelayMin = (pd))

#endif /* INSTALL_LEA_UNICAST_CLIENT */
#ifdef __cplusplus
}
#endif

#endif /* BAP_ASE_H */
