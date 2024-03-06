/******************************************************************************
 Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef GATT_TMAS_SERVER_ACCESS_H_
#define GATT_TMAS_SERVER_ACCESS_H_

#include "gatt_tmas_server.h"
#include "gatt_tmas_server_common.h"

#ifndef CSR_TARGET_PRODUCT_VM
#include "gatt_leaudio_server_db.h"
#else
#include "gatt_tmas_server_db.h"
#endif
/***************************************************************************
NAME
    gattTmasServerHandleAccessIndication

DESCRIPTION
    Handle the access indications that were sent
    to the TMAS Server library.
*/
void gattTmasServerHandleAccessIndication(GTMAS *tmasServer,
                                          GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd);

#endif /* GATT_TMAS_SERVER_ACCESS_H_ */
