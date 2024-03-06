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

/*****************************************************************************/
bool ConnectionSetRootKeys(cl_root_keys* root_keys)
{
    UNUSED(root_keys);

    /* Not Supported */
    return FALSE;
}

/*****************************************************************************/
bool ConnectionSmGetLocalIrk(cl_irk *clirk)
{
    UNUSED(clirk);

    /* Not Supported */
    return FALSE;
}

