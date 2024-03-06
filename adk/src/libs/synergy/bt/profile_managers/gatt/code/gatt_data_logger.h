#ifndef GATT_DATA_LOGGER_H__
#define GATT_DATA_LOGGER_H__
/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_gatt_main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t            cid;                /*!< ATT bearer handle */
    uint16_t            connectionHandle;  /*!< ATT connection LE-ACL handle */
    uint8_t             locallyOriginated; /*!< Indication resultant of locally
                                                   originated request */
    uint16_t            valueLength;       /*!< Data length */
    uint8_t             value[1];           /*!< Refer to the ATT Req/Rsp PDU, 
                                              starts with opcode at first octet */
} GattAttDebugInfo; 


void gattDataLoggerUpstreamMsgIndSend( CsrSchedQid phandle, void *msg );


void gattDataLoggerUpstreamAttDebugIndSend( void *msg );

#ifdef __cplusplus
}
#endif

#endif /* GATT_DATA_LOGGER_H__ */

