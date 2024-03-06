/*****************************************************************************

            (c) Qualcomm Technologies International, Ltd. 2023
            Confidential information of Qualcomm

            WARNING: This is an auto-generated file!
                     DO NOT EDIT!

*****************************************************************************/
#ifndef IPC_MSG_PRIM_H
#define IPC_MSG_PRIM_H


#define IPC_MSG_PROTOCOL_HEADER_WORD                                    (0xC409)


/*******************************************************************************

  NAME
    IPC_MSG_ID

  DESCRIPTION
    **************** REQ IDs **************************** ****************
    RESP IDs **************************** Make sure that the responses are
    identical to their requests, but with bit 14 set.

 VALUES
    watchdog_ping_req           -
    reset_ipc_req               -
    teardown_ipc_req            -
    data_channel_activate_req   -
    data_channel_deactivate_req -
    IPC_ERROR_IND               -
    WATCHDOG_PING_RES           -
    RESET_IPC_RES               -
    TEARDOWN_IPC_RES            -
    DATA_CHANNEL_ACTIVATE_RES   -
    DATA_CHANNEL_DEACTIVATE_RES -
    PX_SETUP_READY_IND          -

*******************************************************************************/
typedef enum
{
    IPC_MSG_ID_WATCHDOG_PING_REQ = 0x0014,
    IPC_MSG_ID_RESET_IPC_REQ = 0x0015,
    IPC_MSG_ID_TEARDOWN_IPC_REQ = 0x0016,
    IPC_MSG_ID_DATA_CHANNEL_ACTIVATE_REQ = 0x0018,
    IPC_MSG_ID_DATA_CHANNEL_DEACTIVATE_REQ = 0x0019,
    IPC_MSG_ID_IPC_ERROR_IND = 0x4013,
    IPC_MSG_ID_WATCHDOG_PING_RES = 0x4014,
    IPC_MSG_ID_RESET_IPC_RES = 0x4015,
    IPC_MSG_ID_TEARDOWN_IPC_RES = 0x4016,
    IPC_MSG_ID_DATA_CHANNEL_ACTIVATE_RES = 0x4018,
    IPC_MSG_ID_DATA_CHANNEL_DEACTIVATE_RES = 0x4019,
    IPC_MSG_ID_PX_SETUP_READY_IND = 0x401A
} IPC_MSG_ID;


#define IPC_MSG_PRIM_ANY_SIZE 1

/*******************************************************************************

  NAME
    IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ

  DESCRIPTION

  MEMBERS
    data_channel_id - Data channel ID
    channel         - Pointer to data channel object for the given data channel
                      ID

*******************************************************************************/
typedef struct
{
    uint16 _data[3];
} IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ;

