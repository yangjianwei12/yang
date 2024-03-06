/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#ifndef GATT_HIDS_SERVER_HANDOVER_H_
#define GATT_HIDS_SERVER_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"

/***************************************************************************
NAME
    gattHidsServerHandoverVeto

DESCRIPTION
    Veto the handover of HIDS Server data

    @return TRUE if the module wishes to veto the handover attempt.
*/

CsrBool gattHidsServerHandoverVeto(ServiceHandle hidsServiceHandle);

/***************************************************************************
NAME
    gattHidsServerHandoverMarshal

DESCRIPTION
    Marshal HIDS Server data

    @param hidsServiceHandle The server instance handle returned in GattHidsServerInit().
    @param cid connection id of the remote device
    @param buf Address to which the marshaller will write the marshalled byte stream.
    @param length Space in the marshal byte stream buffer
    @param[out] written number of bytes written to the buffer

    @return TRUE if module marshaling complete, otherwise FALSE
*/

CsrBool gattHidsServerHandoverMarshal(ServiceHandle hidsServiceHandle,
                                      CsrBtConnId cid,
                                      CsrUint8 *buf,
                                      CsrUint16 length,
                                      CsrUint16 *written);

/***************************************************************************
NAME
    gattHidsServerHandoverUnmarshal

DESCRIPTION
    Unmarshal HIDS Server data

    @param hidsServiceHandle The server instance handle returned in GattHidsServerInit().
    @param cid connection id of the remote device
    @param buf Address of the byte stream to be unmarshalled.
    @param length Amount of data in the marshal byte stream buffer.
    @param[out] written consumed the number of bytes written to the buffer

    @return TRUE if module unmarshalling complete, otherwise FALSE
*/

CsrBool gattHidsServerHandoverUnmarshal(ServiceHandle hidsServiceHandle,
                                        CsrBtConnId cid,
                                        const CsrUint8 *buf,
                                        CsrUint16 length,
                                        CsrUint16 *written);

/***************************************************************************
NAME
    gattHidsServerHandoverCommit

DESCRIPTION
    Commit HIDS Server handover data

    @param hidsServiceHandle The server instance handle returned in GattHidsServerInit().
    @param cid connection id of the remote device
    @param newPrimary TRUE if TWS primary role requested, else secondary

    @return void
*/

void gattHidsServerHandoverCommit(ServiceHandle hidsServiceHandle,
                                  CsrBtConnId cid,
                                  const bool newPrimary);

/***************************************************************************
NAME
    gattHidsServerHandoverComplete

DESCRIPTION
    Complete the handover of HIDS Server data

    @param hidsServiceHandle The server instance handle returned in GattHidsServerInit().
    @param newPrimary TRUE if TWS primary role requested, else secondary

    @return void
*/

void gattHidsServerHandoverComplete(ServiceHandle hidsServiceHandle, const bool newPrimary);

/***************************************************************************
NAME
    gattHidsServerHandoverAbort

DESCRIPTION
    Abort the handover of HIDS Server data

    @param hidsServiceHandle The server instance handle returned in GattHidsServerInit()

    @return void
*/

void gattHidsServerHandoverAbort(ServiceHandle hidsServiceHandle);

#endif
