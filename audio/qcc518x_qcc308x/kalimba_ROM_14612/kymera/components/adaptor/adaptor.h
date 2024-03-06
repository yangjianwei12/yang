/****************************************************************************
 * Copyright (c) 2013 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup adaptor Adaptors to kymera interface
 *
 * Interface to exchange messages between the primary processor of the audio
 * subsystem and the rest of the system. This is in contrast to sending
 * messages to tasks managed by the scheduler ("sched_oxygen").
 *
 * The submodules are called adaptors because the same high level message
 * has to be adapted or serialized to various protocols.
 */
/**
 * \file adaptor.h
 * \ingroup adaptor
 *
 * Public definitions of functions to exchange messages between the primary
 * processor and the rest of the system.
 */

#ifndef _ADAPTOR_H_
#define _ADAPTOR_H_

#include "types.h"
#include "adaptor/connection_id.h"
#if defined(SUPPORTS_MULTI_CORE)
#include "kip_msg_prim.h"
#endif

/****************************************************************************
Public Constant Declarations
*/

#define ADAPTOR_ANY_SIZE 1

/****************************************************************************
Public Type Definitions
*/

/**
 * Adapter Message IDs
 */
typedef enum
{
    AMSGID_FROM_OPERATOR        = 0x00000,
    AMSGID_FROM_FRAMEWORK       = 0x00001,
    AMSGID_PS_ENTRY_READ        = 0x00002,
    AMSGID_PS_ENTRY_WRITE       = 0x00003,
    AMSGID_PS_ENTRY_DELETE      = 0x00004,
    AMSGID_PS_SHUTDOWN_COMPLETE = 0x00005,
    AMSGID_FILE_DETAILS_QUERY   = 0x00006,
    AMSGID_KIP_START            = 0x10000,
    AMSGID_KIP_END              = 0x1ffff
} ADAPTOR_MSGID;

typedef struct
{
    uint16f ext_op_id;
    uint16f client_id;
    uint16f msg_id;
    uint16f length;
    uint16f message[ADAPTOR_ANY_SIZE];
} ADAPTOR_FROM_OPERATOR_MSG;
#define ADAPTOR_FROM_OPERATOR_MSG_MESSAGE_WORD_OFFSET (4)
#define ADAPTOR_FROM_OPERATOR_MSG_WORD_SIZE (5)

typedef struct
{
    uint16f key;
    uint16f message[ADAPTOR_ANY_SIZE];
} ADAPTOR_FROM_FRAMEWORK_MSG;
#define ADAPTOR_FROM_FRAMEWORK_MSG_MESSAGE_WORD_OFFSET (1)
#define ADAPTOR_FROM_FRAMEWORK_MSG_WORD_SIZE (2)

typedef struct
{
    uint16f key_id;
    uint16f offset;
} ADAPTOR_PS_ENTRY_READ_MSG;
#define ADAPTOR_PS_ENTRY_READ_MSG_WORD_SIZE (2)

typedef struct
{
    uint16f key_id;
    uint16f total_len;
    uint16f offset;
    uint16f data[ADAPTOR_ANY_SIZE];
} ADAPTOR_PS_ENTRY_WRITE_MSG;
#define ADAPTOR_PS_ENTRY_WRITE_MSG_DATA_WORD_OFFSET (3)
#define ADAPTOR_PS_ENTRY_WRITE_MSG_WORD_SIZE (4)

typedef struct
{
    uint16f key_id;
} ADAPTOR_PS_ENTRY_DELETE_MSG;
#define ADAPTOR_PS_ENTRY_DELETE_MSG_WORD_SIZE (1)

#define ADAPTOR_PS_SHUTDOWN_COMPLETE_MSG_WORD_SIZE (0)

typedef struct
{
    uint16f filename[ADAPTOR_ANY_SIZE]; /*!< Packed file name: 2 characters per word. */
} ADAPTOR_FILE_DETAILS_QUERY_MSG;
#define ADAPTOR_FILE_DETAILS_QUERY_MSG_DATA_WORD_OFFSET (0)
#define ADAPTOR_FILE_DETAILS_QUERY_MSG_WORD_SIZE (1)

/* The GNU compiler is better at dealing with types than the Kalimba
   compiler. So use the former to prevent the direct use of weak types
   which has been the source of several bugs and protocol violations. */
