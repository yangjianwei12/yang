/*****************************************************************************
*
* Copyright (c) 2014 - 2017 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/** \file
 *
 *  IPC exposed interface to KIP.
 *
 */
/****************************************************************************
Include Files
*/

#ifndef IPC_KIP_H
#define IPC_KIP_H

#include "types.h"
#include "ipc_status.h"
#include "proc/proc.h"

/****************************************************************************
Public Type Declarations
*/

/*
 * Enumerations for signal IDs
 */

#define IPC_SIGNAL_ID_VALUE(ipc_signal_type, ipc_signal_source, ipc_signal_number)      ( \
        ( ((ipc_signal_type) & 0x1) << 15 ) |\
        ( ((ipc_signal_source) & 0x7) << 12 ) |\
        (ipc_signal_number) )

#define IPC_SIGNAL_TYPE_INTERNAL    0
#define IPC_SIGNAL_TYPE_FRAMEWORK   1

#define IPC_SIGNAL_SOURCE_INTERNAL  0
#define IPC_SIGNAL_SOURCE_KYMERA    1
#define IPC_SIGNAL_SOURCE_GENERIC   2


typedef enum
{
    IPC_SIGNAL_INTERNAL_0 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_INTERNAL, IPC_SIGNAL_SOURCE_INTERNAL, 0),
    IPC_SIGNAL_INTERNAL_1 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_INTERNAL, IPC_SIGNAL_SOURCE_INTERNAL, 1),
    IPC_SIGNAL_INTERNAL_2 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_INTERNAL, IPC_SIGNAL_SOURCE_INTERNAL, 2),
    IPC_SIGNAL_INTERNAL_3 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_INTERNAL, IPC_SIGNAL_SOURCE_INTERNAL, 3),
    IPC_SIGNAL_INTERNAL_4 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_INTERNAL, IPC_SIGNAL_SOURCE_INTERNAL, 4),
    IPC_SIGNAL_INTERNAL_5 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_INTERNAL, IPC_SIGNAL_SOURCE_INTERNAL, 5),
}IPC_SIGNAL_ID_INTERNAL;

typedef enum
{
    IPC_SIGNAL_KALIMBA_0 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_FRAMEWORK, IPC_SIGNAL_SOURCE_KYMERA, 0),
    IPC_SIGNAL_KALIMBA_1 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_FRAMEWORK, IPC_SIGNAL_SOURCE_KYMERA, 1),
    IPC_SIGNAL_KALIMBA_2 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_FRAMEWORK, IPC_SIGNAL_SOURCE_KYMERA, 2),
    IPC_SIGNAL_KALIMBA_3 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_FRAMEWORK, IPC_SIGNAL_SOURCE_KYMERA, 3),
    IPC_SIGNAL_KALIMBA_4 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_FRAMEWORK, IPC_SIGNAL_SOURCE_KYMERA, 4),
    IPC_SIGNAL_KALIMBA_5 = IPC_SIGNAL_ID_VALUE(IPC_SIGNAL_TYPE_FRAMEWORK, IPC_SIGNAL_SOURCE_KYMERA, 5),
}IPC_SIGNAL_ID_KALIMBA;


typedef struct
{
    uint16         seqno;
    uint16         msgid;
    uint16         length;
    uint16         *message;

} ipc_msg;

typedef enum IPC_DATA_DIRECTION
{
    IPC_DATA_CHANNEL_READ      = 0,
    IPC_DATA_CHANNEL_WRITE     = 1

} IPC_DATA_DIRECTION;

/* IPC events that KIP needs to handle */
typedef enum IPC_EVENT
{
    IPC_SETUP_READY                      = 0,
    IPC_SETUP_COMPLETE                   = 1,
    IPC_DATA_CHANNEL_ACTIVATED           = 2,
    IPC_DATA_CHANNEL_DEACTIVATED         = 3,
    IPC_TEARDOWN_COMPLETED               = 4,
    IPC_TEARDOWN_IND                     = 5,
    IPC_MSGHANDLER_FAILED                = 6,
    IPC_WATCHDOG_COMPLETED               = 7,
    IPC_RESET_REQUEST                    = 8,
    IPC_RESET_COMPLETED                  = 9,
    IPC_ERROR_IND                        = 10
} IPC_EVENT;


/****************************************************************************
Public Constant and macros
*/

/* TODO this is really not the best place for these constants, they should be private, but it is not worse than it used to be
    the real problem is that there should not be this dependency of KIP knowing about datachan... */
#define IPC_MAX_DATA_PORTS              32
#define IPC_MAX_DATA_CHANNELS           16
/* NB. lower half of the channels reserved for P0 and the top half for P1,
    so in effect the limit is actually IPC_MAX_DATA_CHANNELS/2 */

#define IPC_DATA_CHANNEL_PORT_BIT       8
#define IPC_DATA_CHANNEL_DIR_BIT        7

