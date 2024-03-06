/* Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#ifndef CSIP_INDICATION_H_
#define CSIP_INDICATION_H_

#include "gatt_csis_client.h"

#include "csip_private.h"

/***************************************************************************
NAME
    csipHandleLockStatusInd

DESCRIPTION
    Handle a GATT_CSIS_CLIENT_NOTIFICATION_IND message for LOCK characteristic.
*/
void csipHandleLockStatusInd(Csip *csipInst, uint8 lockStatus);

/***************************************************************************
NAME
    csipHandleSizeChangedInd

DESCRIPTION
    Handle a GATT_CSIS_CLIENT_NOTIFICATION_IND message for SIZE characteristic.
*/
void csipHandleSizeChangedInd(Csip *csipInst, uint8 sizeValue);

/***************************************************************************
NAME
    csipHandleSizeChangedInd

DESCRIPTION
    Handle a GATT_CSIS_CLIENT_NOTIFICATION_IND message for SIRK characteristic.
*/
void csipHandleSirkChangedInd(Csip *csipInst, uint8 *sirk);

#endif
