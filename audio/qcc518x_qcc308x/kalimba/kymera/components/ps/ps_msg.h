/****************************************************************************
 * Copyright (c) 2016 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file ps_msg.h
 * \ingroup ps
 *
 * Persistent Storage private API between "ps" and "ps_msg".
 */

#ifndef PS_MSG_H
#define PS_MSG_H

#include "ps/ps_common.h"
#include "ps/ps_for_adaptors.h"

typedef enum
{
    PS_MSG_NONE             = 0x0000,
    /* Message name & ID for the shutdown notification that comes from
       the application layer. */
    PS_MSG_SHUTDOWN_REQ     = 0x0001,
    PS_MSG_READ_RESP        = 0x0002,
    PS_MSG_WRITE_RESP       = 0x0003,
    PS_MSG_REGISTER_REQ     = 0x0004,
    /* Message IDs used in messages to the platform-specific PS tasks. */
    PS_MSG_WRITE_REQ        = 0x4000,
    PS_MSG_READ_REQ         = 0x4001,
    PS_MSG_ENTRY_DELETE_REQ = 0x4002,
    PS_MSG_DELETE_REQ       = 0x4003,
    /* Ensures that the type will use at least 16 bits. */
    PS_MSG_INVALID          = 0xFFFF,
} PS_MSG_TYPE;

typedef union
{
    PS_READ_CALLBACK         read;
    PS_WRITE_CALLBACK        write;
    PS_ENTRY_DELETE_CALLBACK entry_del;
    PS_DELETE_CALLBACK       del;
    PS_SHUTDOWN_CALLBACK     shutdown;
} PS_CALLBACK;

/** Structure of message payload sent to the platform-specific PS message handler. */
/* TODO: PS_MSG_DATA should be split in different messages. */
typedef struct
{
    /** callback function pointer - for responding */
    PS_CALLBACK callback;

    /** The PS key value */
    PS_KEY_TYPE key;

    /** PS rank */
    PERSISTENCE_RANK rank;

    /** ACCMD: Indicate whether the request was successful */
    bool success;

    /** ACCMD: Size of value associated with a key. */
    uint16 total_length;

    /** Length of data vehiculated for the PS key */
    uint16 data_length;

    /** Placeholder for data for the PS key */
    uint16 data[];
} PS_MSG_DATA;

#endif /* PS_MSG_H */

