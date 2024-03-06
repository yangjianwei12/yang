/*******************************************************************************

Copyright (C) 2020-2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP Broadcast Assistant private interface.
 */

/**
 * \defgroup Broadcast_Assistant_Private_Interface BAP
 * @{
 */

#ifndef BAP_BROADCAST_ASSISTANT_PRIVATE_H_
#define BAP_BROADCAST_ASSISTANT_PRIVATE_H_

#include "bluetooth.h"
#include "../bap_client_list_util.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef INSTALL_LEA_BROADCAST_ASSISTANT

typedef uint8 bapAssistantState;
#define BAP_ASSISTANT_STATE_IDLE              (0x00)
#define BAP_ASSISTANT_STATE_ADDING_SOURCE     (0x01)
#define BAP_ASSISTANT_STATE_MODIFYING_SOURCE  (0x02)
#define BAP_ASSISTANT_STATE_REMOVING_SOURCE   (0x03)
#define BAP_ASSISTANT_STATE_SEND_CODES        (0x04)
#define BAP_ASSISTANT_STATE_START_SCAN        (0x05)
#define BAP_ASSISTANT_STATE_STOP_SCAN         (0x06)
#define BAP_ASSISTANT_STATE_SYNC_TO_SRC       (0x07)

/* PA Sync operation by Assistant */
#define BAP_ASSISTANT_PA_NO_SYNC        (0x00u)
#define BAP_ASSISTANT_PA_SYNC           (0x01u)

/* PA Sync State */
#define BAP_ASSISTANT_PA_STATE_NO_SYNC        (0x00u)
#define BAP_ASSISTANT_PA_STATE_SYNC_INFO_REQ  (0x01u)
#define BAP_ASSISTANT_PA_STATE_SYNC           (0x02u)
#define BAP_ASSISTANT_PA_STATE_FAIL_TO_SYNC   (0x03u)
#define BAP_ASSISTANT_PA_STATE_NO_PAST        (0x04u)

/* BIG encryption state */
#define BAP_ASSISTANT_NOT_ENCRYPTED           (0x00u)
#define BAP_ASSISTANT_BROADCAST_CODE_REQ      (0x01u)
#define BAP_ASSISTANT_DECRYPTING              (0x02u)
#define BAP_ASSISTANT_BAD_CODE                (0x03u)

typedef struct BroadcastAssistant
{
    bapAssistantState  assistantState;
    BD_ADDR_T          sourceAddress;      /*! BT address of the Broadcast Source*/
    uint8            advertiseAddType;   /*! advertiser type of source */
    bool               srcCollocated;
    uint8            advSid;             /*! Advertising SID */
    uint8            paSyncState;        /*! PA Synchronization state */
    uint16           paInterval;         /*! PA interval */
    uint8            numbSubGroups;      /*! Number of subgroups */
    BapSubgroupInfo    subgroupInfo[BAP_MAX_SUPPORTED_NUM_SUBGROUPS]; 
    bool               sourceIdPending;
    uint8            sourceId;           /* Valid range is 0x00-0xFF */
    uint8            serviceDataOctet0;  /* Octet 0 of the service data */
    uint16           syncHandle;
    uint32           broadcastId;
    bool               controlOpResponse;  /*! TRUE if the operation has to be executed with the
                                               GATT Write procedure for Broadcast Audio Scan Control
                                               Point Operations with Scan Delegator*/
    bool               longWrite;
} BroadcastAssistant;

#endif /* INSTALL_LEA_BROADCAST_ASSISTANT */

#ifdef __cplusplus
}
#endif

#endif /* BAP_BROADCAST_ASSISTANT_PRIVATE_H_ */

/**@}*/
