/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Qualcomm Debug over Charger Comms channel messages.
*/

#ifndef QCOM_DEBUG_CHANNEL_MESSAGES_H_
#define QCOM_DEBUG_CHANNEL_MESSAGES_H_

#include <stdint.h>

/*-----------------------------------------------------------------------------
------------------ INCLUDES ---------------------------------------------------
-----------------------------------------------------------------------------*/

/** Type required for the Debug Unlock Header macros. */
typedef uint16_t uint16;

/**
 * DEBUG UNLOCK HEADER
 *
 * This uses the same format for both directions. The bits in the header that
 * are reserved for future definition (RFD) must be zero.
 *
 * HOST TO DEVICE (case to earbud):
 *
 * [15:3]   RFD
 * [2:1]    Action
 * [0]      RFD
 *
 * DEVICE TO HOST (earbud to case):
 *
 * [15:3]   RFD
 * [2:1]    Action that was requested
 * [0]      Status of requested action
 */

#define CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET    0
#define CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE      2
#define CHARGER_COMMS_DEBUG_UNLOCK_DATA_OFFSET \
    ((CHARGER_COMMS_DEBUG_UNLOCK_HEADER_OFFSET) + (CHARGER_COMMS_DEBUG_UNLOCK_HEADER_SIZE))

#define CHARGER_COMMS_DEBUG_UNLOCK_STATUS_FALSE     0
#define CHARGER_COMMS_DEBUG_UNLOCK_STATUS_TRUE      1

#define CHARGER_COMMS_DEBUG_UNLOCK_STATUS_LSB       0
#define CHARGER_COMMS_DEBUG_UNLOCK_STATUS_MASK      \
    (1 << (CHARGER_COMMS_DEBUG_UNLOCK_STATUS_LSB))

#define CHARGER_COMMS_DEBUG_UNLOCK_STATUS_SET(x, v) \
    (void)( \
        (x) = (uint16) \
        ((x) & ~(CHARGER_COMMS_DEBUG_UNLOCK_STATUS_MASK)) | \
        ((uint16)((v) << (CHARGER_COMMS_DEBUG_UNLOCK_STATUS_LSB))) \
    )
#define CHARGER_COMMS_DEBUG_UNLOCK_STATUS_GET(x) \
    (((x) & (CHARGER_COMMS_DEBUG_UNLOCK_STATUS_MASK)) >> \
        (CHARGER_COMMS_DEBUG_UNLOCK_STATUS_LSB))

typedef enum
{
    /**
     * \brief Request the random token from the device
     *
     * Response status:
     * - TRUE: Success
     * - FALSE: Failed to retrieve random token
     */
    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_READ_RANDOM_TOKEN     = 0,
    /**
     * \brief Send the unlock token to the device
     *
     * Response status:
     * - TRUE: The debug interface is unlocked
     * - FALSE: The action failed
     */
    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_WRITE_UNLOCK_TOKEN    = 1,
    /**
     * \brief Request the debug unlock status of the device
     *
     * Response status:
     * - TRUE: The debug interface is unlocked
     * - FALSE: The debug interface is locked
     */
    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_READ_UNLOCK_STATUS    = 2,
    /**
     * \brief Lock the debug interface on the device
     *
     * Response status:
     * - TRUE: The debug interface is locked
     */
    CHARGER_COMMS_DEBUG_UNLOCK_ACTION_LOCK_DEBUG            = 3
} CHARGER_COMMS_DEBUG_UNLOCK_ACTION;

#define CHARGER_COMMS_DEBUG_UNLOCK_ACTION_LSB       1
#define CHARGER_COMMS_DEBUG_UNLOCK_ACTION_MASK      \
    (7 << (CHARGER_COMMS_DEBUG_UNLOCK_ACTION_LSB))

#define CHARGER_COMMS_DEBUG_UNLOCK_ACTION_SET(x, v) \
    (void)( \
        (x) = (uint16) \
        ((x) & ~(CHARGER_COMMS_DEBUG_UNLOCK_ACTION_MASK)) | \
        ((uint16)((v) << (CHARGER_COMMS_DEBUG_UNLOCK_ACTION_LSB))) \
    )
#define CHARGER_COMMS_DEBUG_UNLOCK_ACTION_GET(x) \
    ((CHARGER_COMMS_DEBUG_UNLOCK_ACTION) \
        ((x) & (CHARGER_COMMS_DEBUG_UNLOCK_ACTION_MASK)) >> \
        (CHARGER_COMMS_DEBUG_UNLOCK_ACTION_LSB))

#endif /* QCOM_DEBUG_CHANNEL_MESSAGES_H_ */