#define IPC_DATA_CHANNEL_PORT_MASK      0xFF
#define IPC_DATA_CHANNEL_NUM_MASK       0x7F
#define IPC_DATA_CHANNEL_DIR_MASK       1U

/*
 * Signal FIFO is used to send signal elements between cores, where signals
 * correspond to the data channel(s). The signal FIFO has a fixed depth.
 * Ideally, the signal fifo should be able to take one of each possible signal
 * per each data channel. However, the number of signals is a variable
 * specified as a parameter in 'ipc_setup_comms'.
 * So the FIFO depth should at least carry one signal per data channel.
 */
#define IPC_NUM_FIFO_ELEMENTS   IPC_MAX_DATA_CHANNELS

/*
 * Sequence number macro
 */
#define IPC_NEXT_SEQNO()                (ipc_next_seqno())

#define ipc_set_data_channelid(channel_number, direction, port)      \
                      ( (uint16) ( (((port)&IPC_DATA_CHANNEL_PORT_MASK)<<IPC_DATA_CHANNEL_PORT_BIT) |        \
                        (((direction)&IPC_DATA_CHANNEL_DIR_MASK)<<IPC_DATA_CHANNEL_DIR_BIT) |     \
                         ((channel_number)&IPC_DATA_CHANNEL_NUM_MASK) ) )

/* The following macros are exposed to stream to get port and direction from channel id */
#define ipc_get_data_channelid_port(chanid)                          \
                      (((chanid)>>IPC_DATA_CHANNEL_PORT_BIT)&IPC_DATA_CHANNEL_PORT_MASK)

#define ipc_get_data_channelid_dir(chanid)                           \
                      (((chanid)>>IPC_DATA_CHANNEL_DIR_BIT)&IPC_DATA_CHANNEL_DIR_MASK)

#define ipc_get_data_channelid_channum(chanid)                       \
                      ((chanid)&IPC_DATA_CHANNEL_NUM_MASK)

#define ipc_invert_chanid_dir(chanid)                                \
                      ((chanid) ^ (1U<<IPC_DATA_CHANNEL_DIR_BIT))

/*
 * Macros for Message Channel channel id
 */
#define IPC_CHANID_INACTIVE             0x00
#define IPC_CHANID_INPROGRESS           0x01
#define IPC_CHANID_ACTIVE               0x03
#define IPC_CHANID_STATE_BIT            14
#define IPC_CHANID_OWNER_BIT            11
#define IPC_CHANID_PARTICIPANT_BIT      8

#define ipc_set_chanid(chan_state, chan_owner, chan_ptpt, offset)     \
                      ( (((chan_state)&3)<<IPC_CHANID_STATE_BIT) | (((chan_owner)&7)<<IPC_CHANID_OWNER_BIT) | (((chan_ptpt)&7)<<IPC_CHANID_PARTICIPANT_BIT) | ((offset)&15) )

#define ipc_get_chanid_owner(chanid)                          \
                      (((chanid)>>IPC_CHANID_OWNER_BIT)&7)

/* The following macro is exposed to kip_mgr. */
#define ipc_get_chanid_participant(chanid)                    \
                      (((chanid)>>IPC_CHANID_PARTICIPANT_BIT)&7)

#define ipc_get_chanid_remote_proc(chanid) \
               (ipc_get_chanid_owner(chanid) == hal_get_reg_processor_id())? \
                ipc_get_chanid_participant(chanid): ipc_get_chanid_owner(chanid)


/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/
/**
 * \brief  Basic IPC shutdown
 *
 */
extern void ipc_shutdown(void);
/**
 * \brief Start IPC
 *
 */
extern void ipc_init(void);

/**
 * \brief ipc_next_seqno
 *
 * \return  next sequence number for external messages
 *
 */
extern uint16 ipc_next_seqno(void);

/**
 * \brief Send an external IPC message
 *
 * \param[in] channel id - Message channel ID (CS-336170-DD Figure 14 in 10.1.3)
 * \param[in] msg        - Pointer to message structure which contains :
 *                         > seqno   - The IPC message sequence ID
 *                         > msgid   - The IPC message ID
 *                         > length  - the number of uint16s in 'message' buffer
 *                         > message - Pointer to buffer of data to send over IPC
 *
 * \return  0 if successfully sent external message else error code
 */
extern IPC_STATUS ipc_send_message(uint16 channel_id, ipc_msg *msg);

#if defined(INSTALL_EFUSE_FLEXROM_FEATURES)
/**
 * \brief  Return pointer to copy of efuse block in aux_states.
 *         The original efuse block was retrieved from curator
 *         but is not visible from P1. Upon P1 start, when the
 *         aux_states are alocated, a copy of the efuses is
 *         kept there. This function returns a pointer to that
 *         copy.
 *
 * \return Pointer to copy of efuses block
 */
extern uint16 *ipc_get_ptr_to_efuse_copy(void);
#endif /* INSTALL_EFUSE_FLEXROM_FEATURES */

#endif /* IPC_KIP_H */
