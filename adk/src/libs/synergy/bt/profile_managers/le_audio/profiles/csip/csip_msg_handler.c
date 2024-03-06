/* Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "gatt_csis_client.h"

#include "csip_private.h"
#include "csip_debug.h"
#include "csip_init.h"
#include "csip_destroy.h"
#include "csip_indication.h"
#include "csip_read.h"
#include "csip_write.h"
#include "csip_notification.h"
#include "gatt_service_discovery_lib.h"
#include "csr_bt_gatt_client_util_lib.h"

CsrBool csipInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profileHndlElm = (ProfileHandleListElm_t *)elem;
    ServiceHandle profileHandle = *(ServiceHandle *)data;

    return (profileHndlElm->profileHandle == profileHandle);
}

CsrBool csipProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profileHndlElm = (ProfileHandleListElm_t *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    Csip *csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);

    return csipInst && (csipInst->cid == btConnId);
}

CsrBool csipProfileHndlFindByCsisSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profileHndlElm = (ProfileHandleListElm_t *)elem;
    ServiceHandle csisSrvcHndl = *(ServiceHandle *)data;
    Csip *csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(profileHndlElm->profileHandle);

    return csipInst && (csipInst->csisSrvcHdl == csisSrvcHndl);
}

/****************************************************************************/
static void handleCsipGattSrvcDiscMsg(CsipMainInst *inst, void *msg)
{
    Csip *csipInst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = CSIP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList, cfm->cid);
            if (elem)
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (csipInst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                GattCsisClientInitParams initData;
                CSIP_DEBUG("(CSIP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                    cfm->srvcInfo[0].startHandle, cfm->srvcInfo[0].endHandle, cfm->srvcInfo[0].srvcId);

                initData.cid = cfm->cid;
                initData.startHandle = cfm->srvcInfo[0].startHandle;
                initData.endHandle = cfm->srvcInfo[0].endHandle;

                csipInst->startHandle = cfm->srvcInfo[0].startHandle;
                csipInst->endHandle = cfm->srvcInfo[0].endHandle;

                GattCsisClientInit(csipInst->lib_task, &initData, NULL);
            }
            else
            {
                csipSendInitCfm(csipInst, CSIP_STATUS_DISCOVERY_ERR);
                CSIP_REMOVE_SERVICE_HANDLE(inst->profileHandleList, csipInst->csipSrvcHdl);
                FREE_CSIP_CLIENT_INST(csipInst->csipSrvcHdl);
            }

            if (cfm->srvcInfoCount && cfm->srvcInfo)
            {
                CsrPmemFree(cfm->srvcInfo);
                cfm->srvcInfo = NULL;
            }
        }
        break;

        default:
        {
            /* Unrecognised GATT Manager message */
            CSIP_WARNING("Gatt SD Msg not handled \n");
        }
        break;
    }
}

