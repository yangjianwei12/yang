/******************************************************************************
 Copyright (c) 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.
 
 REVISION:      $Revision: #57 $
******************************************************************************/

#include "chp_seeker_private.h"
#include "chp_seeker_debug.h"
#include "chp_seeker_init.h"
#include "chp_seeker_destroy.h"
#include "chp_seeker_indication.h"
#include "chp_seeker_read.h"
#include "chp_seeker_write.h"
#include "gatt_service_discovery_lib.h"

CsrBool chpSeekerInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle profile_handle = *(ServiceHandle *)data;

    if (profile_hndl_elm)
        return (profile_hndl_elm->profile_handle == profile_handle);

    return FALSE;
}

CsrBool chpSeekerProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    CHP *chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if (chpSeekerInst)
        return (chpSeekerInst->cid == btConnId);

    return FALSE;
}

CsrBool chpSeekerProfileHndlFindByTdsSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle tdsSrvcHndl = *(ServiceHandle *)data;
    CHP *chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if (chpSeekerInst)
        return (chpSeekerInst->tdsSrvcHndl == tdsSrvcHndl);

    return FALSE;
}

/****************************************************************************/
static void handleGattSrvcDiscMsg(ChpSeekerMainInst *inst, Msg *msg)
{
    CHP *chpSeekerInst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = FIND_CHP_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList, cfm->cid);
            if (elem)
                chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (chpSeekerInst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                uint16 index, count;

                count = cfm->srvcInfoCount;

                for (index = 0; index < count; index++)
                {
                    GattTdsClientInitData initData;
                    CHP_INFO("(CHP) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                              cfm->srvcInfo[0].startHandle, cfm->srvcInfo[0].endHandle, cfm->srvcInfo[0].srvcId);

                    initData.cid = cfm->cid;
                    initData.startHandle = cfm->srvcInfo[index].startHandle;
                    initData.endHandle = cfm->srvcInfo[index].endHandle;

                    GattTdsClientInitReq(chpSeekerInst->libTask, &initData, NULL);
                }
                free(cfm->srvcInfo);
            }
            else
            {
                chpSeekerSendInitCfm(chpSeekerInst, CHP_SEEKER_STATUS_DISCOVERY_ERR);
                REMOVE_CHP_SERVICE_HANDLE(inst->profileHandleList, chpSeekerInst->chpSeekerSrvcHndl);
                FREE_CHP_INST(chpSeekerInst->chpSeekerSrvcHndl);
            }
            break;
        }

        default:
        {
            /* Unrecognised GATT Manager message */
            CHP_WARNING("Gatt SD Msg not handled \n");
        }
        break;
    }
}

