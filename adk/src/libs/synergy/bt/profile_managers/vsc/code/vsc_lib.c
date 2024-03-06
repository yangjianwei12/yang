/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#include "vsc_lib.h"
#include "vsc_main.h"


/*----------------------------------------------------------------------------*
 *  NAME
 *      VscRegister
 *
 *  DESCRIPTION
 *      Register the application instance to the VSC library
 *
 *  PARAMETERS
 *        phandle:            application handle
 *----------------------------------------------------------------------------*/
void VscRegister(phandle_t phandle)
{
    VscApplicationInstanceElement *elem = VSC_ADD_APP_INSTANCE(vscBtData.InstanceList);
    VscRegisterCfm *prim = (VscRegisterCfm*)CsrPmemZalloc(sizeof(VscRegisterCfm));

    elem->handle = phandle;

    prim->type = VSC_REGISTER_CFM;
    prim->result = VSC_RESULT_SUCCESS;

    VSC_PUT_MESSAGE(phandle, prim);

    vscBtData.len += 1;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscDeregister
 *
 *  DESCRIPTION
 *      Deregister the application instance from the VSC library.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *----------------------------------------------------------------------------*/
void VscDeregister(phandle_t phandle)
{
    VscApplicationInstanceElement *elem = VSC_FIND_HANDLE(vscBtData.InstanceList, &phandle);
    VSC_REMOVE_INSTANCE(vscBtData.InstanceList, elem);

    vscBtData.len -= 1;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscRegisterReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_REGISTER_REQ to register with VSDM.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *----------------------------------------------------------------------------*/
void VscRegisterReqSend(phandle_t phandle)
{
    VSDM_REGISTER_REQ_T *prim = (VSDM_REGISTER_REQ_T*)CsrPmemAlloc(sizeof(VSDM_REGISTER_REQ_T));

    prim->type = VSDM_REGISTER_REQ;

    if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
    {
        /* Store Application handle in prim */
        prim->phandle = phandle;

        CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
    }
    else
    {
        prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
        vscBtData.vscVar.appHandle = phandle;
        VSC_PutMsg(prim);
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadLocalQlmSuppFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ which returns the Qlm
 *      features that are supported.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *----------------------------------------------------------------------------*/
void VscReadLocalQlmSuppFeaturesReqSend(phandle_t phandle)
{
    VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T *prim = (VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T*)CsrPmemAlloc(sizeof (VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T));
    prim->type = VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ;

    if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
    {
        /* Store Application handle in prim */
        prim->phandle = phandle;

        CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
    }
    else
    {

        VSC_LOCK_QUEUE(vscBtData);
        prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
        vscBtData.vscVar.appHandle = phandle;
        VSC_PutMsg(prim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadRemoteQlmSuppFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ to recieve the QLM features
 *      that are supported by the remote device.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *        handle:             connection handle
 *        bdaddr:             bluetooth device address
 *----------------------------------------------------------------------------*/
void VscReadRemoteQlmSuppFeaturesReqSend(phandle_t phandle,
                                         CsrBdAddr bdaddr)
{
    VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T *prim = (VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T*)CsrPmemAlloc(sizeof (VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T));
    prim->type = VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ;
    prim->handle = 0;
    prim->bd_addr = bdaddr;

    if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
    {
        /* Store Application handle in prim */
        prim->phandle = phandle;

        CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
    }
    else
    {
        VSC_LOCK_QUEUE(vscBtData);
        prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
        vscBtData.vscVar.appHandle = phandle;
        VSC_PutMsg(prim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscWriteScHostSuppOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ which sets the an array of compID,
 *      min_lmpVersion, and min_lmpSubVersion pararmeters to be used by the
 *      Controller to determine whether to override the
 *      Secure_Connections_Host_Support_LMP Feature bit.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *        numCompIds:         number of compIDs
 *        compId:             compIDs to apply host mode override values
 *        minLmpVersion:      min_lmpVersion associated with compIDs
 *        minLmpSubVersion:   min_lmpSubVersion associated with compIDs
 *----------------------------------------------------------------------------*/
void VscWriteScHostSuppOverrideReqSend(phandle_t phandle,
                                       uint8_t numCompIds,
                                       uint16_t compId[],
                                       uint8_t minLmpVersion[],
                                       uint16_t minLmpSubVersion[])
{
   CsrUint8 i;
   VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T *prim = (VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T*)CsrPmemAlloc(sizeof (VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T));
   prim->type = VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ;
   prim->num_compIDs = numCompIds;

   for (i = 0; i < VSDM_MAX_NO_OF_COMPIDS; i++)
   {
       prim->compID[i] = compId[i];
       prim->min_lmpVersion[i] = minLmpVersion[i];
       prim->min_lmpSubVersion[i] = minLmpSubVersion[i];
   }

   if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
   {
       /* Store Application handle in prim */
       prim->phandle = phandle;

       CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
   }
   else
   {
       VSC_LOCK_QUEUE(vscBtData);
       prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
       vscBtData.vscVar.appHandle = phandle;
       VSC_PutMsg(prim);
   }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadScHostSuppOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ which reads Controller to
 *      determine whether to override the Secure_Connections_Host_Support_LMP
 *      Feature bit.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *----------------------------------------------------------------------------*/
void VscReadScHostSuppOverrideReqSend(phandle_t phandle){

   VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T *prim = (VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T*)CsrPmemAlloc(sizeof (VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T));

   prim->type = VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ;

   if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
   {
       /* Store Application handle in prim */
       prim->phandle = phandle;

       CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
   }
   else
   {
       VSC_LOCK_QUEUE(vscBtData);
       prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
       vscBtData.vscVar.appHandle = phandle;
       VSC_PutMsg(prim);
   }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscWriteScHostSuppCodOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ which writes the bit number
 *      of the CoD and corresponding value to be used by the Controller to
 *      determine whether to override the Secure_Connections_Host_Support LMP
 *      Feature bit.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *        bitNumber:          bit position in the class of device(0 to 23)
 *        enable:             enable or disable SC based on class of device
 *----------------------------------------------------------------------------*/
void VscWriteScHostSuppCodOverrideReqSend(phandle_t phandle,
                                          uint8_t bitNumber,
                                          VscScCodType enable)
{
   VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ_T *prim = (VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ_T*)CsrPmemAlloc(sizeof (VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ_T));

   prim->type = VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ;
   prim->bit_number = bitNumber;
   prim->enable = enable;

   if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
   {
       /* Store Application handle in prim */
       prim->phandle = phandle;

       CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
   }
   else
   {
       VSC_LOCK_QUEUE(vscBtData);
       prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
       vscBtData.vscVar.appHandle = phandle;
       VSC_PutMsg(prim);
   }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadScHostSuppCodOverrideReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ which reads the Bit_Number
 *      and Bit_Value parameters in the Class of Device (CoD) used by the Controller.
 *
 *  PARAMETERS
 *        phandle:            Application handle
 *----------------------------------------------------------------------------*/
void VscReadScHostSuppCodOverrideReqSend(phandle_t phandle)
{
    VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ_T *prim = (VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ_T*)CsrPmemAlloc(sizeof (VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ_T));
    prim->type = VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ;

    if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
    {
        /* Store Application handle in prim */
        prim->phandle = phandle;

        CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
    }else
    {
        VSC_LOCK_QUEUE(vscBtData);
        prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
        vscBtData.vscVar.appHandle = phandle;
        VSC_PutMsg(prim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscSetQhsHostModeReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_SET_QHS_HOST_MODE_REQ which is used by the Host to tell the
 *       Controller which QHS mode to use on the indicated Transport.

 *
 *  PARAMETERS
 *        phandle:            application handle
 *        transport:          transport type
 *        qhdHostMode:        QHS mode type host want to set
 *----------------------------------------------------------------------------*/
void VscSetQhsHostModeReqSend(phandle_t phandle,
                              VscQhsTransport transport,
                              VscQhsHostMode qhsHostMode)
{
   VSDM_SET_QHS_HOST_MODE_REQ_T *prim = (VSDM_SET_QHS_HOST_MODE_REQ_T*)CsrPmemAlloc(sizeof (VSDM_SET_QHS_HOST_MODE_REQ_T));
   prim->type = VSDM_SET_QHS_HOST_MODE_REQ;
   prim->transport = transport;
   prim->qhs_host_mode = qhsHostMode;

   if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
   {
       /* Store Application handle in prim */
       prim->phandle = phandle;

       CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
   }
   else
   {
       VSC_LOCK_QUEUE(vscBtData);
       prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
       vscBtData.vscVar.appHandle = phandle;
       VSC_PutMsg(prim);
   }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscSetWbmFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_SET_WBM_FEATURES_REQ which is used to enable and disable the
 *      support of the Weak Bitmask (WBM)on a connection handle basis.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *        enable_mask:        enable or disable WBM features
 *----------------------------------------------------------------------------*/
void VscSetWbmFeaturesReqSend(phandle_t phandle,
                              hci_connection_handle_t conn_handle,
                              enable_bit_mask_t enableMask)
{
   VSDM_SET_WBM_FEATURES_REQ_T *prim = (VSDM_SET_WBM_FEATURES_REQ_T*)CsrPmemAlloc(sizeof (VSDM_SET_WBM_FEATURES_REQ_T));

   prim->type = VSDM_SET_WBM_FEATURES_REQ;
   prim->conn_handle = conn_handle;
   prim->enable_mask = enableMask;

   if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
   {
       /* Store Application handle in prim */
       prim->phandle = phandle;

       CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
   }
   else
   {
       VSC_LOCK_QUEUE(vscBtData);
       prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
       vscBtData.vscVar.appHandle = phandle;
       VSC_PutMsg(prim);
   }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscConvertRpaToIaReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_CONVERT_RPA_TO_IA_REQ which converts RPA to Ia.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *        rpa:                resolvable private address
 *----------------------------------------------------------------------------*/
void VscConvertRpaToIaReqSend(phandle_t phandle, CsrBdAddr rpa)
{
   VSDM_CONVERT_RPA_TO_IA_REQ_T *prim = (VSDM_CONVERT_RPA_TO_IA_REQ_T*)CsrPmemAlloc(sizeof (VSDM_CONVERT_RPA_TO_IA_REQ_T));

   prim->type = VSDM_CONVERT_RPA_TO_IA_REQ;
   prim->rpa = rpa;

   if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
   {
       /* Store Application handle in prim */
       prim->phandle = phandle;

       CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
   }
   else
   {
       VSC_LOCK_QUEUE(vscBtData);
       prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
       vscBtData.vscVar.appHandle = phandle;
       VSC_PutMsg(prim);
   }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscWriteTruncatedPageScanEnableReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ_T which enables or
 *      disables Truncated Page Scan.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *        enable:             state to be set
 *----------------------------------------------------------------------------*/
void VscWriteTruncatedPageScanEnableReqSend(phandle_t phandle, uint8_t enable)
{
    VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ_T *prim = (VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ_T*)CsrPmemAlloc(sizeof(VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ_T));

    prim->type = VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ;
    prim->phandle = 0x0000;
    prim->enable = enable;

    if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
    {
        /* Store Application handle in prim */
        prim->phandle = phandle;

        CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
    }
    else
    {
        VSC_LOCK_QUEUE(vscBtData);
        vscBtData.vscVar.appHandle = phandle;
        VSC_PutMsg(prim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscSetStreamingModeReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_SET_STREAMING_MODE_REQ, application is expected to
 *      receive VscSetStreamingModeCfm from the library.
 *
 *  PARAMETERS
 *        phandle:            destination handle
 *        tp_addrt:           address of remote device
 *        streaming_mode:     the type of streaming mode to be set
 *----------------------------------------------------------------------------*/
void VscSetStreamingModeReqSend(phandle_t phandle, TP_BD_ADDR_T *tp_addrt, uint8_t streaming_mode)
{
    VSDM_SET_STREAMING_MODE_REQ_T *prim = (VSDM_SET_STREAMING_MODE_REQ_T*)CsrPmemAlloc(sizeof(VSDM_SET_STREAMING_MODE_REQ_T));

    prim->type = VSDM_SET_STREAMING_MODE_REQ;
    prim->tp_addrt.addrt.type = tp_addrt->addrt.type;
    prim->tp_addrt.addrt.addr.lap = tp_addrt->addrt.addr.lap;
    prim->tp_addrt.addrt.addr.nap = tp_addrt->addrt.addr.nap;
    prim->tp_addrt.addrt.addr.uap = tp_addrt->addrt.addr.uap;
    prim->streaming_mode = streaming_mode;

    if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
    {
        /* Store Application handle in prim */
        prim->phandle = phandle;

        CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
    }
    else
    {
        VSC_LOCK_QUEUE(vscBtData);
        prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
        vscBtData.vscVar.appHandle = phandle;
        VSC_PutMsg(prim);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      VscReadRemoteQllSuppFeaturesReqSend
 *
 *  DESCRIPTION
 *      Sends VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ to recieve the QLL features
 *      that are supported by the remote device.
 *
 *  PARAMETERS
 *        phandle:            application handle
 *        tpAddrt:            bluetooth device address
 *----------------------------------------------------------------------------*/
void VscReadRemoteQllSuppFeaturesReqSend(phandle_t phandle,
                                         CsrBtTypedAddr tpAddrt)
{
    VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ_T *prim = (VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ_T*)CsrPmemAlloc(sizeof (VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ_T));
    prim->type = VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ;
    prim->handle = 0;

    prim->tp_addrt.addrt.type = tpAddrt.type;
    prim->tp_addrt.addrt.addr.lap = tpAddrt.addr.lap;
    prim->tp_addrt.addrt.addr.uap = tpAddrt.addr.uap;
    prim->tp_addrt.addrt.addr.nap = tpAddrt.addr.nap;

    if(VSC_QUEUE_LOCKED(&vscBtData.vscVar))
    {
        /* Store Application handle in prim */
        prim->phandle = phandle;

        CsrMessageQueuePush(&vscBtData.vscVar.saveQueue, BT_VSDM_PRIM, prim);
    }
    else
    {
        VSC_LOCK_QUEUE(vscBtData);
        prim->phandle = CSR_BT_VSDM_IFACEQUEUE;
        vscBtData.vscVar.appHandle = phandle;
        VSC_PutMsg(prim);
    }
}