#ifdef __GNUC__
typedef union
{
    ADAPTOR_FROM_OPERATOR_MSG                     *adaptor00;
    ADAPTOR_FROM_FRAMEWORK_MSG                    *adaptor01;
    ADAPTOR_PS_ENTRY_READ_MSG                     *adaptor02;
    ADAPTOR_PS_ENTRY_WRITE_MSG                    *adaptor03;
    ADAPTOR_PS_ENTRY_DELETE_MSG                   *adaptor04;
    ADAPTOR_FILE_DETAILS_QUERY_MSG                *adaptor05;
#if defined(SUPPORTS_MULTI_CORE)
    KIP_MSG_REQ_STRUC                             *kip00;
    KIP_MSG_MESSAGE_FROM_OPERATOR_REQ             *kip01;
    KIP_MSG_PS_WRITE_REQ                          *kip02;
    KIP_MSG_PS_SHUTDOWN_REQ                       *kip03;
    KIP_MSG_PS_SHUTDOWN_COMPLETE_REQ              *kip04;
    KIP_MSG_HANDLE_EOF_REQ                        *kip05;
    KIP_MSG_PUBLISH_FAULT_REQ                     *kip06;
    KIP_MSG_FILE_MGR_RELEASE_FILE_REQ             *kip07;
    KIP_MSG_PS_READ_RES                           *kip08;
    KIP_MSG_PS_WRITE_RES                          *kip09;
    KIP_MSG_OPERATOR_MESSAGE_REQ                  *kip10;
    KIP_MSG_OPLIST_CMD_REQ                        *kip11;
    KIP_MSG_CREATE_OPERATOR_REQ                   *kip12;
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_REQ        *kip13;
    KIP_MSG_METADATA_CHANNEL_ACTIVATED_RES        *kip14;
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_RES       *kip15;
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_RES       *kip16;
    KIP_MSG_STREAM_CONNECT_REQ                    *kip17;
    KIP_MSG_STREAM_CONNECT_RES                    *kip18;
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_RES          *kip19;
    KIP_MSG_TRANSFORM_LIST_REMOVE_ENTRY_REQ       *kip20;
    KIP_MSG_STREAM_DESTROY_ENDPOINTS_REQ          *kip21;
    KIP_MSG_STREAM_TRANSFORM_DISCONNECT_REQ       *kip22;
    KIP_MSG_STREAM_CREATE_ENDPOINTS_REQ           *kip23;
    KIP_MSG_STREAM_CREATE_ENDPOINTS_RES           *kip24;
    KIP_MSG_CAP_DOWNLOAD_KCS_CALLBACK_REQ         *kip25;
    KIP_MSG_CAP_DOWNLOAD_REQ_DATA_RES             *kip26;
    KIP_MSG_CAP_DOWNLOAD_START_DOWNLOAD_REQ       *kip27;
    KIP_MSG_CAP_DOWNLOAD_REQ_DATA_REQ             *kip28;
    KIP_MSG_CAP_DOWNLOAD_REMOVE_KCS_REQ           *kip29;
    KIP_MSG_CAP_DOWNLOAD_REMOVE_KCS_RES           *kip30;
    KIP_MSG_CAP_DOWNLOAD_ADD_MAPPED_RECORD_REQ    *kip31;
    KIP_MSG_CAP_DOWNLOAD_ADD_ELF_REQ              *kip32;
    KIP_MSG_CAP_DOWNLOAD_REMOVE_ELF_REQ           *kip33;
#endif /* defined(SUPPORTS_MULTI_CORE) */
} ADAPTOR_DATA;
#define ADAPTOR_NULL ((ADAPTOR_DATA)(ADAPTOR_FROM_OPERATOR_MSG*) NULL)
#else
typedef uint16f* ADAPTOR_DATA;
#define ADAPTOR_NULL NULL
#endif

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief  Returns the type of adaptor a connection link is
 *         associated with.
 *
 * \param  conn_id the connection ID in forward direction
 * 
 * \return the type of adaptor.
 */
extern ADAPTOR_TARGET adaptor_get_target(CONNECTION_LINK conn_id);

/**
 * \brief  Send message function.
 *
 * \param  conn_id the connection ID (sender and recipient ID codes)
 * \param  msg_id ID of the message
 * \param  msg_length length of payload
 * \param  msg_data pointer to the message payload
 *
 * \note   There can be at most 16 bits of useful data in each word
 *         of the array pointed by msg_data. KIP uses uint16 while
 *         other adaptors use uint16f.
 * 
 * \return success/fail (true/false).
 */
extern bool adaptor_send_message(CONNECTION_LINK conn_id, ADAPTOR_MSGID msg_id,
                                 unsigned msg_length, ADAPTOR_DATA msg_data);

/**
 * \brief Unpack a list of parameters in a message received over 16-bit wide
 *        transport for use in a platform independent module.
 *
 * \param pparam Pointer to the start of the parameter list within the message.
 * \param count  Number of elements in the list.
 *
 * \return Unpacked list, which needs to be freed after use
 *         or NULL if count is 0.
 */
extern uint16f *adaptor_unpack_list_to_unsigned(const uint16 *pparam,
                                                unsigned count);

/**
 * \brief Converts an unsigned array into uint16 array.
 *        Allocates the destination uint16 array if required.
 *
 * \param ppdu Pointer to the destination uint16 array.
 *             Can be NULL in which case, the destination array
 *             is allocated on a non-zero count.
 * \param psrc pointer to the source unsigned array.
 * \param count number of elements in the source list.
 *
 * \return pointer to the destination uint16 array, if allocated.
 */
extern uint16 *adaptor_pack_list_to_uint16(uint16 *ppdu,
                                           const uint16f *psrc,
                                           unsigned count);

/**
 * \brief Check if a pointer is owned by an adaptor.
 *
 * \param context Pointer to be checked.
 *
 * \return TRUE if an adaptor owns the pointer.
 *
 * \note This function is useful to API that callback
 *       to functions for tasks belonging to different
 *       components and for which the pointer might
 *       become invalid before the function is called
 *       back.
 */
extern bool adaptor_does_context_exist(void *context);

#endif /* _ADAPTOR_H_ */
