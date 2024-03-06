/* Copyright (c) 2021-2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include <gatt_telephone_bearer_client.h>

#include "ccp_private.h"
#include "ccp_msg_handler.h"
#include "ccp_debug.h"
#include "ccp_init.h"
#include "ccp_destroy.h"
#include "ccp_indication.h"
#include "ccp_read.h"
#include "ccp_write.h"
#include "ccp_notification.h"
#include "ccp_common.h"
#include "gatt_service_discovery_lib.h"


CsrBool ccpInstFindBySrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle profile_handle = *(ServiceHandle *)data;

    return (profile_hndl_elm->profile_handle == profile_handle);
}

CsrBool ccpProfileHndlFindByBtConnId(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    CsrBtConnId     btConnId   = *(CsrBtConnId *) data;
    CCP *ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if(ccp_inst == NULL)
        return FALSE;

    return (ccp_inst->cid == btConnId);
}

CsrBool ccpProfileHndlFindByTbsSrvcHndl(CsrCmnListElm_t *elem, void *data)
{
    ProfileHandleListElm_t *profile_hndl_elm = (ProfileHandleListElm_t *)elem;
    ServiceHandle tbs_srvc_hndl = *(ServiceHandle *)data;
    CCP *ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(profile_hndl_elm->profile_handle);

    if(ccp_inst == NULL)
        return FALSE;

    return (ccp_inst->tbs_srvc_hdl == tbs_srvc_hndl);
}



/*************************************************************/
static void ccpHandleGattTbsClientMsg(ccp_main_inst *inst, void *msg)
{
    CCP *ccp_inst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattTelephoneBearerClientMessageId *prim = (GattTelephoneBearerClientMessageId *)msg;

    switch (*prim)
    {
        case GATT_TELEPHONE_BEARER_CLIENT_INIT_CFM:
        {
            const GattTelephoneBearerClientInitCfm* message;
            message = (GattTelephoneBearerClientInitCfm*) msg;
            /* Find ccp instance using connection_id_t */
            elem = CCP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profile_handle_list,
                                                       message->cid);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if (ccp_inst)
                {
                    ccpHandleTbsClientInitResp(ccp_inst,
                                              (const GattTelephoneBearerClientInitCfm *)msg);
                }
            }
        }
        break;

        case GATT_TELEPHONE_BEARER_CLIENT_TERMINATE_CFM:
        {
            const GattTelephoneBearerClientTerminateCfm* message;
            message = (GattTelephoneBearerClientTerminateCfm*) msg;

            /* Find ccp instance using connection_id_t */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->srvcHndl);
            if(elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsClientTerminateResp(ccp_inst,
                                                   (const GattTelephoneBearerClientTerminateCfm *)msg);
            }

            CCP_REMOVE_SERVICE_HANDLE(inst->profile_handle_list, elem->profile_handle);
        }
        break;

        /* Characteristic Read Confirmation messages */
        case GATT_TELEPHONE_BEARER_CLIENT_READ_PROVIDER_NAME_CFM:
        {
            const GattTelephoneBearerClientReadProviderNameCfm* message;
            message = (GattTelephoneBearerClientReadProviderNameCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);
                if(ccp_inst)
                    ccpHandleTbsReadProviderNameCfm(ccp_inst,
                                                   (const GattTelephoneBearerClientReadProviderNameCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_UCI_CFM:
        {
            const GattTelephoneBearerClientReadBearerUciCfm* message;
            message = (GattTelephoneBearerClientReadBearerUciCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadBearerUciCfm(ccp_inst,
                                                (const GattTelephoneBearerClientReadBearerUciCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_TECHNOLOGY_CFM:
        {
            const GattTelephoneBearerClientReadBearerTechnologyCfm* message;
            message = (GattTelephoneBearerClientReadBearerTechnologyCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadBearerTechnologyCfm(ccp_inst,
                                                   (const GattTelephoneBearerClientReadBearerTechnologyCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_BEARER_URI_SCHEMES_SUPPORTED_LIST_CFM:
        {
            const GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm* message;
            message = (GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                   ccpHandleTbsReadBearerUriSchemesCfm(ccp_inst,
                                                      (const GattTelephoneBearerClientReadBearerUriSchemesSupportedListCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_CFM:
        {
            const GattTelephoneBearerClientReadSignalStrengthCfm* message;
            message = (GattTelephoneBearerClientReadSignalStrengthCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadSignalStrengthCfm(ccp_inst,
                                          (const GattTelephoneBearerClientReadSignalStrengthCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_SIGNAL_STRENGTH_INTERVAL_CFM:
        {
            const GattTelephoneBearerClientReadSignalStrengthIntervalCfm* message;
            message = (GattTelephoneBearerClientReadSignalStrengthIntervalCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadSignalStrengthIntervalCfm(ccp_inst,
                                          (const GattTelephoneBearerClientReadSignalStrengthIntervalCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_CURRENT_CALLS_LIST_CFM:
        {
            const GattTelephoneBearerClientReadCurrentCallsListCfm* message;
            message = (GattTelephoneBearerClientReadCurrentCallsListCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadCurrentCallsListCfm(ccp_inst,
                                          (const GattTelephoneBearerClientReadCurrentCallsListCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_CONTENT_CONTROL_ID_CFM:
        {
            const GattTelephoneBearerClientReadContentControlIdCfm* message;
            message = (GattTelephoneBearerClientReadContentControlIdCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadContentControlIdCfm(ccp_inst,
                                          (const GattTelephoneBearerClientReadContentControlIdCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_FEATURE_AND_STATUS_FLAGS_CFM:
        {
            const GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm* message;
            message = (GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadFlagsCfm(ccp_inst,
                                          (const GattTelephoneBearerClientReadFeatureAndStatusFlagsCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_TARGET_BEARER_URI_CFM:
        {
            const GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm* message;
            message = (GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadIncomingCallTargetBearerUriCfm(ccp_inst,
                                          (const GattTelephoneBearerClientReadIncomingCallTargetBearerUriCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_STATE_CFM:
        {
            const GattTelephoneBearerClientMsgReadCallStateCfm* message;
            message = (GattTelephoneBearerClientMsgReadCallStateCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadCallStateCfm(ccp_inst,
                                          (const GattTelephoneBearerClientMsgReadCallStateCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_INCOMING_CALL_CFM:
        {
            const GattTelephoneBearerClientMsgReadIncomingCallCfm* message;
            message = (GattTelephoneBearerClientMsgReadIncomingCallCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadIncomingCallCfm(ccp_inst,
                                          (const GattTelephoneBearerClientMsgReadIncomingCallCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_CALL_FRIENDLY_NAME_CFM:
        {
            const GattTelephoneBearerClientMsgReadCallFriendlyNameCfm* message;
            message = (GattTelephoneBearerClientMsgReadCallFriendlyNameCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadCallFriendlyNameCfm(ccp_inst,
                                          (const GattTelephoneBearerClientMsgReadCallFriendlyNameCfm*)msg);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_READ_CCP_OPTIONAL_OPCODES_CFM:
        {
            const GattTelephoneBearerClientReadOptionalOpcodesCfm* message;
            message = (GattTelephoneBearerClientReadOptionalOpcodesCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsReadOptionalOpcodesCfm(ccp_inst,
                                          (const GattTelephoneBearerClientReadOptionalOpcodesCfm*)msg);
            }
        }
        break;

        /* Characteristic Notification Confirmation messages */
        case GATT_TELEPHONE_BEARER_CLIENT_PROVIDER_NAME_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientProviderNameSetNotificationCfm* message;
            message = (GattTelephoneBearerClientProviderNameSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_PROVIDER_NAME_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_TECHNOLOGY_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientTechnologySetNotificationCfm* message;
            message = (GattTelephoneBearerClientTechnologySetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_TECHNOLOGY_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientSignalStrengthSetNotificationCfm* message;
            message = (GattTelephoneBearerClientSignalStrengthSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_SIGNAL_STRENGTH_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CURRENT_CALLS_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientCurrentCallsSetNotificationCfm* message;
            message = (GattTelephoneBearerClientCurrentCallsSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_CURRENT_CALLS_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_FLAGS_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientFlagsSetNotificationCfm* message;
            message = (GattTelephoneBearerClientFlagsSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_FLAGS_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientIncomingCallTargetBearerUriSetNotificationCfm* message;
            message = (GattTelephoneBearerClientIncomingCallTargetBearerUriSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_INCOMING_CALL_TARGET_BEARER_URI_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CALL_STATE_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientCallStateSetNotificationCfm* message;
            message = (GattTelephoneBearerClientCallStateSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_CALL_STATE_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientCallControlPointSetNotificationCfm* message;
            message = (GattTelephoneBearerClientCallControlPointSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_CALL_CONTROL_POINT_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_TERMINATION_REASON_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientTerminationReasonSetNotificationCfm* message;
            message = (GattTelephoneBearerClientTerminationReasonSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_TERMINATION_REASON_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientIncomingCallSetNotificationCfm* message;
            message = (GattTelephoneBearerClientIncomingCallSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_INCOMING_CALL_SET_NOTIFICATION_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM:
        {
            const GattTelephoneBearerClientCallFriendlyNameSetNotificationCfm* message;
            message = (GattTelephoneBearerClientCallFriendlyNameSetNotificationCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                CCP_CALL_FRIENDLY_NAME_SET_NOTIFICATION_CFM);
            }
        }
        break;

        /* Write Characteristic CFMs */
        case GATT_TELEPHONE_BEARER_CLIENT_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM:
        {
            const GattTelephoneBearerClientWriteSignalStrengthIntervalCfm* message;
            message = (GattTelephoneBearerClientWriteSignalStrengthIntervalCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                 CCP_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_WRITE_CALL_CONTROL_POINT_CFM:
        {
            const GattTelephoneBearerClientWriteCallControlPointCfm* message;
            message = (GattTelephoneBearerClientWriteCallControlPointCfm*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpSendTbsSetNtfCfm(ccp_inst, message->status,
                                 CCP_WRITE_CALL_CONTROL_POINT_CFM);
            }
        }
        break;

        /* Characteristic Indication messages */
        case GATT_TELEPHONE_BEARER_CLIENT_PROVIDER_NAME_IND:
        {
            const GattTelephoneBearerClientProviderNameInd* message;
            message = (GattTelephoneBearerClientProviderNameInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsProviderNameInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_BEARER_TECHNOLOGY_IND:
        {
            const GattTelephoneBearerClientBearerTechnologyInd* message;
            message = (GattTelephoneBearerClientBearerTechnologyInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsBearerTechnologyInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_SIGNAL_STRENGTH_IND:
        {
            const GattTelephoneBearerClientSignalStrengthInd* message;
            message = (GattTelephoneBearerClientSignalStrengthInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsSignalStrengthInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CURRENT_CALLS_IND:
        {
            const GattTelephoneBearerClientCurrentCallsInd* message;
            message = (GattTelephoneBearerClientCurrentCallsInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsCurrentCallsInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_FLAGS_IND:
        {
            const GattTelephoneBearerClientFlagsInd* message;
            message = (GattTelephoneBearerClientFlagsInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsFlagsInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_TARGET_BEARER_URI_IND:
        {
            const GattTelephoneBearerClientIncomingCallTargetBearerUriInd* message;
            message = (GattTelephoneBearerClientIncomingCallTargetBearerUriInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsIncomingCallTargetBearerUriInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CALL_STATE_IND:
        {
            const GattTelephoneBearerClientCallStateInd* message;
            message = (GattTelephoneBearerClientCallStateInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsCallStateInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CALL_CONTROL_POINT_IND:
        {
            const GattTelephoneBearerClientCallControlPointInd* message;
            message = (GattTelephoneBearerClientCallControlPointInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsCallControlPointInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_TERMINATION_REASON_IND:
        {
            const GattTelephoneBearerClientTerminationReasonInd* message;
            message = (GattTelephoneBearerClientTerminationReasonInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsTerminationReasonInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_INCOMING_CALL_IND:
        {
            const GattTelephoneBearerClientIncomingCallInd* message;
            message = (GattTelephoneBearerClientIncomingCallInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsIncomingCallInd(ccp_inst, message);
            }
        }
        break;
        case GATT_TELEPHONE_BEARER_CLIENT_CALL_FRIENDLY_NAME_IND:
        {
            const GattTelephoneBearerClientCallFriendlyNameInd* message;
            message = (GattTelephoneBearerClientCallFriendlyNameInd*) msg;

            /* Find ccp instance using tbs service handle */
            elem = CCP_FIND_PROFILE_HANDLE_BY_TBS_SERVICE_HANDLE(inst->profile_handle_list,
                                                                 message->tbsHandle);
            if (elem)
            {
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

                if(ccp_inst)
                    ccpHandleTbsCallFriendlyNameInd(ccp_inst, message);
            }
        }
        break;

        default:
        {
            /* Unrecognised GATT TBS Client message */
            CCP_WARNING("Gatt TBS Client Msg not handled [0x%x]\n", *prim);
        }
        break;
    }
}


/***************************************************************************/
static void  ccpHandleInternalMessage(ccp_main_inst *inst, void *msg)
{
    CCP * ccp_inst = NULL;
    ccp_internal_msg_t *prim = (ccp_internal_msg_t *)msg;

    if (inst)
    {
        switch(*prim)
        {
            /* handle messages*/
            case CCP_INTERNAL_WRITE_SIGNAL_STRENGTH_INTERVAL_CFM:
            break;

            case CCP_INTERNAL_WRITE_CALL_CONTROL_POINT_CFM:
            {
                CcpInternalWriteCallControlPointCfm *message = (CcpInternalWriteCallControlPointCfm *) msg;

                ccp_inst = FIND_CCP_INST_BY_SERVICE_HANDLE(message->prfl_hndl);
                if(ccp_inst)
                {
                    ccp_inst->pending_op = 0;

                    ccpTbsControlPointOp(message->prfl_hndl,
                                         message);
                }
            }
            break;

            default:
            {
                /* Internal unrecognised messages */
                CCP_WARNING("CCP Unknown Internal Lib Message\n");
            }
            break;
        }
    }
}


/****************************************************************************/
static void handleGattSrvcDiscMsg(ccp_main_inst *inst, void *msg)
{
    CCP *ccp_inst = NULL;
    ProfileHandleListElm_t* elem = NULL;
    GattSdPrim* prim = (GattSdPrim*)msg;

    switch (*prim)
    {
        case GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM:
        {
            GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *cfm =
                (GATT_SERVICE_DISCOVERY_FIND_SERVICE_RANGE_CFM_T *) msg;

            elem = CCP_FIND_PROFILE_HANDLE_BY_BTCONNID(inst->profile_handle_list, cfm->cid);
            if (elem)
                ccp_inst = FIND_CCP_INST_BY_PROFILE_HANDLE(elem->profile_handle);

            if (ccp_inst == NULL)
                return;

            if ((cfm->result == GATT_SD_RESULT_SUCCESS) && cfm->srvcInfoCount)
            {
                GattTelephoneBearerClientInitData init_data;
                CCP_DEBUG("(TBS) : Start Hndl = 0x%x, End Hndl = 0x%x, Id = 0x%x\n",
                    cfm->srvcInfo[0].startHandle, cfm->srvcInfo[0].endHandle, cfm->srvcInfo[0].srvcId);

                init_data.cid = cfm->cid;
                init_data.startHandle = cfm->srvcInfo[0].startHandle;
                init_data.endHandle = cfm->srvcInfo[0].endHandle;

                ccp_inst->gtbs_start_handle = cfm->srvcInfo[0].startHandle;
                ccp_inst->gtbs_end_handle = cfm->srvcInfo[0].endHandle;

                GattTelephoneBearerClientInit(ccp_inst->lib_task, &init_data, NULL);
                free(cfm->srvcInfo);
            }
            else
            {
                ccpSendInitCfm(ccp_inst, CCP_STATUS_DISCOVERY_ERR);
                CCP_REMOVE_SERVICE_HANDLE(inst->profile_handle_list, ccp_inst->ccp_srvc_hdl);
                FREE_CCP_CLIENT_INST(ccp_inst->ccp_srvc_hdl);
            }
            break;
        }

        default:
        {
            /* Unrecognised GATT Manager message */
            CCP_DEBUG("CCP:Gatt SD Msg not handled\n");
        }
        break;
    }
}


/****************************************************************************/
void ccpMsgHandler(void **gash)
{
    CsrUint16 eventClass = 0;
    void *msg = NULL;
    ccp_main_inst *inst = (ccp_main_inst * )*gash;

    if (CsrSchedMessageGet(&eventClass, &msg))
    {
        switch (eventClass)
        {
            case GATT_SRVC_DISC_PRIM:
                handleGattSrvcDiscMsg(inst, msg);
                break;
            case CCP_PRIM:
                ccpHandleInternalMessage(inst, msg);
                break;
            case TBS_CLIENT_PRIM:
                ccpHandleGattTbsClientMsg(inst, msg);
                break;
            default:
                CCP_WARNING("CCP Profile Msg not handled \n");
        }

        SynergyMessageFree(eventClass, msg);
    }
}

