/* Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd. */
/*  */

/*!
@file    gatt_csis_server_private.h
@brief  
*/

#ifndef GATT_CSIS_SERVER_PRIVATE_H
#define GATT_CSIS_SERVER_PRIVATE_H

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>
#include <bdaddr.h>
#include <connection.h>

#include <gatt.h>
#include <gatt_manager.h>

#include "gatt_csis_server.h"
#include "gatt_csis_server_db.h"
#include "gatt_csis_server_debug.h"

/* Macros for creating messages */
#define MAKE_CSIS_SERVER_MESSAGE(TYPE) TYPE##_T* message = (TYPE##_T *)PanicNull(calloc(1,sizeof(TYPE##_T)))

/*To be used for WORD order values*/
#define MAKE_CSIS_SERVER_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicNull(calloc(1,sizeof(TYPE##_T) + LEN))

#define GATT_CSIS_SERVER_CCC_NOTIFY                   (0x01)
#define GATT_CSIS_SERVER_CCC_INDICATE                 (0x02)

#define GATT_CSIS_CLIENT_CONFIG_MASK (GATT_CSIS_SERVER_CCC_NOTIFY | GATT_CSIS_SERVER_CCC_NOTIFY)
#define GET_CSIS_CLIENT_CONFIG(config)          (config & GATT_CSIS_CLIENT_CONFIG_MASK )

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_CSIS_SERVER_CCC_VALUE_SIZE           (sizeof(uint8) * 2)

/* Size of the SIRK characteristic (number of octets)*/
#define GATT_CSIS_SERVER_SIRK_SIZE                (sizeof(uint8) * 16)

#define GATT_CSIS_SERVER_SIRK_SIZE_PLUS_TYPE_SIZE  GATT_CSIS_SERVER_SIRK_SIZE + (sizeof(uint8) * 1)

/* Size of the Lock characteristic (number of octets)*/
#define GATT_CSIS_SERVER_LOCK_SIZE                (sizeof(uint8) * 1)

/* Size of the SIZE characteristic (number of octets)*/
#define GATT_CSIS_SERVER_SIZE_CHARACTERISTCIS_SIZE        (sizeof(uint8) * 1)

/* Size of the RANK characteristic (number of octets)*/
#define GATT_CSIS_SERVER_RANK_CHARACTERISTCIS_SIZE        (sizeof(uint8) * 1)

/* Default Lock expiry timeout(in seconds) from spec */
#define GATT_CSIS_SERVER_LOCK_EXPIRY_TIMEOUT_DEFAULT_VALUE    60

/* Length of RSI */
#define GATT_CSIS_RSI_DATA_LENGTH          (sizeof(uint8) * 6)

/* Application error codes as per spec */
#define GATT_CSIS_LOCK_DENIED                        (0x80)
#define GATT_CSIS_LOCK_RELEASE_NOT_ALLLOWED          (0x81)
#define GATT_CSIS_INVALID_LOCK_VALUE                 (0x82)
#define GATT_CSIS_OOB_SIRK_ONLY                      (0x83)
#define GATT_CSIS_LOCK_ALREADY_GRANTED               (0x84)

#define LOCK_TIMER_UPDATE_REQUIRED                   0x01
#define LOCK_TIMER_UPDATE_NOT_REQUIRED               0x00

#define GATT_CSIS_ENCRYPTED_SIRK                     0x00
#define GATT_CSIS_PLAIN_TEXT_SIRK                    0x01

#define GATT_CSIS_SERVER_NONE                       (0x00)
#define GATT_CSIS_SERVER_READ_ACCESS                (0x01)
#define GATT_CSIS_SERVER_NOTIFY                     (0x02)

/* Enum For LIB internal messages */
typedef enum
{
    CSIS_SERVER_INTERNAL_MSG_BASE = 0,
    CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER,

    /* End message limit */
    CSIS_SERVER_INTERNAL_MSG_TOP                  /* Top of message */
}csis_client_internal_msg_t;

typedef struct
{
    uint16                cid;
    GattCsisServerConfigType  client_cfg;
} csis_client_data;


/*! @brief Definition of data required for association.
 */
typedef struct
{
    uint8                rank;
    uint8                cs_size;
    uint8                sirk[SIZE_SIRK_KEY];
    uint8                sirkOp;
    csis_client_data     connected_clients[GATT_CSIS_MAX_CONNECTIONS];
} csis_data;

/*! @brief The CSIS server internal structure for the server role.

    This structure is NOT visible to the application
*/
typedef struct _gatt_csis_server_t
{
    TaskData lib_task;
    Task app_task;

    /* Service handle provided by the service_handle lib when the server
     * memory instance is created
     */
    ServiceHandle srvc_hndl;

    /*! Information to be provided in service characteristics. */
    csis_data data;
} GCSISS_T;

/* Internal Message Structure to for firing Lock timer */
typedef struct
{
    connection_id_t cid;
} CSIS_SERVER_INTERNAL_MSG_LOCK_TIMEOUT_TIMER_T;

#endif /* GATT_CSIS_SERVER_PRIVATE_H */
