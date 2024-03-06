/*******************************************************************************

Copyright (C) 2019 - 2020 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

#include "dmlib_private.h"

/*----------------------------------------------------------------------------*
 *  NAME
 *      dm_periodic_scan_sync_to_train_req
 *
 *  DESCRIPTION
 *      Build and send a DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ primitive.
 *      If pp_prim != NULL, primitive is returned, not sent.
 *
 *  RETURNS
 *      void
 *
 *----------------------------------------------------------------------------*/
void dm_periodic_scan_sync_to_train_req(
    phandle_t       phandle,
    uint8_t         report_periodic,
    uint16_t        skip,
    uint16_t        sync_timeout,
    uint8_t         sync_cte_type,
    uint16_t        attempt_sync_for_x_seconds,
    uint8_t         number_of_periodic_trains,
    DM_ULP_PERIODIC_SCAN_TRAINS_T  periodic_trains[],
    DM_UPRIM_T    **pp_prim
    )
{
    uint8_t index;
    DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ_T *prim =
            zpnew(DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ_T);

    prim->type = DM_ULP_PERIODIC_SCAN_SYNC_TO_TRAIN_REQ;
    prim->phandle = phandle;
    prim->report_periodic = report_periodic;
    prim->skip = skip;
    prim->sync_timeout = sync_timeout;
    prim->sync_cte_type = sync_cte_type;
    prim->attempt_sync_for_x_seconds = attempt_sync_for_x_seconds;
    prim->number_of_periodic_trains = number_of_periodic_trains;

    for (index = 0; index < DM_MAX_PERIODIC_TRAIN_LIST_SIZE && index < number_of_periodic_trains; index++)
    {
        prim->periodic_trains[index] = periodic_trains[index];
    }

    if (pp_prim)
    {
        *pp_prim = (DM_UPRIM_T *) prim;
    }
    else
    {
         DM_PutMsg(prim);
    }
}
