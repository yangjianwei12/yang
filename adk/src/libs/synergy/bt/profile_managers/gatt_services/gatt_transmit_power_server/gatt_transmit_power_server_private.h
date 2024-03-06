/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

/* 
    FILE NAME
    gatt_transmit_power_server_private.h

DESCRIPTION
    Header file for the Transmit Power Server Service data structure.
*/

/*!
@file   gatt_transmit_power_server_private.h
@brief  Header file for the Transmit Power Server data structure.

        This file documents the basic data structure of Transmit Power Server Service.
*/

#ifndef GATT_TRANSMIT_POWER_SERVER_PRIVATE_H
#define GATT_TRANSMIT_POWER_SERVER_PRIVATE_H

#include "gatt_transmit_power_server.h"
#include "gatt_transmit_power_server_db.h"
#include "gatt_transmit_power_server_debug.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_lib.h"

typedef struct __GTPSS_T
{
    GTPSS *tps;
} GTPSS_T;


#define MAKE_TPS_MESSAGE(TYPE) TYPE *message = (TYPE*)CsrPmemZalloc(sizeof(TYPE))
#define MAKE_TPS_MESSAGE_WITH_LEN(TYPE, LEN) TYPE *message = (TYPE *)CsrPmemAlloc(sizeof(TYPE) + ((LEN) - 1) * sizeof(uint8))

#define TpsMessageSend(TASK, ID, MSG) {\
    MSG->id = ID; \
    CsrSchedMessagePut(TASK, TPS_SERVER_PRIM, MSG);\
    }

#endif /* GATT_TRANSMIT_POWER_SERVER_PRIVATE_H */
