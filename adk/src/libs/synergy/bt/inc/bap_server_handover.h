/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #57 $
******************************************************************************/

#ifndef BAP_SERVER_HANDOVER_H_
#define BAP_SERVER_HANDOVER_H_


#include "bdaddr.h"

/***************************************************************************
NAME
    BapServerHandoverVeto

DESCRIPTION
    Veto the handover of BAP Server data

    @return TRUE if the module wishes to veto the handover attempt.
*/

bool BapServerHandoverVeto(void);

/***************************************************************************
NAME
    BapServerHandoverMarshal

DESCRIPTION
    Marshal BAP Server data

    @param tpBdAddr Bluetooth address of the link to be marshalled
    @param buf Address to which the marshaller will write the marshalled byte stream.
    @param length Space in the marshal byte stream buffer
    @param written Number of bytes written to the buffer

    @return TRUE if module marshaling complete, otherwise FALSE
*/

bool BapServerHandoverMarshal(const tp_bdaddr *tpBdAddr,
                              uint8 *buf,
                              uint16 length,
                              uint16 *written);

/***************************************************************************
NAME
    BapServerHandoverUnmarshal

DESCRIPTION
    Unmarshal BAP Server data

    @param tpBdAddr Bluetooth address of the link to be unmarshalled
    @param buf Address of the byte stream to be unmarshalled.
    @param length Amount of data in the marshal byte stream buffer.
    @param consumed The number of bytes written to the buffer

    @return TRUE if module unmarshalling complete, otherwise FALSE
*/

bool BapServerHandoverUnmarshal(const tp_bdaddr *tpBdAddr,
                                const uint8 *buf,
                                uint16 length,
                                uint16 *consume);

/***************************************************************************
NAME
    BapServerHandoverCommit

DESCRIPTION
    Commit BAP Server handover data

    @param tpBdAddr Bluetooth address of the link to be committed
    @param newPrimary TRUE if TWS primary role requested, else secondary

    @return void
*/

void BapServerHandoverCommit(const tp_bdaddr *tpBdAddr,
                             const bool newPrimary);

/***************************************************************************
NAME
    BapServerHandoverComplete

DESCRIPTION
    Complete the handover of BAP Server data

    @param newPrimary TRUE if TWS primary role requested, else secondary

    @return void
*/

void BapServerHandoverComplete(const bool newPrimary);

/***************************************************************************
NAME
    BapServerHandoverAbort

DESCRIPTION
    Abort the handover of BAP Server data

    @return void
*/

void BapServerHandoverAbort(void);

#endif
