/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROLE_SELECTION_SERVER_ACCESS_H_
#define GATT_ROLE_SELECTION_SERVER_ACCESS_H_

#include <gatt_lib.h>

#include "gatt_role_selection_server.h"

/*! Handles the read access message that was sent to 
    the role selection library.
*/
void handleRoleSelectionServiceReadAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                          const CsrBtGattDbAccessReadInd *access_ind);

/*! Handles the write access message that was sent to 
    the role selection library.
*/
void handleRoleSelectionServiceWriteAccess(GATT_ROLE_SELECTION_SERVER *instance, 
                                           const CsrBtGattDbAccessWriteInd *access_ind);

#endif /* GATT_ROLE_SELECTION_SERVER_ACCESS_H_ */