/*************************************************************/
static void csipHandleGattCsisClientMsg(CsipMainInst *inst, void *msg)
{
    Csip * csipInst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattCsisMessageId *prim = (GattCsisMessageId *)msg;

    CSIP_INFO("csipHandleGattCsisClientMsg MESSAGE:GattCsisMessageId:0x%x", *prim);

    switch (*prim)
    {
        case GATT_CSIS_CLIENT_INIT_CFM:
        {
            const GattCsisClientInitCfm* message;
            message = (GattCsisClientInitCfm*) msg;

            /* Find csip instance using connection_id_t */
            elem = CSIP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList,
                                                       message->cid);
            if (elem)
            {
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }

            if (csipInst)
            {
                csipHandleCsisClientInitResp(csipInst,
                                       (const GattCsisClientInitCfm *)msg);
            }
        }
        break;

        case GATT_CSIS_CLIENT_TERMINATE_CFM:
        {
            const GattCsisClientTerminateCfm* message;
            message = (GattCsisClientTerminateCfm*) msg;
            /* Find csip instance using connection_id_t */
            elem = CSIP_FIND_PROFILE_HANDLE_BY_CSIS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
            {
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }

            if (csipInst)
            {
                csipHandleCsisClientTerminateResp(csipInst,
                                            (const GattCsisClientTerminateCfm *)msg);
            }

            CSIP_REMOVE_SERVICE_HANDLE(inst->profileHandleList, elem->profileHandle);
        }
        break;

        case GATT_CSIS_CLIENT_NOTIFICATION_CFM:
        {
            const GattCsisClientNotificationCfm* message;
            message = (GattCsisClientNotificationCfm*) msg;

            /* Find csip instance using csis service handle */
            elem = CSIP_FIND_PROFILE_HANDLE_BY_CSIS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
            {
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }

            if (csipInst)
            {
                csipSendCsisSetNtfCfm(csipInst, message->status);
            }
        }
        break;

        case GATT_CSIS_CLIENT_READ_CS_INFO_CFM:
        {
            const GattCsisClientReadCsInfoCfm* message;
            message = (GattCsisClientReadCsInfoCfm*) msg;

            /* Find csip instance using csis service handle */
            elem = CSIP_FIND_PROFILE_HANDLE_BY_CSIS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
            {
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }

            if (csipInst)
            {
                csipSendReadCSInfoCfm(csipInst, message->status, message->csInfo,
                                      message->sizeValue, &message->value[0]);
            }
        }
        break;


        case GATT_CSIS_CLIENT_WRITE_LOCK_CFM:
        {
            const GattCsisClientWriteLockCfm* message;
            message = (GattCsisClientWriteLockCfm*) msg;

            /* Find csip instance using csis service handle */
            elem = CSIP_FIND_PROFILE_HANDLE_BY_CSIS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
            {
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }

            if (csipInst)
            {
                csipSendSetLockCfm(csipInst, message->status);
            }
        }
        break;

        case GATT_CSIS_CLIENT_NOTIFICATION_IND:
        {
            const GattCsisClientNotificationInd* message;
            message = (GattCsisClientNotificationInd*) msg;

            /* Find csip instance using csis service handle */
            elem = CSIP_FIND_PROFILE_HANDLE_BY_CSIS_SERVICE_HANDLE(inst->profileHandleList,
                                                       message->srvcHndl);
            if (elem)
            {
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);
            }

            if (csipInst)
            {
                if(message->csInfo == GATT_CSIS_CLIENT_SIRK)
                    csipHandleSirkChangedInd(csipInst, (uint8 *)&message->value[0]);
                else if (message->csInfo == GATT_CSIS_CLIENT_SIZE)
                    csipHandleSizeChangedInd(csipInst, message->value[0]);
                else if (message->csInfo == GATT_CSIS_CLIENT_LOCK)
                    csipHandleLockStatusInd(csipInst, message->value[0]);
            }
        }
        break;

        default:
        {
            /* Unrecognised GATT CSIS Client message */
            CSIP_WARNING("Gatt CSIP Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}



/***************************************************************************/
static void  csipHandleInternalMessage(CsipMainInst *inst, void *msg)
{
    CSR_UNUSED(inst);
    CSR_UNUSED(msg);
}


static void csipHandleDecryptSirkCfm(Csip *csipInst,
                                     CsrBtResultCode status,
                                     uint8 *value)
{
    CsipSirkDecryptCfm *message = 
        (CsipSirkDecryptCfm *) CsrPmemZalloc(sizeof(CsipSirkDecryptCfm));

    message->id = CSIP_SIRK_DECRYPT_CFM;
    message->prflHndl = csipInst->csipSrvcHdl;
    message->status = ((status == CSR_BT_RESULT_CODE_CM_SUCCESS) ?
                   CSIP_STATUS_SUCCESS : CSIP_STATUS_FAILED);

    if (value)
    {
        CsrMemCpy(&message->sirkValue[0], value, CSIP_SIRK_SIZE * sizeof(uint8));
    }

    CsipMessageSend(csipInst->app_task, message);
}

/****************************************************************************/
static void csipHandleCmMsg(CsipMainInst *inst, Msg msg)
{
    Csip * csipInst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    CsrBtCmPrim *prim = (CsrBtCmPrim *)msg;

    switch (*prim)
    {
        /* Handle CM related primitives here */
        case CSR_BT_CM_LE_SIRK_OPERATION_CFM:
        {
            CsrBtCmLeSirkOperationCfm* cfm;
            CsrBtTypedAddr typedAddr;
            CsrBtConnId cid;

            cfm = (CsrBtCmLeSirkOperationCfm*) msg;

            CsrBtAddrCopy(&(typedAddr.addr), &(cfm->tpAddrt.addrt.addr));

            typedAddr.type = cfm->tpAddrt.addrt.type;
            cid = CsrBtGattClientUtilFindConnIdByAddr(&typedAddr);

            CSIP_DEBUG(" CSR_BT_CM_LE_SIRK_OPERATION_CFM Status %d\n", cfm->resultCode);

            /* Find csip instance using connection_id_t */
            elem = CSIP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList,
                                                       cid);
            if (elem)
                csipInst = FIND_CSIP_INST_BY_PROFILE_HANDLE(elem->profileHandle);

            if (csipInst)
               csipHandleDecryptSirkCfm(csipInst, cfm->resultCode, cfm->sirkKey);
        }
        break;

         default:
        break;

    }
}


/****************************************************************************/
void csipMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    CsipMainInst *inst = (CsipMainInst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                handleCsipGattSrvcDiscMsg(inst, msg);
                break;
            case CSIP_PRIM:
                csipHandleInternalMessage(inst, msg);
                break;
            case CSIS_CLIENT_PRIM:
                csipHandleGattCsisClientMsg(inst, msg);
                break;
            case CSR_BT_CM_PRIM:
                csipHandleCmMsg(inst, msg);
                break;

            default:
                CSIP_WARNING("Profile Msg not handled \n");
        }

        SynergyMessageFree(eventClass, msg);
    }
}

