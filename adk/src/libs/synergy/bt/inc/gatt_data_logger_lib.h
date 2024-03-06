#ifndef GATT_DATA_LOGGER_LIB_H__
#define GATT_DATA_LOGGER_LIB_H__
/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/


#include "csr_synergy.h"
#include "csr_msg_transport.h"
#include "csr_bt_gatt_prim.h"
#include "csr_bt_gatt_utils.h"
#include "csr_bt_tasks.h"
#include "csr_pmem.h"

#ifdef __cplusplus
extern "C" {
#endif


/* --------------------------------------------------------------------
   Name:
   GattDataLoggerSendPrim

   Description:
   Using this API, Application response for GATT transaction shall be
   posted to GATT data logger utililty.

   Parameters: None
   -------------------------------------------------------------------- */

void GattDataLoggerSendPrim(void);


/* --------------------------------------------------------------------
   Name:
   GattDataLoggerRegisterClientHandles

   Description:
   Using this API, GATT Client service register discovered attribute handles
   of remote connected server for GATT data logger utililty.

   Parameters: None
    CsrSchedQid   _phandle   - Protocol handle of the higher layer entity 
    CsrBtConnId   _btConnId  - Connection identifier 
    void * _clientHandles:  An allocated pointer of the attribute handle values

  Returns:
    This returns a TRUE on success. On failure it returns FALSE 

   -------------------------------------------------------------------- */


bool GattDataLoggerRegisterClientHandles(CsrSchedQid phandle,
                                          CsrBtConnId btConnId,
                                            void *clientHandles);



#endif /* GATT_DATA_LOGGER_LIB_H__ */
