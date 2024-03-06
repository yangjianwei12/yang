/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_GMAS_SERVER_ACCESS_H_
#define GATT_GMAS_SERVER_ACCESS_H_

#include "gatt_gmas_server.h"
#include "gatt_gmas_server_common.h"

#ifndef CSR_TARGET_PRODUCT_VM
#include "gatt_leaudio_server_db.h"
#else
#include "gatt_gmas_server_db.h"
#endif
/***************************************************************************
NAME
    gattGmasServerHandleAccessIndication

DESCRIPTION
    Handle the access indications that were sent
    to the GMAS Server library.
*/
void gattGmasServerHandleAccessIndication(GGMAS *gmasServer,
                                          GATT_MANAGER_SERVER_ACCESS_IND_T *const accessInd);

#endif /* GATT_GMAS_SERVER_ACCESS_H_ */
