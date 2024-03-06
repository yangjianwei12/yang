/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #58 $
******************************************************************************/
/* 
    FILE NAME
    gatt_transport_discovery_server_private.h

DESCRIPTION
    Header file for the Transport Discovery Server Service data structure.
*/

/*!
@file   gatt_transport_discovery_server_private.h
@brief  Header file for the Transport Discovery Server data structure.

        This file documents the basic data structure of Transport Discovery Service.
*/

#ifndef GATT_TRANSPORT_DISCOVERY_SERVER_PRIVATE_H
#define GATT_TRANSPORT_DISCOVERY_SERVER_PRIVATE_H


#include "gatt_transport_discovery_server.h"
#include "gatt_transport_discovery_server_db.h"
#include "gatt_transport_discovery_server_debug.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"

typedef struct __GTDS
{
    GTDS_T *tds;
} GTDS;

    /* Required octets for values sent to Client Configuration Descriptor */
#define GATT_CLIENT_CONFIG_NUM_OCTETS   2


#define MAKE_TDS_MESSAGE(TYPE) TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))
#define MAKE_TDS_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *)CsrPmemAlloc(sizeof(TYPE) + ((LEN) - 1) * sizeof(uint8))

#define TdsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, TDS_SERVER_PRIM, MSG);\
    }


#endif /* GATT_TRANSPORT_DISCOVERY_SERVER_PRIVATE_H */
