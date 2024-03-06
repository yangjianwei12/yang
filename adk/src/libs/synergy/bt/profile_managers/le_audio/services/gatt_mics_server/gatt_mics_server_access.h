/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_MICS_SERVER_ACCESS_H_
#define GATT_MICS_SERVER_ACCESS_H_

#include "gatt_mics_server.h"
#include "gatt_mics_server_private.h"
#include "gatt_mics_server_common.h"


/***************************************************************************
NAME
    micsHandleAccessIndication

DESCRIPTION
    Handle the access indications that were sent
    to the Telephone Bearer Service library.
*/
void micsHandleAccessIndication(
        GMICS_T *mics,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd);


#endif /* GATT_MICS_SERVER_ACCESS_H_ */