/* The following macros take IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ *ipc_msg_data_channel_activate_req_ptr */
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_DATA_CHANNEL_ID_WORD_OFFSET (0)
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_DATA_CHANNEL_ID_GET(ipc_msg_data_channel_activate_req_ptr) ((ipc_msg_data_channel_activate_req_ptr)->_data[0])
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_DATA_CHANNEL_ID_SET(ipc_msg_data_channel_activate_req_ptr, data_channel_id) ((ipc_msg_data_channel_activate_req_ptr)->_data[0] = (uint16)(data_channel_id))
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_CHANNEL_WORD_OFFSET (1)
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_CHANNEL_GET(ipc_msg_data_channel_activate_req_ptr)  \
    (((uint32)((ipc_msg_data_channel_activate_req_ptr)->_data[1]) | \
      ((uint32)((ipc_msg_data_channel_activate_req_ptr)->_data[2]) << 16)))
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_CHANNEL_SET(ipc_msg_data_channel_activate_req_ptr, channel) do { \
        (ipc_msg_data_channel_activate_req_ptr)->_data[1] = (uint16)((channel) & 0xffff); \
        (ipc_msg_data_channel_activate_req_ptr)->_data[2] = (uint16)((channel) >> 16); } while (0)
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_WORD_SIZE (3)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_CREATE(data_channel_id, channel) \
    (uint16)(data_channel_id), \
    (uint16)((channel) & 0xffff), \
    (uint16)((channel) >> 16)
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_REQ_PACK(ipc_msg_data_channel_activate_req_ptr, data_channel_id, channel) \
    do { \
        (ipc_msg_data_channel_activate_req_ptr)->_data[0] = (uint16)((uint16)(data_channel_id)); \
        (ipc_msg_data_channel_activate_req_ptr)->_data[1] = (uint16)((uint16)((channel) & 0xffff)); \
        (ipc_msg_data_channel_activate_req_ptr)->_data[2] = (uint16)(((channel) >> 16)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_DATA_CHANNEL_ACTIVATE_RES

  DESCRIPTION

  MEMBERS
    status          - status code
    data_channel_id - Data channel ID

*******************************************************************************/
typedef struct
{
    uint16 _data[2];
} IPC_MSG_DATA_CHANNEL_ACTIVATE_RES;

/* The following macros take IPC_MSG_DATA_CHANNEL_ACTIVATE_RES *ipc_msg_data_channel_activate_res_ptr */
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_STATUS_WORD_OFFSET (0)
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_STATUS_GET(ipc_msg_data_channel_activate_res_ptr) ((ipc_msg_data_channel_activate_res_ptr)->_data[0])
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_STATUS_SET(ipc_msg_data_channel_activate_res_ptr, status) ((ipc_msg_data_channel_activate_res_ptr)->_data[0] = (uint16)(status))
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_DATA_CHANNEL_ID_WORD_OFFSET (1)
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_DATA_CHANNEL_ID_GET(ipc_msg_data_channel_activate_res_ptr) ((ipc_msg_data_channel_activate_res_ptr)->_data[1])
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_DATA_CHANNEL_ID_SET(ipc_msg_data_channel_activate_res_ptr, data_channel_id) ((ipc_msg_data_channel_activate_res_ptr)->_data[1] = (uint16)(data_channel_id))
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_WORD_SIZE (2)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_CREATE(status, data_channel_id) \
    (uint16)(status), \
    (uint16)(data_channel_id)
#define IPC_MSG_DATA_CHANNEL_ACTIVATE_RES_PACK(ipc_msg_data_channel_activate_res_ptr, status, data_channel_id) \
    do { \
        (ipc_msg_data_channel_activate_res_ptr)->_data[0] = (uint16)((uint16)(status)); \
        (ipc_msg_data_channel_activate_res_ptr)->_data[1] = (uint16)((uint16)(data_channel_id)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ

  DESCRIPTION

  MEMBERS
    data_channel_id - Data channel ID

*******************************************************************************/
typedef struct
{
    uint16 _data[1];
} IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ;

/* The following macros take IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ *ipc_msg_data_channel_deactivate_req_ptr */
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ_DATA_CHANNEL_ID_WORD_OFFSET (0)
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ_DATA_CHANNEL_ID_GET(ipc_msg_data_channel_deactivate_req_ptr) ((ipc_msg_data_channel_deactivate_req_ptr)->_data[0])
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ_DATA_CHANNEL_ID_SET(ipc_msg_data_channel_deactivate_req_ptr, data_channel_id) ((ipc_msg_data_channel_deactivate_req_ptr)->_data[0] = (uint16)(data_channel_id))
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ_WORD_SIZE (1)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ_CREATE(data_channel_id) \
    (uint16)(data_channel_id)
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_REQ_PACK(ipc_msg_data_channel_deactivate_req_ptr, data_channel_id) \
    do { \
        (ipc_msg_data_channel_deactivate_req_ptr)->_data[0] = (uint16)((uint16)(data_channel_id)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES

  DESCRIPTION

  MEMBERS
    status          - status code
    data_channel_id - Data channel ID

*******************************************************************************/
typedef struct
{
    uint16 _data[2];
} IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES;

/* The following macros take IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES *ipc_msg_data_channel_deactivate_res_ptr */
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_STATUS_WORD_OFFSET (0)
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_STATUS_GET(ipc_msg_data_channel_deactivate_res_ptr) ((ipc_msg_data_channel_deactivate_res_ptr)->_data[0])
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_STATUS_SET(ipc_msg_data_channel_deactivate_res_ptr, status) ((ipc_msg_data_channel_deactivate_res_ptr)->_data[0] = (uint16)(status))
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_DATA_CHANNEL_ID_WORD_OFFSET (1)
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_DATA_CHANNEL_ID_GET(ipc_msg_data_channel_deactivate_res_ptr) ((ipc_msg_data_channel_deactivate_res_ptr)->_data[1])
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_DATA_CHANNEL_ID_SET(ipc_msg_data_channel_deactivate_res_ptr, data_channel_id) ((ipc_msg_data_channel_deactivate_res_ptr)->_data[1] = (uint16)(data_channel_id))
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_WORD_SIZE (2)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_CREATE(status, data_channel_id) \
    (uint16)(status), \
    (uint16)(data_channel_id)
#define IPC_MSG_DATA_CHANNEL_DEACTIVATE_RES_PACK(ipc_msg_data_channel_deactivate_res_ptr, status, data_channel_id) \
    do { \
        (ipc_msg_data_channel_deactivate_res_ptr)->_data[0] = (uint16)((uint16)(status)); \
        (ipc_msg_data_channel_deactivate_res_ptr)->_data[1] = (uint16)((uint16)(data_channel_id)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_IPC_ERROR_IND

  DESCRIPTION

  MEMBERS
    error_type  - Error type
    error_level - Error level
    error_code  - Error code

*******************************************************************************/
typedef struct
{
    uint16 _data[3];
} IPC_MSG_IPC_ERROR_IND;

/* The following macros take IPC_MSG_IPC_ERROR_IND *ipc_msg_ipc_error_ind_ptr */
#define IPC_MSG_IPC_ERROR_IND_ERROR_TYPE_WORD_OFFSET (0)
#define IPC_MSG_IPC_ERROR_IND_ERROR_TYPE_GET(ipc_msg_ipc_error_ind_ptr) ((ipc_msg_ipc_error_ind_ptr)->_data[0])
#define IPC_MSG_IPC_ERROR_IND_ERROR_TYPE_SET(ipc_msg_ipc_error_ind_ptr, error_type) ((ipc_msg_ipc_error_ind_ptr)->_data[0] = (uint16)(error_type))
#define IPC_MSG_IPC_ERROR_IND_ERROR_LEVEL_WORD_OFFSET (1)
#define IPC_MSG_IPC_ERROR_IND_ERROR_LEVEL_GET(ipc_msg_ipc_error_ind_ptr) ((ipc_msg_ipc_error_ind_ptr)->_data[1])
#define IPC_MSG_IPC_ERROR_IND_ERROR_LEVEL_SET(ipc_msg_ipc_error_ind_ptr, error_level) ((ipc_msg_ipc_error_ind_ptr)->_data[1] = (uint16)(error_level))
#define IPC_MSG_IPC_ERROR_IND_ERROR_CODE_WORD_OFFSET (2)
#define IPC_MSG_IPC_ERROR_IND_ERROR_CODE_GET(ipc_msg_ipc_error_ind_ptr) ((ipc_msg_ipc_error_ind_ptr)->_data[2])
#define IPC_MSG_IPC_ERROR_IND_ERROR_CODE_SET(ipc_msg_ipc_error_ind_ptr, error_code) ((ipc_msg_ipc_error_ind_ptr)->_data[2] = (uint16)(error_code))
#define IPC_MSG_IPC_ERROR_IND_WORD_SIZE (3)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_IPC_ERROR_IND_CREATE(error_type, error_level, error_code) \
    (uint16)(error_type), \
    (uint16)(error_level), \
    (uint16)(error_code)
#define IPC_MSG_IPC_ERROR_IND_PACK(ipc_msg_ipc_error_ind_ptr, error_type, error_level, error_code) \
    do { \
        (ipc_msg_ipc_error_ind_ptr)->_data[0] = (uint16)((uint16)(error_type)); \
        (ipc_msg_ipc_error_ind_ptr)->_data[1] = (uint16)((uint16)(error_level)); \
        (ipc_msg_ipc_error_ind_ptr)->_data[2] = (uint16)((uint16)(error_code)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_PX_SETUP_READY_IND

  DESCRIPTION

  MEMBERS
    status - status code

*******************************************************************************/
typedef struct
{
    uint16 _data[1];
} IPC_MSG_PX_SETUP_READY_IND;

/* The following macros take IPC_MSG_PX_SETUP_READY_IND *ipc_msg_px_setup_ready_ind_ptr */
#define IPC_MSG_PX_SETUP_READY_IND_STATUS_WORD_OFFSET (0)
#define IPC_MSG_PX_SETUP_READY_IND_STATUS_GET(ipc_msg_px_setup_ready_ind_ptr) ((ipc_msg_px_setup_ready_ind_ptr)->_data[0])
#define IPC_MSG_PX_SETUP_READY_IND_STATUS_SET(ipc_msg_px_setup_ready_ind_ptr, status) ((ipc_msg_px_setup_ready_ind_ptr)->_data[0] = (uint16)(status))
#define IPC_MSG_PX_SETUP_READY_IND_WORD_SIZE (1)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_PX_SETUP_READY_IND_CREATE(status) \
    (uint16)(status)
#define IPC_MSG_PX_SETUP_READY_IND_PACK(ipc_msg_px_setup_ready_ind_ptr, status) \
    do { \
        (ipc_msg_px_setup_ready_ind_ptr)->_data[0] = (uint16)((uint16)(status)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_RESET_IPC_REQ

  DESCRIPTION

  MEMBERS
    message_channel_id - Message Channel ID as returned by ipc_setup_comms

*******************************************************************************/
typedef struct
{
    uint16 _data[1];
} IPC_MSG_RESET_IPC_REQ;

/* The following macros take IPC_MSG_RESET_IPC_REQ *ipc_msg_reset_ipc_req_ptr */
#define IPC_MSG_RESET_IPC_REQ_MESSAGE_CHANNEL_ID_WORD_OFFSET (0)
#define IPC_MSG_RESET_IPC_REQ_MESSAGE_CHANNEL_ID_GET(ipc_msg_reset_ipc_req_ptr) ((ipc_msg_reset_ipc_req_ptr)->_data[0])
#define IPC_MSG_RESET_IPC_REQ_MESSAGE_CHANNEL_ID_SET(ipc_msg_reset_ipc_req_ptr, message_channel_id) ((ipc_msg_reset_ipc_req_ptr)->_data[0] = (uint16)(message_channel_id))
#define IPC_MSG_RESET_IPC_REQ_WORD_SIZE (1)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_RESET_IPC_REQ_CREATE(message_channel_id) \
    (uint16)(message_channel_id)
#define IPC_MSG_RESET_IPC_REQ_PACK(ipc_msg_reset_ipc_req_ptr, message_channel_id) \
    do { \
        (ipc_msg_reset_ipc_req_ptr)->_data[0] = (uint16)((uint16)(message_channel_id)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_RESET_IPC_RES

  DESCRIPTION

  MEMBERS
    status             - status code
    message_channel_id - Message Channel ID as returned by ipc_setup_comms

*******************************************************************************/
typedef struct
{
    uint16 _data[2];
} IPC_MSG_RESET_IPC_RES;

/* The following macros take IPC_MSG_RESET_IPC_RES *ipc_msg_reset_ipc_res_ptr */
#define IPC_MSG_RESET_IPC_RES_STATUS_WORD_OFFSET (0)
#define IPC_MSG_RESET_IPC_RES_STATUS_GET(ipc_msg_reset_ipc_res_ptr) ((ipc_msg_reset_ipc_res_ptr)->_data[0])
#define IPC_MSG_RESET_IPC_RES_STATUS_SET(ipc_msg_reset_ipc_res_ptr, status) ((ipc_msg_reset_ipc_res_ptr)->_data[0] = (uint16)(status))
#define IPC_MSG_RESET_IPC_RES_MESSAGE_CHANNEL_ID_WORD_OFFSET (1)
#define IPC_MSG_RESET_IPC_RES_MESSAGE_CHANNEL_ID_GET(ipc_msg_reset_ipc_res_ptr) ((ipc_msg_reset_ipc_res_ptr)->_data[1])
#define IPC_MSG_RESET_IPC_RES_MESSAGE_CHANNEL_ID_SET(ipc_msg_reset_ipc_res_ptr, message_channel_id) ((ipc_msg_reset_ipc_res_ptr)->_data[1] = (uint16)(message_channel_id))
#define IPC_MSG_RESET_IPC_RES_WORD_SIZE (2)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_RESET_IPC_RES_CREATE(status, message_channel_id) \
    (uint16)(status), \
    (uint16)(message_channel_id)
#define IPC_MSG_RESET_IPC_RES_PACK(ipc_msg_reset_ipc_res_ptr, status, message_channel_id) \
    do { \
        (ipc_msg_reset_ipc_res_ptr)->_data[0] = (uint16)((uint16)(status)); \
        (ipc_msg_reset_ipc_res_ptr)->_data[1] = (uint16)((uint16)(message_channel_id)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_TEARDOWN_IPC_REQ

  DESCRIPTION

  MEMBERS
    message_channel_id - Message Channel ID as returned by ipc_setup_comms

*******************************************************************************/
typedef struct
{
    uint16 _data[1];
} IPC_MSG_TEARDOWN_IPC_REQ;

/* The following macros take IPC_MSG_TEARDOWN_IPC_REQ *ipc_msg_teardown_ipc_req_ptr */
#define IPC_MSG_TEARDOWN_IPC_REQ_MESSAGE_CHANNEL_ID_WORD_OFFSET (0)
#define IPC_MSG_TEARDOWN_IPC_REQ_MESSAGE_CHANNEL_ID_GET(ipc_msg_teardown_ipc_req_ptr) ((ipc_msg_teardown_ipc_req_ptr)->_data[0])
#define IPC_MSG_TEARDOWN_IPC_REQ_MESSAGE_CHANNEL_ID_SET(ipc_msg_teardown_ipc_req_ptr, message_channel_id) ((ipc_msg_teardown_ipc_req_ptr)->_data[0] = (uint16)(message_channel_id))
#define IPC_MSG_TEARDOWN_IPC_REQ_WORD_SIZE (1)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_TEARDOWN_IPC_REQ_CREATE(message_channel_id) \
    (uint16)(message_channel_id)
#define IPC_MSG_TEARDOWN_IPC_REQ_PACK(ipc_msg_teardown_ipc_req_ptr, message_channel_id) \
    do { \
        (ipc_msg_teardown_ipc_req_ptr)->_data[0] = (uint16)((uint16)(message_channel_id)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_TEARDOWN_IPC_RES

  DESCRIPTION

  MEMBERS
    status             - status code
    message_channel_id - Message Channel ID as returned by ipc_setup_comms

*******************************************************************************/
typedef struct
{
    uint16 _data[2];
} IPC_MSG_TEARDOWN_IPC_RES;

/* The following macros take IPC_MSG_TEARDOWN_IPC_RES *ipc_msg_teardown_ipc_res_ptr */
#define IPC_MSG_TEARDOWN_IPC_RES_STATUS_WORD_OFFSET (0)
#define IPC_MSG_TEARDOWN_IPC_RES_STATUS_GET(ipc_msg_teardown_ipc_res_ptr) ((ipc_msg_teardown_ipc_res_ptr)->_data[0])
#define IPC_MSG_TEARDOWN_IPC_RES_STATUS_SET(ipc_msg_teardown_ipc_res_ptr, status) ((ipc_msg_teardown_ipc_res_ptr)->_data[0] = (uint16)(status))
#define IPC_MSG_TEARDOWN_IPC_RES_MESSAGE_CHANNEL_ID_WORD_OFFSET (1)
#define IPC_MSG_TEARDOWN_IPC_RES_MESSAGE_CHANNEL_ID_GET(ipc_msg_teardown_ipc_res_ptr) ((ipc_msg_teardown_ipc_res_ptr)->_data[1])
#define IPC_MSG_TEARDOWN_IPC_RES_MESSAGE_CHANNEL_ID_SET(ipc_msg_teardown_ipc_res_ptr, message_channel_id) ((ipc_msg_teardown_ipc_res_ptr)->_data[1] = (uint16)(message_channel_id))
#define IPC_MSG_TEARDOWN_IPC_RES_WORD_SIZE (2)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_TEARDOWN_IPC_RES_CREATE(status, message_channel_id) \
    (uint16)(status), \
    (uint16)(message_channel_id)
#define IPC_MSG_TEARDOWN_IPC_RES_PACK(ipc_msg_teardown_ipc_res_ptr, status, message_channel_id) \
    do { \
        (ipc_msg_teardown_ipc_res_ptr)->_data[0] = (uint16)((uint16)(status)); \
        (ipc_msg_teardown_ipc_res_ptr)->_data[1] = (uint16)((uint16)(message_channel_id)); \
    } while (0)


/*******************************************************************************

  NAME
    IPC_MSG_WATCHDOG_PING_RES

  DESCRIPTION

  MEMBERS
    status - status code

*******************************************************************************/
typedef struct
{
    uint16 _data[1];
} IPC_MSG_WATCHDOG_PING_RES;

/* The following macros take IPC_MSG_WATCHDOG_PING_RES *ipc_msg_watchdog_ping_res_ptr */
#define IPC_MSG_WATCHDOG_PING_RES_STATUS_WORD_OFFSET (0)
#define IPC_MSG_WATCHDOG_PING_RES_STATUS_GET(ipc_msg_watchdog_ping_res_ptr) ((ipc_msg_watchdog_ping_res_ptr)->_data[0])
#define IPC_MSG_WATCHDOG_PING_RES_STATUS_SET(ipc_msg_watchdog_ping_res_ptr, status) ((ipc_msg_watchdog_ping_res_ptr)->_data[0] = (uint16)(status))
#define IPC_MSG_WATCHDOG_PING_RES_WORD_SIZE (1)
/*lint -e(773) allow unparenthesized*/
#define IPC_MSG_WATCHDOG_PING_RES_CREATE(status) \
    (uint16)(status)
#define IPC_MSG_WATCHDOG_PING_RES_PACK(ipc_msg_watchdog_ping_res_ptr, status) \
    do { \
        (ipc_msg_watchdog_ping_res_ptr)->_data[0] = (uint16)((uint16)(status)); \
    } while (0)


#endif /* IPC_MSG_PRIM_H */

