/****************************************************************************
Copyright (c) 2021 Qualcomm Technologies International, Ltd.


FILE NAME
    connection_key_manipulation.c

DESCRIPTION
    This file contains the management entity responsible for manipulating keys

NOTES
    Builds requiring this should include CONFIG_KEY_MANIPULATION in the
    makefile. e.g.
        CONFIG_FEATURES:=CONFIG_KEY_MANIPULATION
*/

#include "connection.h"
#include "connection_no_ble.h"
#include <logging.h>
#include <stdlib.h>
#include <bdaddr.h>
#include <csr_bt_td_db.h>
#include <cm_lib.h>

/*****************************************************************************/
/*! 
    @brief This function updates Updates ER (Encryption Root) 
    and IR (Identity Root) root key values of the device.

    @return true if the keys are updated successfully, false otherwise.
*/
bool ConnectionSetRootKeys(cl_root_keys* root_keys)
{
    packed_root_keys rtks;
    CsrBtTdDbSystemInfo systemInfo = { 0 };

    if (root_keys)
    {
        memmove(&rtks.er, root_keys->er, sizeof(rtks.er));
        memmove(&rtks.ir, root_keys->ir, sizeof(rtks.ir));

        if (VmUpdateRootKeys(&rtks))
        {
            /* Rewrite keys, so the local IRK is updated */
            CmRefreshAllDevices();

            CsrBtTdDbGetSystemInfo(&systemInfo);
            memcpy(&systemInfo.er, root_keys->er, sizeof(systemInfo.er));
            memcpy(&systemInfo.ir, root_keys->ir, sizeof(systemInfo.ir));
            return (CsrBtTdDbSetSystemInfo(&systemInfo) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS);
        }
    }

    return FALSE;
}
/*****************************************************************************/
/*! 
    @brief This function retrieves the local Identity Resolving Key (IRK).
    @return true if successfully retrieved the IRK
*/
bool ConnectionSmGetLocalIrk(cl_irk *clirk)
{
    packed_irk pirk;

    if (clirk)
    {
        if (VmGetLocalIrk(&pirk))
        {
            memmove(&clirk->irk, &pirk, sizeof(clirk->irk));
            return TRUE;
        }
    }

    return FALSE;
}

