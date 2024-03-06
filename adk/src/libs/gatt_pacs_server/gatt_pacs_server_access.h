/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#ifndef GATT_PACS_SERVER_ACCESS_H_
#define GATT_PACS_SERVER_ACCESS_H_

#include "gatt_pacs_server_private.h"

/***************************************************************************
NAME
    handlePacsServerAccessInd

DESCRIPTION
    Handle the access indications that were sent
    to the PACS Server library.
*/
void handlePacsServerAccessInd(GPACSS_T *pacs, const GATT_MANAGER_SERVER_ACCESS_IND_T *access_ind);

#endif

