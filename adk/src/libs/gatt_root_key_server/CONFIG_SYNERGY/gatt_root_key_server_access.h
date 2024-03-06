/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_ROOT_KEY_SERVER_ACCESS_H_
#define GATT_ROOT_KEY_SERVER_ACCESS_H_


/***************************************************************************
NAME
    handleRootKeyReadAccess

DESCRIPTION
    Handles the CsrBtGattDbAccessReadInd message that was sent to 
    the root key library.
*/
void handleRootKeyReadAccess(GATT_ROOT_KEY_SERVER *instance,
                             const CsrBtGattDbAccessReadInd *access_ind);


/***************************************************************************
NAME
    handleRootKeyWriteAccess

DESCRIPTION
    Handles the CsrBtGattDbAccessWriteInd message that was sent to 
    the root key library.
*/
void handleRootKeyWriteAccess(GATT_ROOT_KEY_SERVER *instance,
                              const CsrBtGattDbAccessWriteInd *access_ind);



#endif /* GATT_ROOT_KEY_SERVER_ACCESS_H_ */
