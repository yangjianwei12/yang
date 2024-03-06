/*******************************************************************************

Copyright (C) 2018-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP private interface.
 */

/**
 * \defgroup BAP_CLIENT_LIST_UTIL_PRIVATE_H_ BAP Private Interface
 * @{
 */

#ifndef BAP_CLIENT_LIST_UTIL_PRIVATE_H_
#define BAP_CLIENT_LIST_UTIL_PRIVATE_H_

#include "bap_client_type_name.h"
#include "bluetooth.h"
#include "bap_client_list.h"
#include "bap_client_list_util.h"

#include "csr_bt_gatt_prim.h"
#include "service_handle.h"


#ifdef __cplusplus
extern "C" {
#endif

#define GLOBAL_BAP_PROFILE_ID         (0x8FD1)
#define GLOBAL_ACTIVITY_ID            (0x01)

/*! BAP PAC Codec Specific Configurations  */
#define BAP_PAC_SAMPLING_FREQUENCY_LENGTH                   0x03
#define BAP_PAC_SAMPLING_FREQUENCY_TYPE                     0x01
#define BAP_PAC_SUPPORTED_FRAME_DURATION_LENGTH             0x02
#define BAP_PAC_SUPPORTED_FRAME_DURATION_TYPE               0x02
#define BAP_PAC_AUDIO_CHANNEL_COUNTS_LENGTH                 0x02
#define BAP_PAC_AUDIO_CHANNEL_COUNTS_TYPE                   0x03
#define BAP_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_LENGTH     0x05
#define BAP_PAC_SUPPORTED_OCTETS_PER_CODEC_FRAME_TYPE       0x04
#define BAP_PAC_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU_TYPE     0x05
#define BAP_PAC_SUPPORTED_MAX_CODEC_FRAMES_PER_SDU_LEN      0x02

#define BAP_PAC_PREFERRED_AUDIO_CONTEXT     0x01
#define BAP_PAC_STREAMING_AUDIO_CONTEXT     0x02
#define BAP_PAC_VENDOR_SPECIFIC_METADATA    0xFF

#define BAP_PAC_PREFERRED_AUDIO_CONTEXT_LEN     0x03
#define BAP_PAC_STREAMING_AUDIO_CONTEXT_LEN     0x03

/* BAP ASCS Codec configuration parameters */
#define SAMPLING_FREQUENCY_LENGTH           0x02
#define SAMPLING_FREQUENCY_TYPE             0x01
#define SAMPLING_FREQUENCY_TYPE_VS          0x81

#define FRAME_DURATION_LENGTH               0x02
#define FRAME_DURATION_TYPE                 0x02

#define AUDIO_CHANNEL_ALLOCATION_LENGTH     0x05
#define AUDIO_CHANNEL_ALLOCATION_TYPE       0x03
#define AUDIO_CHANNEL_ALLOCATION_TYPE_VS    0x83

#define OCTETS_PER_CODEC_FRAME_LENGTH       0x03
#define OCTETS_PER_CODEC_FRAME_TYPE         0x04

#define LC3_BLOCKS_PER_SDU_LENGTH           0x02
#define LC3_BLOCKS_PER_SDU_TYPE             0x05

#define STREAMING_AUDIO_CONTEXT_LENGTH      0x03
#define STREAMING_AUDIO_CONTEXT_TYPE        0x02

#define PREFFERED_AUDIO_CONTEXT_LENGTH      0x03
#define PREFFERED_AUDIO_CONTEXT_TYPE        0x01

#define SAMPLING_FREQUENCY_8kHz          (0x01)
#define SAMPLING_FREQUENCY_11_025kHz     (0x02)
#define SAMPLING_FREQUENCY_16kHz         (0x03)
#define SAMPLING_FREQUENCY_22_050kHz     (0x04)
#define SAMPLING_FREQUENCY_24kHz         (0x05)
#define SAMPLING_FREQUENCY_32kHz         (0x06)
#define SAMPLING_FREQUENCY_44_1kHz       (0x07)
#define SAMPLING_FREQUENCY_48kHz         (0x08)
#define SAMPLING_FREQUENCY_88_200kHz     (0x09)
#define SAMPLING_FREQUENCY_96kHz         (0x0A)
#define SAMPLING_FREQUENCY_176_420kHz    (0x0B)
#define SAMPLING_FREQUENCY_192kHz        (0x0C)
#define SAMPLING_FREQUENCY_384kHz        (0x0D)


typedef int16_t Status;

#define STATUS_SUCCESS                        (0)
#define STATUS_ERROR                         (-1)
#define STATUS_ERROR_TIMEOUT                 (-2)
#define STATUS_ERROR_NO_MEMORY               (-3)
#define STATUS_SCHEDULER_ALREADY_INITIALISED (-4)

typedef enum
{
    BAP_ANNOUNCER_STATE_IDLE,
    BAP_ANNOUNCER_STATE_ANNOUNCING_TARGETED,
    BAP_ANNOUNCER_STATE_ANNOUNCING_GENERAL
} BapAnnouncerState;

typedef enum
{
    BAP_DISCOVERER_STATE_IDLE,
    BAP_DISCOVERER_STATE_DISCOVERING
} BapDiscoverState;

typedef enum
{
    BAP_CONTROLLER_STATE_IDLE,
    BAP_CONTROLLER_STATE_CONNECTING,
    BAP_CONTROLLER_STATE_CONNECTED
} BapControllerState;

/*! \brief BAP struct.
 */

typedef struct BAP
{
    CsrBtGattId           gattId;
    phandle_t             appPhandle;

#ifdef INSTALL_LEA_UNICAST_CLIENT
    BapClientList         profileList;
    BapClientList         connectionList;
    BapClientList         streamGroupList;
    struct
    {
        BapControllerState  state;
        struct BapConnection *connection;
    } controller;
    type_name_declare_rtti_member_variable
#endif

#ifdef INSTALL_LEA_BROADCAST_SOURCE
    BapClientList         bigStreamList; /* list for Broadcast source instance */
#endif
} BAP;

struct BapProfile;
struct BapStreamGroup;
struct BapAse;
struct BAP_INITIATOR_CONNECTION;
struct BAP_ACCEPTOR_CONNECTION;


#define CSR_BT_GATT_UUID_ASCS ((CsrBtUuid16) 0x184E)
#define CSR_BT_GATT_UUID_PACS ((CsrBtUuid16) 0x1850)
#define CSR_BT_GATT_UUID_BASS ((CsrBtUuid16) 0x184F)

#define BAP_SERVICE_DATA_AD_TYPE (0x16u)

/* Targeted announcement UUID as defined in GAM IOP Test Plan 07r10-clean spec. */
#define BAP_TARGETED_ANNOUNCEMENT_UUID (0x7fffu)

/* General announcement UUID as defined in GAM IOP Test Plan 07r10-clean spec. */
#define BAP_GENERAL_ANNOUNCEMENT_UUID (0x7ffeu)


/*! \brief Invalid CIG ID value. */
#define INVALID_CIG_ID 0xff

/*! \brief Invalid CIS ID value. */
#define INVALID_CIS_ID 0xff

/*! \brief Invalid ASE ID value.
 *         ASCS d09r01 states that: [The ase_id] "Shall not be assigned a value of 0x00 by the server" */
#define INVALID_ASE_ID 0x00

/*! \brief Invalid CIS handle value. */
#define INVALID_CIS_HANDLE 0xffff

/*! \brief Initialise the BAP structure.
 *
 *  \param [in] bap A pointer to a BAP structure.
 *
 *  \return Nothing.
 */
void bapClientInitialiseList(BAP * const bap);

/*! \brief Start the BAP module.
 *
 *  \details This functions is responsible for starting the BAP module after the Scheduler has been
 *           started and the HCI interface is up and running.
 *
 *  \return Nothing.
 */
void bapClientStart(void);

#ifdef INSTALL_LEA_UNICAST_CLIENT
Status bapClientAddPacRecord(BAP * const bap,
                             uint16 profileId,
                             BapServerDirection serverDirection,
                             BapCodecSubRecord* codecSubrecord,
                             uint16 * const pacRecordId);

Status bapClientRemovePacRecord(BAP* const bap,
                                uint16 profileId,
                                uint16 pacRecordId);

Status bapClientAddCodecSubrecordRecord(BAP * const bap,
                                        uint16 pacRecordId,
                                        BapServerDirection serverDirection,
                                        BapCodecSubRecord * const codecSubrecord);

void bapClientSetAdvData(BAP * const bap,
                         uint8 advDataLen,
                         uint8 * const advData);

Bool bapClientFindProfileById(BAP * const bap,
                              uint16 profileId,
                              struct BapProfile ** const profile);

Bool bapClientFindConnectionByCid(BAP * const bap,
                                  BapProfileHandle cid,
                                  struct BapConnection ** const connection);

Bool bapClientFindConnectionByPacsSrvcHndl(BAP* const bap,
                                           ServiceHandle pacsSrvcHndl,
                                           struct BapConnection** const connection);

Bool bapClientFindConnectionByAscsSrvcHndl(BAP* const bap,
                                           ServiceHandle ascsSrvcHndl,
                                           struct BapConnection** const connection);


Bool bapClientFindConnectionByTypedBdAddr(BAP * const bap,
                                          TYPED_BD_ADDR_T * const peerAddrt,
                                          struct BapConnection ** const connection);

void bapClientAddStreamGroup(BAP * const bap,
                             struct BapStreamGroup * const streamGroup);

Bool bapClientFindStreamGroupById(BAP * const bap,
                                  BapProfileHandle streamGroupId,
                                  struct BapStreamGroup ** const streamGroup);

Bool bapClientFindStreamGroupByCigId(BAP * const bap,
                                     uint8 cigId,
                                     struct BapStreamGroup ** const streamGroup);

void bapClientDestroyList(BAP * const bap);

void bapClientSendDeinitCfmSuccess(BAP *const bap,
                                   struct BapConnection *const connection);
#endif
#ifdef __cplusplus
}
#endif

#endif /* BAP_PRIVATE_H_ */

/**@}*/
