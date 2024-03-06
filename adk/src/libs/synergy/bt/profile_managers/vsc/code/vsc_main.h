/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/
#ifndef VSC_MAIN_H
#define VSC_MAIN_H

#include "vsc_lib.h"
#include "csr_message_queue.h"
#include "csr_types.h"
#include "hci_prim.h"

#include "csr_pmem.h"
#include "csr_list.h"
#include "csr_log_text_2.h"

#define VSC_QUEUE_UNLOCK           ((CsrUint16) 0xFFFF)
#define VSC_QUEUE_LOCK             ((CsrUint16) 0x0000)
#define VSC_QUEUE_LOCKED(_dm)      ((_dm)->lockMsg != VSC_QUEUE_UNLOCK)
#define VSC_SCHED_QID_INVALID      ((CsrSchedQid) 0xFFFF)
CSR_LOG_TEXT_HANDLE_DECLARE(CsrBtVscLto);
#define VSC_TEXT_REGISTER           CSR_LOG_TEXT_REGISTER(&CsrBtVscLto, "BT_VSC", 0, NULL)
#define VSC_INFO(...)               CSR_LOG_TEXT_INFO((CsrBtVscLto, 0, __VA_ARGS__))
#define VSC_ERROR(...)              CSR_LOG_TEXT_ERROR((CsrBtVscLto, 0, __VA_ARGS__))
#define VSC_PANIC(...)              {CsrPanic(CSR_TECH_BT, CSR_BT_PANIC_MYSTERY, __VA_ARGS__);}

#define VSC_UNLOCK_QUEUE(_vsc) _vsc->vscVar.lockMsg = VSC_QUEUE_UNLOCK;
#define VSC_LOCK_QUEUE(_vsc) _vsc.vscVar.lockMsg = VSC_QUEUE_LOCK;
#define VSC_PUT_MESSAGE(phandle, msg) {CsrSchedMessagePut(phandle, BT_VSDM_PRIM, (void *)msg);}


typedef struct VscApplicationInstanceElementTag
{
    struct VscApplicationInstanceElementTag    *next;
    struct VscApplicationInstanceElementTag    *prev;
    CsrSchedQid                                handle;
}VscApplicationInstanceElement;


typedef struct
{
    CsrMessageQueueType     *saveQueue;
    CsrUint16               lockMsg;
    CsrSchedQid             appHandle;
    CsrBdAddr               bdAddr;
}vscVariables;


typedef struct
{
    void                    *recvMsgP;
    vscVariables            vscVar;
    uint8                   len;
    CsrCmnList_t            InstanceList;
}vscInstanceData_t;

#define VSC_ADD_APP_INSTANCE(_List) \
    (VscApplicationInstanceElement *)CsrCmnListElementAddLast(&(_List), \
                                                                 sizeof(VscApplicationInstanceElement))

#define VSC_FIND_HANDLE(_List,_handle) \
    ((VscApplicationInstanceElement *)CsrCmnListSearch(&(_List), \
                                                          VscFindHandle, \
                                                        (void *)(_handle)))

#define VSC_REMOVE_INSTANCE(_List,_Elem) \
    (CsrCmnListElementRemove((CsrCmnList_t *)&(_List), \
                             (CsrCmnListElm_t *)(_Elem)))

CsrBool VscFindHandle(CsrCmnListElm_t *elem, void *value);

extern vscInstanceData_t vscBtData;

void VscInit(void **gash);
void VscHandler(void **gash);

void VscInitInstData(vscInstanceData_t *vscData);

void VscLockQueue(vscInstanceData_t *vscData);
void VscUnlockQueue(vscInstanceData_t *vscData);
void VscLocalQueueHandler(void);
void VscRestoreQueueHandler(vscInstanceData_t *vscData);

void VscPutMessage(CsrSchedQid phandle, void *msg);
void VscProfileProvider(vscInstanceData_t *vscData);
void VscArrivalHandler(vscInstanceData_t  *vscData);

void VscSendRegisterReq(vscInstanceData_t *vscData);
void VscSendReadLocalQlmSuppFeaturesReq(vscInstanceData_t *vscData);
void VscSendReadRemoteQlmSuppFeaturesReq(vscInstanceData_t *vscData);
void VscSendWriteScHostSuppOverridReq(vscInstanceData_t *vscData);
void VscSendReadScHostSuppOverrideReq(vscInstanceData_t *vscData);
void VscSendWriteScHostSuppCodOverridReq(vscInstanceData_t *vscData);
void VscSendReadScHostSuppCodOverrideReq(vscInstanceData_t *vscData);
void VscSendSetQhsHostModeReq(vscInstanceData_t *vscData);
void VscSendSetWbmFeaturesReq(vscInstanceData_t *vscData);
void VscSendConvertRpaToIaReq(vscInstanceData_t *vscData);
void VscSetStreamingModeReq(vscInstanceData_t *vscData);
void VscSendReadRemoteQllSuppFeaturesReq(vscInstanceData_t *vscData);
void VscRestoreQueueHandler(vscInstanceData_t *vscData);

/* Arrival Handler function prototypes */
void VscQlmConnectionCompleteIndHandler(vscInstanceData_t *vscData);
void VscQcmPhyChangeIndHandler(vscInstanceData_t *vscData);
void VscIncomingPageIndHandler(vscInstanceData_t *vscData);
void VscQllConnectionCompleteIndHandler(vscInstanceData_t *vscData);

void VscHciCfmHandler(vscInstanceData_t *vscData);
void VscRegisterCfmHandler(vscInstanceData_t *vscData);
void VscReadLocalQlmSuppFeaturesCfmHandler(vscInstanceData_t *vscData);
void VscReadRemoteQlmSuppFeaturesCfmHandler(vscInstanceData_t *vscData);
void VscWriteScHostSuppOverrideCfmHandler(vscInstanceData_t *vscData);
void VscReadScHostSuppOverrideCfmHandler(vscInstanceData_t *vscData);
void VscWriteScHostSuppCodOverrideCfmHandler(vscInstanceData_t *vscData);
void VscReadScHostSuppCodOverrideCfmHandler(vscInstanceData_t *vscData);
void VscSetQhsHostModeCfmHandler(vscInstanceData_t *vscData);
void VscSetWbmFeaturesCfmHandler(vscInstanceData_t *vscData);
void VscConvertRpaToIaCfmHandler(vscInstanceData_t *vscData);
void VscWriteTruncatedPageScanEnableCfmHandler(vscInstanceData_t *vscData);
void VscSetStreamingModeCfmHandler(vscInstanceData_t *vscData);
void VscReadRemoteQllSuppFeaturesCfmHandler(vscInstanceData_t *vscData);

typedef void (* SignalHandlerType)(vscInstanceData_t * taskData);

#endif /* VSC_MAIN_H */
