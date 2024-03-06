/******************************************************************************
 Copyright (c) 2020-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_MCS_SERVER_ACCESS_H_
#define GATT_MCS_SERVER_ACCESS_H_

#include "gatt_mcs_server.h"
#include "gatt_mcs_server_private.h"
#include "gatt_mcs_server_common.h"

#ifndef CSR_TARGET_PRODUCT_VM
#include "gatt_leaudio_server_db.h"
#else
#include "gatt_mcs_server_db.h"
#endif

/***************************************************************************
NAME
    mcsFindCid

DESCRIPTION
    Find index of the provided cid
*/
bool mcsFindCid(const GMCS_T *mcs, connection_id_t cid, uint8 *index);

/***************************************************************************
NAME
    mcsHandleAccessIndication

DESCRIPTION
    Handle the access indications that were sent
    to the Media Control Service Service library.
*/
void mcsHandleAccessIndication(
        GMCS_T *mcs,
        GATT_MANAGER_SERVER_ACCESS_IND_T const *accessInd,
        uint16 maxRespValueLen);


/***************************************************************************
NAME
    checkValidOpcode

DESCRIPTION
    Validate the opcode for the given call id returns new state following
    opcode or if invalid transition assumed that the
    callIndex is valid
*/
bool checkValidOpcode(GattMcsOpcodeTypeSupported supportedOc,
                                       GattMcsMediaControlPointType oc);

/***************************************************************************
NAME
    validatePlayingOrder

DESCRIPTION
    Validate the opcode for the given call id returns new state following
    opcode or TBS_CALL_STATE_INVALID if invalid transition assumed that the
    callIndex is valid
*/

bool validatePlayingOrder(GattMcsMediaSupportedPlayingOrderType pos,
                                       GattMcsMediaPlayingOrderType po);


void sendMcsMediaControlPointAccessResponse(
                                   GMCS_T *mcs,
                                   connection_id_t cid,
                                   GattMcsMediaControlPointType opcode,
                                   GattMediaControlPointAccessResultCode resultCode);

#endif /* GATT_MCS_SERVER_ACCESS_H_ */