/*************************************************************/
static void chpSeekerHandleGattTdsClientMsg(ChpSeekerMainInst *inst, void *msg)
{
    CHP * chpSeekerInst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattTdsClientMessageId *prim = (GattTdsClientMessageId *)msg;

    switch (*prim)
    {
        case GATT_TDS_CLIENT_INIT_CFM:
        {
            const GattTdsClientInitCfm* message;
            message = (GattTdsClientInitCfm*) msg;

            /* Find CHP instance using connection_id_t */
            elem = FIND_CHP_PROFILE_HANDLE_BY_BTCONNID(inst->profileHandleList,
                                                       message->cid);
            if (elem)
                chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (chpSeekerInst)
                chpSeekerHandleTdsClientInitResp(chpSeekerInst,
                                                (const GattTdsClientInitCfm *)msg);
        }
        break;

        case GATT_TDS_CLIENT_TERMINATE_CFM:
        {
            const GattTdsClientTerminateCfm* message;
            message = (GattTdsClientTerminateCfm*) msg;

            /* Find CHP instance using connection_id_t */
            elem = FIND_CHP_PROFILE_HANDLE_BY_TDS_SERVICE_HANDLE(inst->profileHandleList,
                                                                 message->srvcHndl);
            if (elem)
                chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (chpSeekerInst)
                chpSeekerHandleTdsClientTerminateResp(chpSeekerInst,
                                                     (const GattTdsClientTerminateCfm *)msg);
        }
        break;

        case GATT_TDS_CLIENT_INDICATION_CFM:
        {
            const GattTdsClientIndicationCfm* message;
            message = (GattTdsClientIndicationCfm*) msg;

            /* Find CHP instance using connection_id_t */
            elem = FIND_CHP_PROFILE_HANDLE_BY_TDS_SERVICE_HANDLE(inst->profileHandleList,
                                                                 message->srvcHndl);
            if (elem)
                chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (chpSeekerInst)
                chpSeekerHandleTdsClientIndResp(chpSeekerInst,
                                               (const GattTdsClientIndicationCfm *)msg);
        }
        break;

        case GATT_TDS_SET_TDS_CONTROL_POINT_CFM:
        {
            const GattTdsClientSetTdsControlPointCfm* message;
            message = (GattTdsClientSetTdsControlPointCfm*) msg;

            /* Find CHP instance using connection_id_t */
            elem = FIND_CHP_PROFILE_HANDLE_BY_TDS_SERVICE_HANDLE(inst->profileHandleList,
                                                                 message->srvcHndl);
            if (elem)
                chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (chpSeekerInst)
                chpSeekerHandleTdsClientSetTdsCPResp(chpSeekerInst,
                                                    (const GattTdsClientSetTdsControlPointCfm *)msg);
        }
        break;

        case GATT_TDS_CLIENT_GET_TDS_ATTRIBUTE_CFM:
        {
            const GattTdsClientGetTdsAttributeCfm* message;
            message = (GattTdsClientGetTdsAttributeCfm*) msg;

            /* Find CHP instance using connection_id_t */
            elem = FIND_CHP_PROFILE_HANDLE_BY_TDS_SERVICE_HANDLE(inst->profileHandleList,
                                                                 message->srvcHndl);
            if (elem)
                chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (chpSeekerInst)
                chpSeekerHandleTdsClientGetTdsAttributeResp(chpSeekerInst,
                                                           (const GattTdsClientGetTdsAttributeCfm *)msg);
        }
        break;

        case GATT_TDS_CLIENT_CONTROL_POINT_IND:
        {
            const GattTdsClientTdsCPAttributeInd* message;
            message = (GattTdsClientTdsCPAttributeInd*) msg;

            /* Find CHP instance using connection_id_t */
            elem = FIND_CHP_PROFILE_HANDLE_BY_TDS_SERVICE_HANDLE(inst->profileHandleList,
                                                                 message->srvcHndl);
            if (elem)
                chpSeekerInst = FIND_CHP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (chpSeekerInst)
                chpSeekerHandleTdsClientControlPointInd(chpSeekerInst,
                                                       (const GattTdsClientTdsCPAttributeInd *)msg);
        }
        break;

        default:
        {
            /* Unrecognised GATT TDS Client message */
            CHP_WARNING("Gatt TDS Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}


/***************************************************************************/
static void  chpSeekerHandleInternalMessage(ChpSeekerMainInst *inst, void *msg)
{
    CHP_INFO("chpSeekerHandleInternalMessage Message \n");
    CSR_UNUSED(inst);
    CSR_UNUSED(msg);
}

/****************************************************************************/
void chpSeekerMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    ChpSeekerMainInst *inst = (ChpSeekerMainInst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                handleGattSrvcDiscMsg(inst, msg);
                break;
            case CHP_SEEKER_PRIM:
                chpSeekerHandleInternalMessage(inst, msg);
                break;
            case TDS_CLIENT_PRIM:
                chpSeekerHandleGattTdsClientMsg(inst, msg);
                break;
            default:
                CHP_WARNING("Profile Msg not handled \n");
        }
        SynergyMessageFree(eventClass, msg);
    }
}
