/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#ifndef GATT_PACS_SERVER_HANDOVER_H_
#define GATT_PACS_SERVER_HANDOVER_H_

#include "service_handle.h"
#include "csr_bt_profiles.h"

/***************************************************************************
NAME
    gattPacsServerHandoverVeto

DESCRIPTION
    Veto the handover of PACS Server data

    @return TRUE if the module wishes to veto the handover attempt.
*/

CsrBool gattPacsServerHandoverVeto(void);

/***************************************************************************
NAME
    gattPacsServerHandoverMarshal

DESCRIPTION
    Marshal PACS Server data

    @param pacsServiceHandle The server instance handle returned in GattPacsServerInit().
    @param cid connection id of the remote device
    @param buf Address to which the marshaller will write the marshalled byte stream.
    @param length Space in the marshal byte stream buffer
    @param[out] written number of bytes written to the buffer

    @return TRUE if module marshaling complete, otherwise FALSE
*/

CsrBool gattPacsServerHandoverMarshal(ServiceHandle pacsServiceHandle,
                                      CsrBtConnId cid,
                                      CsrUint8 *buf,
                                      CsrUint16 length,
                                      CsrUint16 *written);

/***************************************************************************
NAME
    gattPacsServerHandoverUnmarshal

DESCRIPTION
    Unmarshal PACS Server data

    @param pacsServiceHandle The server instance handle returned in GattPacsServerInit().
    @param cid connection id of the remote device
    @param buf Address of the byte stream to be unmarshalled.
    @param length Amount of data in the marshal byte stream buffer.
    @param[out] written consumed the number of bytes written to the buffer

    @return TRUE if module unmarshalling complete, otherwise FALSE
*/

CsrBool gattPacsServerHandoverUnmarshal(ServiceHandle pacsServiceHandle,
                                        CsrBtConnId cid,
                                        const CsrUint8 *buf,
                                        CsrUint16 length,
                                        CsrUint16 *written);

/***************************************************************************
NAME
    gattPacsServerHandoverCommit

DESCRIPTION
    Commit PACS Server handover data

    @param pacsServiceHandle The server instance handle returned in GattPacsServerInit().
    @param cid connection id of the remote device
    @param newPrimary TRUE if TWS primary role requested, else secondary

    @return void
*/

void gattPacsServerHandoverCommit(ServiceHandle pacsServiceHandle,
                                  CsrBtConnId cid,
                                  const bool newPrimary);

/***************************************************************************
NAME
    gattPacsServerHandoverComplete

DESCRIPTION
    Complete the handover of PACS Server data

    @param pacsServiceHandle The server instance handle returned in GattPacsServerInit().
    @param newPrimary TRUE if TWS primary role requested, else secondary

    @return void
*/

void gattPacsServerHandoverComplete(ServiceHandle pacsServiceHandle, const bool newPrimary);

/***************************************************************************
NAME
    gattPacsServerHandoverAbort

DESCRIPTION
    Abort the handover of PACS Server data

    @param pacsServiceHandle The server instance handle returned in GattPacsServerInit()

    @return void
*/

void gattPacsServerHandoverAbort(ServiceHandle pacsServiceHandle);

#endif
