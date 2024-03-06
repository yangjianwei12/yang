/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_MICS_SERVER_PRIVATE_H
#define GATT_MICS_SERVER_PRIVATE_H

#include "gatt_mics_server.h"
#include "gatt_mics_server_debug.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include <marshal.h>
#endif

/*! To save space client Client configs are stored as 2 bits only, this macro
 * ensures the client config only takes 2 bits */
#define MICS_CCC_MASK(ccc)     (ccc & (0x0B))
#define MICS_SERVER_NOTIFY     (0x01)
#define MICS_SERVER_INDICATE   (0x02)



#define GATT_MICS_CLIENT_CONFIG_MASK (MICS_SERVER_NOTIFY | MICS_SERVER_NOTIFY)
#define GET_MICS_CLIENT_CONFIG(config)          (config & GATT_MICS_CLIENT_CONFIG_MASK )

#define MICS_SERVER_MUTE_SIZE (1)

/*! @brief Definition of data required for association.
 */
typedef struct
{
    uint8           micsServerMute;
    uint16          numClients;
    GattMicsClientData connectedClients[MICS_MAX_CONNECTIONS];
} micsData;

#ifdef CSR_TARGET_PRODUCT_VM
/* Definition of data required for handover. */

typedef struct
{
    uint8           micsServerMute;
    uint16          numClients;
    GattMicsClientData  connectedClient;
} gattMicsFirstHandoverData;

typedef struct
{
    bool           marshallerInitialised;
    marshaller_t   marshaller;
    bool           unMarshallerInitialised;
    unmarshaller_t unMarshaller;
    gattMicsFirstHandoverData *handoverFirstConnectionInfo;
    GattMicsClientData *handoverSubsequentConnectionInfo[MICS_MAX_CONNECTIONS-1];
} MicsHandoverMgr;
#endif

/*! @brief The Microphone Control service internal structure for the server role.

    This structure is not visible to the application as it is responsible for
    managing resource to pass to the Media Control library.
    The elements of this structure are only modified by the Media Control Service library.
    The application SHOULD NOT modify the values at any time as it could lead
    to undefined behaviour.
 */
typedef struct GMICS_T
{
    AppTaskData libTask;
    AppTask     appTask;

    /* Service handle of the instance */
    ServiceHandle srvcHandle;

    /*! Information to be provided in service characteristics. */
    micsData data;
    /* Gatt id of the TBS server instance*/
    CsrBtGattId gattId;
    /* Indication pending */
    bool ind_pending;
#ifdef CSR_TARGET_PRODUCT_VM
    uint8 handoverStep;

    /*Handvover data */
    MicsHandoverMgr* micsHandoverMgr;
#endif
} GMICS_T;

#define MAKE_MICS_MESSAGE(TYPE) \
    TYPE * message = (TYPE *)CsrPmemZalloc(sizeof(TYPE))

/*!
 *     micsServerNotifyConnectedClients
 *     Notifies all the connected clients of change in server characteristic
 *     of type 'type'.
 *  */
void micsServerNotifyConnectedClients(GMICS_T* mics);

#define MicsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, MICS_SERVER_PRIM, MSG);\
    }

#endif
