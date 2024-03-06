#ifndef OBEX_HANDOVER_H
#define OBEX_HANDOVER_H
/******************************************************************************
 Copyright (c) 2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_marshal_util.h"
#include "csr_bt_obex_private_util.h"

/*-------------------------------------------------------------------------------------*
 *  NAME
 *      ObexHandoverVeto
 *
 *  DESCRIPTION
 *      Vetoes the handover if the processState is not stable, cmState is disconnected
 *      and if data transmission is ongoing.
 *
 *  PARAMETERS
 *      connInstIdList: List of instance IDs which are in connected state.
 *      noOfConnInst  : Count of instance IDs present in the list.
 *-------------------------------------------------------------------------------------*/
CsrBool ObexHandoverVeto(CsrSchedQid *connInstIdList,
                         CsrUint8 noOfConnInst);

/*-------------------------------------------------------------------------------------*
 *  NAME
 *      ObexHandoverSerInstData
 *
 *  DESCRIPTION
 *      Serializes and writes the required data into the marshal object converter for
 *      the obex instances identified by the instance IDs provided by the calling profile.
 *
 *  PARAMETERS
 *      conv          : The marshal object provided by the calling profile.
 *      connInstIdList: List of instance IDs which are in connected state.
 *      noOfConnInst  : Count of instance IDs present in the list.
 *-------------------------------------------------------------------------------------*/
void ObexHandoverSerInstData(CsrBtMarshalUtilInst *conv,
                             CsrSchedQid *connInstIdList,
                             CsrUint8 noOfConnInst);

/*-------------------------------------------------------------------------------------*
 *  NAME
 *      ObexHandoverDeserInstData
 *
 *  DESCRIPTION
 *      Deserializes and reads the data from the marshal object converter to
 *      populate the obex instances identified by the instance IDs provided by the
 *      calling profile.
 *
 *  PARAMETERS
 *      conv          : The marshal object provided by the calling profile.
 *      connInstIdList: List of instance IDs which are in connected state.
 *      noOfConnInst  : Count of instance IDs present in the list.
 *-------------------------------------------------------------------------------------*/
void ObexHandoverDeserInstData(CsrBtMarshalUtilInst *conv,
                               CsrSchedQid *connInstIdList,
                               CsrUint8 noOfConnInst);

/*-------------------------------------------------------------------------------------*
 *  NAME
 *      ObexHandoverCommit
 *
 *  DESCRIPTION
 *      Registers the streams and populates required data for the obex instances
 *      identified by the instance IDs provided by the calling profile.
 *
 *  PARAMETERS
 *      connInstIdList        : List of instance IDs which are in connected state.
 *      noOfConnInst          : Count of instance IDs present in the list.
 *      authenticateIndHandler: Callback function of the calling profile to handle
 *                              authenticate indication
 *      disconnectIndHandler  : Callback function of the calling profile to handle
 *                              disconnect indication
 *-------------------------------------------------------------------------------------*/
void ObexHandoverCommit(CsrSchedQid *connInstIdList,
                        CsrUint8 noOfConnInst,
                        ObexUtilAuthenticateIndFuncType authenticateIndHandler,
                        ObexUtilDisconnectIndFuncType disconnectIndHandler);

/*-------------------------------------------------------------------------------------*
 *  NAME
 *      ObexHandoverAbort
 *
 *  DESCRIPTION
 *      Clean up the data written during ObexHandoverDeserInstData() for the obex instances
 *      identified by the instance IDs provided by the calling profile.
 *
 *  PARAMETERS
 *      connInstIdList: List of instance IDs which are in connected state.
 *      noOfConnInst  : Count of instance IDs present in the list.
 *-------------------------------------------------------------------------------------*/
void ObexHandoverAbort(CsrSchedQid *connInstIdList,
                       CsrUint8 noOfConnInst);

#endif /* OBEX_HANDOVER_H */
