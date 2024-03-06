/* Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_PACS_SERVER_PRIVATE_H
#define GATT_PACS_SERVER_PRIVATE_H

#include <csrtypes.h>
#include <message.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>

#include <gatt.h>
#include <gatt_manager.h>

#include "gatt_pacs_server.h"
#include "gatt_pacs_server_db.h"
#include "gatt_pacs_server_debug.h"

/* Macros for creating messages */
#define MAKE_PACS_SERVER_MESSAGE(TYPE) TYPE##_T* message = (TYPE##_T *)PanicNull(calloc(1,sizeof(TYPE##_T)))

/*To be used for WORD order values*/
#define MAKE_PACS_SERVER_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicNull(calloc(1,sizeof(TYPE##_T) + LEN))

#define GATT_PACS_SERVER_CCC_NOTIFY                   (0x01)
#define GATT_PACS_SERVER_CCC_INDICATE                 (0x02)

#define GATT_PACS_CLIENT_CONFIG_MASK (GATT_PACS_SERVER_CCC_NOTIFY | GATT_PACS_SERVER_CCC_INDICATE)
#define GET_PACS_CLIENT_CONFIG(config)          (config & GATT_PACS_CLIENT_CONFIG_MASK )

/* Required octets for values sent to Client Configuration Descriptor */
#define GATT_PACS_SERVER_CCC_VALUE_SIZE           (sizeof(uint8) * 2)

#define GATT_PACS_AUDIO_CONTEXTS_VALUE_SIZE       (sizeof(uint8) * 4)
#define GATT_PACS_AUDIO_LOCATION_VALUE_SIZE       (sizeof(uint8) * 4)

/* Max PAC records supported */
#define MAX_PAC_RECORD_HANDLES          0x000F
#define DEFAULT_PAC_RECORD_HANDLE_MASK  0x8000

#define PACS_PREFERRED_AUDIO_CONTEXTS_TYPE  0x1
#define PACS_VENDOR_SPECIFIC_METATDATA_TYPE   0xFF

typedef struct
{
    ConnectionIdType                cid;
    GattPacsServerConfigType      client_cfg;
} pacs_client_data;

typedef struct pac_record_list
{
   const GattPacsServerRecordType *pac_record;
   uint16                          pac_record_handle;
   bool                            consumed;
   struct pac_record_list          *next;
} pac_record_list;

/*! @brief Definition of data required for association.
 */
typedef struct
{
    pac_record_list                *sink_pack_record;
    pac_record_list                *source_pack_record;
    pac_record_list                *vs_sink_pack_record;
    pac_record_list                *vs_source_pack_record;
    uint32                         sink_audio_source_location;
    uint32                         source_audio_source_location;
    uint32                         supported_audio_contexts;
    uint32                         available_audio_contexts;
    uint16                         pacs_record_handle_mask;
    pacs_client_data               connected_clients[GATT_PACS_MAX_CONNECTIONS];
} pacs_data;

/*! @brief The PACS server internal structure for the server role.

    This structure is NOT visible to the application
*/
typedef struct _gatt_pacs_server_t
{
    TaskData lib_task;
    Task app_task;

    /* Service handle provided by the service_handle lib when the server
     * memory instance is created
     */
    ServiceHandle srvc_hndl;

    /*! Information to be provided in service characteristics. */
    pacs_data data;
} GPACSS_T;

#endif /* GATT_PACS_SERVER_PRIVATE_H */

