/*!
    \copyright  Copyright (c) 2015 - 2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version        
    \file       bredr_scan_manager.c
    \ingroup    bredr_scan_manager
    \brief	    Implementation of module managing inquiry and page scanning.
*/
#include "bredr_scan_manager_private.h"
#include "bredr_scan_manager.h"
#include "qualcomm_connection_manager.h"

#include <connection_abstraction.h>
#include <stdlib.h>
#include <logging.h>

#include "sdp.h"
#include "bt_device.h"
#ifdef USE_SYNERGY
#include <csr_util.h>
#endif

/* Make the type used for message IDs available in debug tools */
LOGGING_PRESERVE_MESSAGE_ENUM(bredr_scan_manager_messages)

#ifndef HOSTED_TEST_ENVIRONMENT

/*! There is checking that the messages assigned by this module do
not overrun into the next module's message ID allocation */
ASSERT_MESSAGE_GROUP_NOT_OVERFLOWED(BREDR_SCAN_MANAGER, BREDR_SCAN_MANAGER_MESSAGE_END)

#endif

/*! Qualcomm Bluetooth SIG company ID */
#define appConfigBtSigCompanyId() (0x00AU)

/*! Macro to insert UUID into EIR data, order of octets swapped as EIR data is little-endian */
#define EIR_UUID128_2(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)  p,o,n,m,l,k,j,i,h,g,f,e,d,c,b,a
#define EIR_UUID128(uuid)  EIR_UUID128_2(uuid)

/*! Maximum packet size of a DM3 packet  */
#define MAX_PACKET_SIZE_DM3 (121)
/*! Maximum size of content in an extended inquiry response (EIR) packet */
#define EIR_MAX_SIZE        (MAX_PACKET_SIZE_DM3)

/*! \name Tags that can be used in an extended inquiry response (EIR) */
#define EIR_TYPE_LOCAL_NAME_COMPLETE        (0x09)
#define EIR_TYPE_LOCAL_NAME_SHORTENED       (0x08)
#define EIR_TYPE_UUID16_COMPLETE            (0x03)
#define EIR_TYPE_UUID128_COMPLETE           (0x07)
#define EIR_TYPE_MANUFACTURER_SPECIFIC      (0xFF)
#define EIR_SIZE_MANUFACTURER_SPECIFIC      (10)  /* Doesn't include size field */


/*! \brief Scan Manager component main data structure. */
bredr_scan_manager_state_t bredr_scan_manager;

static void bredrScanManager_MessageHandler(Task task, MessageId id, Message msg);

bool BredrScanManager_Init(Task init_task)
{
    DEBUG_LOG("BredrScanManager_Init");

    memset(&bredr_scan_manager, 0, sizeof(bredr_scan_manager));
    bredr_scan_manager.task_data.handler = bredrScanManager_MessageHandler;
    bredrScanManager_InstanceInit(bredrScanManager_PageScanContext());
    bredrScanManager_PageScanContext() -> curr_scan_params = &bredr_scan_manager.curr_ps_params;
    bredrScanManager_InstanceInit(bredrScanManager_InquiryScanContext());
    bredrScanManager_InquiryScanContext() -> curr_scan_params = &bredr_scan_manager.curr_is_params;
    bredrScanManager_InstanceInit(bredrScanManager_TruncatedPageScanContext());
    bredrScanManager_TruncatedPageScanContext() -> curr_scan_params = &bredr_scan_manager.curr_ps_params;

#ifdef USE_SYNERGY
    CmWritePageScanTypeReqSend(&bredr_scan_manager.task_data,
                               HCI_SCAN_TYPE_INTERLACED);

    CmWriteInquiryScanTypeReqSend(&bredr_scan_manager.task_data,
                                  HCI_SCAN_TYPE_INTERLACED);
#else
    ConnectionScanEnableRegisterTask(&bredr_scan_manager.task_data);
    ConnectionWritePageScanType(hci_scan_type_interlaced);
    ConnectionWriteInquiryScanType(hci_scan_type_interlaced);
#endif

    /* Register page scan activity as a low priority feature with bandwidth manager */
    PanicFalse(BandwidthManager_RegisterFeature(
                                                    BANDWIDTH_MGR_FEATURE_PAGE_SCAN,
                                                    low_bandwidth_manager_priority,
                                                    bredrScanManager_AdjustPageScanBandwidth));

    MessageSend(init_task, BREDR_SCAN_MANAGER_INIT_CFM, NULL);

    return TRUE;
}

void BredrScanManager_PageScanParametersRegister(const bredr_scan_manager_parameters_t *params)
{
    DEBUG_LOG("BredrScanManager_PageScanParametersRegister %p", params);
    bredrScanManager_InstanceParameterSetRegister(bredrScanManager_PageScanContext(), params);
    bredrScanManager_InstanceParameterSetRegister(bredrScanManager_TruncatedPageScanContext(), params);
}

void BredrScanManager_InquiryScanParametersRegister(const bredr_scan_manager_parameters_t *params)
{
    DEBUG_LOG("BredrScanManager_InquiryScanParametersRegister %p", params);
    bredrScanManager_InstanceParameterSetRegister(bredrScanManager_InquiryScanContext(), params);
}

void BredrScanManager_PageScanParametersSelect(uint8 index)
{
    DEBUG_LOG("BredrScanManager_PageScanParametersSelect %d", index);
    bredrScanManager_InstanceParameterSetSelect(bredrScanManager_PageScanContext(), index);
    bredrScanManager_InstanceParameterSetSelect(bredrScanManager_TruncatedPageScanContext(), index);
}

void BredrScanManager_InquiryScanParametersSelect(uint8 index)
{
    DEBUG_LOG("BredrScanManager_InquiryScanParametersSelect %d", index);
    bredrScanManager_InstanceParameterSetSelect(bredrScanManager_InquiryScanContext(), index);
}

void BredrScanManager_InquiryScanRequest(Task client, bredr_scan_manager_scan_type_t inq_type)
{
    DEBUG_LOG("BredrScanManager_InquiryScanRequest client %p type %d", client, inq_type);
    bredrScanManager_InstanceClientAddOrUpdate(bredrScanManager_InquiryScanContext(), client, inq_type);
}

void BredrScanManager_InquiryScanRelease(Task client)
{
    DEBUG_LOG("BredrScanManager_InquiryScanRelease client %d", client);
    bredrScanManager_InstanceClientRemove(bredrScanManager_InquiryScanContext(), client);
}

void BredrScanManager_PageScanRequest(Task client, bredr_scan_manager_scan_type_t page_type)
{
    DEBUG_LOG("BredrScanManager_PageScanRequest client %d type %d", client, page_type);
    bredrScanManager_InstanceClientAddOrUpdate(bredrScanManager_PageScanContext(), client, page_type);
}

void BredrScanManager_PageScanRelease(Task client)
{
    DEBUG_LOG("BredrScanManager_PageScanRelease client %d", client);
    bredrScanManager_InstanceClientRemove(bredrScanManager_PageScanContext(), client);
}

void BredrScanManager_TruncatedPageScanRequest(Task client, bredr_scan_manager_scan_type_t page_type)
{
    DEBUG_LOG("BredrScanManager_TruncatedPageScanRequest client %d type %d", client, page_type);
    bredrScanManager_InstanceClientAddOrUpdate(bredrScanManager_TruncatedPageScanContext(), client, page_type);

    bsm_scan_context_t * context = bredrScanManager_TruncatedPageScanContext();

    if(bredrScanManager_PageScanContext()->state != BSM_SCAN_DISABLED)
    {
        if(!context->paused)
        {
            context->paused = TRUE;
        }

        MessageSend(client, bredrScanManager_PausedMsgId(context), NULL);
    }
}

void BredrScanManager_TruncatedPageScanRelease(Task client)
{
    DEBUG_LOG("BredrScanManager_TruncatedPageScanRelease client %d", client);
    bredrScanManager_InstanceClientRemove(bredrScanManager_TruncatedPageScanContext(), client);
}

bool BredrScanManager_IsPageScanEnabledForClient(Task client)
{
    return bredrScanManager_InstanceIsScanEnabledForClient(bredrScanManager_PageScanContext(), client);
}

void BredrScanManager_ScanDisable(Task disabler)
{
    Task old_disabler;
    PanicNull(disabler);
    old_disabler = bredrScanManager_SetDisableTask(disabler);
    PanicFalse(old_disabler == NULL || old_disabler == disabler);

    bredrScanManager_SetGoals();

    if (BredrScanManager_IsScanDisabled())
    {
        bredrScanManager_SendDisableCfm(TRUE);
    }
}

void BredrScanManager_ScanEnable(void)
{
    if (!BredrScanManager_IsScanDisabled() && bredrScanManager_GetDisableTask())
    {
        /* The scan manager was re-enabled before we reached the stable disabled state
         * Send out a confirmation indicating that we are not disabled
         */
        bredrScanManager_SendDisableCfm(FALSE);
    }
    bredrScanManager_SetDisableTask(NULL);
    bredrScanManager_SetGoals();
}

void bredrScanManager_SetGoals(void)
{
    DEBUG_LOG("bredrScanManager_SetGoals");
    bredr_scan_manager_scan_type_t max_page_type = SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE;
    bredr_scan_manager_scan_type_t max_inquiry_type = SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE;
    bredr_scan_manager_scan_type_t max_truncated_page_type = SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE;
    if (!bredrScanManager_IsDisabled())
    {
        /* scanning is currently enabled */
        /* determine the highest priority scan type for all scan instances */
        max_page_type = bredrScanManager_InstanceFindMaxScanType(bredrScanManager_PageScanContext(), bredrScanManager_IsThrottleScanRequested());
        max_inquiry_type = bredrScanManager_InstanceFindMaxScanType(bredrScanManager_InquiryScanContext(), bredrScanManager_IsThrottleScanRequested());
        max_truncated_page_type = bredrScanManager_InstanceFindMaxScanType(bredrScanManager_TruncatedPageScanContext(), bredrScanManager_IsThrottleScanRequested());

    }
    if (bredrScanManager_PageScanContext()->type != max_page_type || bredrScanManager_InquiryScanContext()->type != max_inquiry_type || bredrScanManager_TruncatedPageScanContext()->type != max_truncated_page_type)
    {
        DEBUG_LOG("bredrScanManager_SetGoals: new goals PS enum:bredr_scan_manager_scan_type_t:%d IS enum:bredr_scan_manager_scan_type_t:%d TPS enum:bredr_scan_manager_scan_type_t:%d", max_page_type, max_inquiry_type, max_truncated_page_type);
        /* update the instance scan types and scan parameters */
        if (bredrScanManager_TruncatedPageScanContext()->type != max_truncated_page_type)
        {
            bredrScanManager_TruncatedPageScanContext()->type = max_truncated_page_type;

            if (!TEST_BSM_STATE(bredrScanManager_TruncatedPageScanContext()->state, BSM_SCAN_DISABLED))
            {
                bredrScanManager_InstanceUpdateScanActivity(bredrScanManager_TruncatedPageScanContext(), bredrScanManager_TruncatedPageScanContext()->type);
            }
        }
        if (bredrScanManager_PageScanContext()->type != max_page_type)
        {
            bredrScanManager_PageScanContext()->type = max_page_type;

            if (!TEST_BSM_STATE(bredrScanManager_PageScanContext()->state, BSM_SCAN_DISABLED))
            {
                bredrScanManager_InstanceUpdateScanActivity(bredrScanManager_PageScanContext(), bredrScanManager_PageScanContext()->type);
            }
        }
        if (bredrScanManager_InquiryScanContext()->type != max_inquiry_type)
        {
            bredrScanManager_InquiryScanContext()->type = max_inquiry_type;

            if (!TEST_BSM_STATE(bredrScanManager_InquiryScanContext()->state, BSM_SCAN_DISABLED))
            {
                bredrScanManager_InstanceUpdateScanActivity(bredrScanManager_InquiryScanContext(), bredrScanManager_InquiryScanContext()->type);
            }
        }
    }

    bredrScanManager_RunStateMachine();
}

/* determine which scans we want to enable for given goals */
static void bredrScanManager_TargetStateForGoals(bredr_scan_manager_scan_type_t tps_goal,
                                                 bredr_scan_manager_scan_type_t ps_goal,
                                                 bredr_scan_manager_scan_type_t is_goal,
                                                 bool *target_tps_state,
                                                 bool *target_ps_state,
                                                 bool *target_is_state)
{
    /* start from lower priority features and work up to higher priority features as needed to satisfy the goals */
    if (tps_goal > SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE)
    {
        *target_tps_state = TRUE;
    }
    if (ps_goal > SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE)
    {
        /* Prioritise PS over TPS and override TPS target state to change context,
         * if TPS and PS are both requested at the same time */
        *target_tps_state = FALSE;
        *target_ps_state = TRUE;
    }
    if (is_goal > SCAN_MAN_PARAMS_TYPE_PENDING_RELEASE)
    {
        *target_is_state = TRUE;
    }

    DEBUG_LOG("bredrScanManager_TargetStateForGoals target state to achieve goals: tps:%d ps:%d is:%d", *target_tps_state, *target_ps_state, *target_is_state);
}

static void bredrScanManager_CleanUpInstances(void)
{
    bredrScanManager_InstanceCleanUp(bredrScanManager_PageScanContext());
    bredrScanManager_InstanceCleanUp(bredrScanManager_TruncatedPageScanContext());
    bredrScanManager_InstanceCleanUp(bredrScanManager_InquiryScanContext());
}

void bredrScanManager_RunStateMachine(void)
{
    DEBUG_LOG("bredrScanManager_RunStateMachine");

    uint8 actions_in_progress = TEST_BSM_STATE(bredrScanManager_TruncatedPageScanContext()->state, BSM_SCAN_DISABLING) +
                                TEST_BSM_STATE(bredrScanManager_TruncatedPageScanContext()->state, BSM_SCAN_ENABLING) +
                                TEST_BSM_STATE(bredrScanManager_PageScanContext()->state, BSM_SCAN_DISABLING) +
                                TEST_BSM_STATE(bredrScanManager_PageScanContext()->state, BSM_SCAN_ENABLING) +
                                TEST_BSM_STATE(bredrScanManager_InquiryScanContext()->state, BSM_SCAN_DISABLING) +
                                TEST_BSM_STATE(bredrScanManager_InquiryScanContext()->state, BSM_SCAN_ENABLING);
    if (actions_in_progress)
    {
        if (actions_in_progress>1)
        {
            /* we should never have more than 1 action in progress */
            Panic();
        }
        /* wait for the confirmation to arrive before doing anything else */
        DEBUG_LOG("bredrScanManager_RunStateMachine, action still pending - wait");
        return;
    }

    bool truncated_page_scan = FALSE;
    bool page_scan = FALSE;
    bool inquiry_scan = FALSE;
    bredrScanManager_TargetStateForGoals(bredrScanManager_TruncatedPageScanContext()->type,
                                         bredrScanManager_PageScanContext()->type,
                                         bredrScanManager_InquiryScanContext()->type,
                                         &truncated_page_scan,
                                         &page_scan,
                                         &inquiry_scan);

    /* check whether any action is needed
     * Always perform one action at a time - the confirmations from the stack do not say what the current state is,
     * so requesting multiple actions simultaneously could leave us wondering what was done when the first confirmation
     * arrives back
     */
    /* start with disabling currently enabled features */
    if (truncated_page_scan == FALSE && TEST_BSM_STATE(bredrScanManager_TruncatedPageScanContext()->state, BSM_SCAN_ENABLED))
    {
        DEBUG_LOG("bredrScanManager_RunStateMachine, Disabling TPS");

        SET_BSM_STATE(bredrScanManager_TruncatedPageScanContext()->state, BSM_SCAN_DISABLING);
        QcomConManagerEnableTruncatedScan(bredrScanManager_GetTask(), FALSE);
        return;
    }
    if (page_scan == FALSE && TEST_BSM_STATE(bredrScanManager_PageScanContext()->state, BSM_SCAN_ENABLED))
    {
        DEBUG_LOG("bredrScanManager_RunStateMachine, Disabling PS");

        SET_BSM_STATE(bredrScanManager_PageScanContext()->state, BSM_SCAN_DISABLING);
        bredrScanManager_ConnectionWriteScanEnable();
        return;
    }
    if (inquiry_scan == FALSE && TEST_BSM_STATE(bredrScanManager_InquiryScanContext()->state, BSM_SCAN_ENABLED))
    {
        DEBUG_LOG("bredrScanManager_RunStateMachine, Disabling IS");

        SET_BSM_STATE(bredrScanManager_InquiryScanContext()->state, BSM_SCAN_DISABLING);
        bredrScanManager_ConnectionWriteScanEnable();
        return;
    }
    /* no features need disabling, do any features need enabling? */
    if (truncated_page_scan == TRUE && TEST_BSM_STATE(bredrScanManager_TruncatedPageScanContext()->state, BSM_SCAN_DISABLED))
    {
        DEBUG_LOG("bredrScanManager_RunStateMachine, Enabling TPS");

        bredrScanManager_InstanceUpdateScanActivity(bredrScanManager_TruncatedPageScanContext(), bredrScanManager_TruncatedPageScanContext()->type);

        SET_BSM_STATE(bredrScanManager_TruncatedPageScanContext()->state, BSM_SCAN_ENABLING);
        QcomConManagerEnableTruncatedScan(bredrScanManager_GetTask(), TRUE);

        return;
    }
    if (page_scan == TRUE && TEST_BSM_STATE(bredrScanManager_PageScanContext()->state, BSM_SCAN_DISABLED))
    {
        DEBUG_LOG("bredrScanManager_RunStateMachine, Enabling PS");

        bredrScanManager_InstanceUpdateScanActivity(bredrScanManager_PageScanContext(), bredrScanManager_PageScanContext()->type);

        SET_BSM_STATE(bredrScanManager_PageScanContext()->state, BSM_SCAN_ENABLING);
        bredrScanManager_ConnectionWriteScanEnable();
        return;
    }
    if (inquiry_scan == TRUE && TEST_BSM_STATE(bredrScanManager_InquiryScanContext()->state, BSM_SCAN_DISABLED))
    {
        DEBUG_LOG("bredrScanManager_RunStateMachine, Enabling IS");

        bredrScanManager_InstanceUpdateScanActivity(bredrScanManager_InquiryScanContext(), bredrScanManager_InquiryScanContext()->type);

        SET_BSM_STATE(bredrScanManager_InquiryScanContext()->state, BSM_SCAN_ENABLING);
        bredrScanManager_ConnectionWriteScanEnable();
        return;
    }

    bredrScanManager_CleanUpInstances();
    DEBUG_LOG("bredrScanManager_RunStateMachine, goals achieved");
}

/*! @brief Set up Eir data and write to the connection library. */
static void bredrScanManager_InitialiseEir(const uint8 *local_name, uint16 size_local_name)
{
    static const uint8 eir_16bit_uuids[] =
    {
        EIR_TYPE_UUID16_COMPLETE, /* Complete list of 16-bit Service Class UUIDs */
#ifdef INCLUDE_HFP
        0x1E, 0x11,     /* HFP 0x111E */
        0x08, 0x11,     /* HSP 0x1108 */
#endif
#ifdef INCLUDE_AV
#ifdef INCLUDE_AV_SOURCE
        0x0A, 0x11,     /* AudioSource 0x110A */
#else
        0x0B, 0x11,     /* AudioSink 0x110B */
#endif
        0x0D, 0x11,     /* A2DP 0x110D */
        0x0E, 0x11      /* AVRCP 0x110E */
#endif
    };

#ifdef INCLUDE_ACCESSORY
    static const uint8 eir_128bit_uuids[] =
    {
        EIR_TYPE_UUID128_COMPLETE, /* Complete list of 128-bit Service Class UUIDs */
        EIR_UUID128(UUID_ACCESSORY_SERVICE)
    };
#endif

    uint8 *const eir = (uint8 *)PanicUnlessMalloc(EIR_MAX_SIZE + 1);
    uint8 *const eir_end = eir + EIR_MAX_SIZE + 1;
    uint8 *eir_ptr = eir;
    uint16 eir_space;
    bdaddr peer_bdaddr;

    DEBUG_LOG("bredrScanManager_InitialiseEir");

    /* Get peer device address, default to all 0's if we haven't paired */
    BdaddrSetZero(&peer_bdaddr);
    appDeviceGetPeerBdAddr(&peer_bdaddr);

    /* Copy 16 bit UUIDs into EIR packet */
    *eir_ptr++ = sizeof(eir_16bit_uuids);
    memmove(eir_ptr, eir_16bit_uuids, sizeof(eir_16bit_uuids));
    eir_ptr += sizeof(eir_16bit_uuids);

#ifdef INCLUDE_ACCESSORY
    /* Copy 128 bit UUIDs into EIR packet */
    *eir_ptr++ = sizeof(eir_128bit_uuids);
    memmove(eir_ptr, eir_128bit_uuids, sizeof(eir_128bit_uuids));
    eir_ptr += sizeof(eir_128bit_uuids);
#endif

    /* Calculate space for local device name */
    eir_space = (eir_end - eir_ptr) - 3;  /* Take 3 extra from space for type and size fields and zero terminator */

    /* Check if name need trucating */
    if (eir_space < size_local_name)
    {
        /* Store header for truncated name */
        *eir_ptr++ = eir_space + 1;
        *eir_ptr++ = EIR_TYPE_LOCAL_NAME_SHORTENED;

        /* Clip size of name to space available */
        size_local_name = eir_space;
    }
    else
    {
        /* Store header for complete name */
        *eir_ptr++ = size_local_name + 1;
        *eir_ptr++ = EIR_TYPE_LOCAL_NAME_COMPLETE;
    }

    /* Copy local device name into EIR packet */
    memmove(eir_ptr, local_name, size_local_name);
    eir_ptr += size_local_name;

    /* Add termination character */
    *eir_ptr++ = 0x00;

    /* Write EIR data */
    ConnectionWriteEirData(FALSE, eir_ptr - eir, eir);

#ifndef USE_SYNERGY
    /* Free the EIR data */
    free(eir);
#endif
}

/*! \brief Local name request completed

    The request for the local device name has completed, pass the name to
    scanManager_InitialiseEir() to allow the Extended Inquiry Response to be initialised.
*/
static void scanManager_HandleClDmLocalNameComplete(uint8* name, uint16 name_len, bool success)
{
    bool setup_success = FALSE;

    DEBUG_LOG("scanManager_HandleClDmLocalNameComplete, status %d", success);

    if (success)
    {
        /* Initialise pairing module, this will set EIR data */
        bredrScanManager_InitialiseEir(name, name_len);
        setup_success = TRUE;
    }
    bredr_scan_manager.eir_setup_in_progress = FALSE;

    if(bredr_scan_manager.eir_setup_complete_callback)
    {
        bredr_scan_manager.eir_setup_complete_callback(setup_success);
    }
}

static void bredrScanManager_HandleQcomTruncatedScanEnableCfm(Message msg)
{
    QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM_T * cfm = (QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM_T*)msg;
    if(cfm->status != HCI_SUCCESS)
    {
        DEBUG_LOG("bredrScanManager_HandleQcomTruncatedScanEnableCfm HCI error 0x%x", cfm->status);
        Panic();
    }
    if (!bredrScanManager_InstanceCompleteTransition(bredrScanManager_TruncatedPageScanContext()))
    {
        /* Panic if we were not expecting to receive this notification */
        Panic();
    }
}

#ifdef USE_SYNERGY
static void bredrScanManager_HandleCmPrim(Message msg)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) msg;

    switch (*prim)
    {
        case CSR_BT_CM_WRITE_SCAN_ENABLE_CFM:
        {
            CsrBtCmWriteScanEnableCfm *cfm = (CsrBtCmWriteScanEnableCfm *) msg;
            DEBUG_LOG("bredrScanManager_HandleCmPrim CSR_BT_CM_WRITE_SCAN_ENABLE_CFM");

            if (cfm->resultCode != CSR_BT_RESULT_CODE_CM_SUCCESS)
            {
                DEBUG_LOG("bredrScanManager_HandleCmPrim CSR_BT_CM_WRITE_SCAN_ENABLE_CFM: result:0x%04x supplier:0x%04x", cfm->resultCode, cfm->resultSupplier);
                Panic();
            }
            bredrScanManager_ConnectionHandleClDmWriteScanEnableCfm();
        }
        break;

        case CSR_BT_CM_WRITE_PAGESCAN_TYPE_CFM:
            break;
        case CSR_BT_CM_WRITE_PAGESCAN_SETTINGS_CFM:
            break;
        case CSR_BT_CM_WRITE_INQUIRYSCAN_SETTINGS_CFM:
            break;

        case CSR_BT_CM_READ_LOCAL_NAME_CFM:
        {
            CsrBtCmReadLocalNameCfm *cfm = (CsrBtCmReadLocalNameCfm *)msg;
            DEBUG_LOG("bredrScanManager_HandleCmPrim CSR_BT_CM_READ_LOCAL_NAME_CFM");
            bool status = FALSE;
            uint16 len = 0;
            if (cfm->localName)
            {
                status = TRUE;
                len = CsrStrLen((char *) cfm->localName);
            }

            scanManager_HandleClDmLocalNameComplete(cfm->localName, len, status);
        }
        break;

        default:
        {
            DEBUG_LOG("bredrScanManager_HandleCmPrim unhandled CM message 0x%x",
                      *prim);
        }
        break;
    }
    CmFreeUpstreamMessageContents((void *) msg);
}

static void bredrScanManager_MessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case CM_PRIM:
        {
            bredrScanManager_HandleCmPrim(msg);
        }
        break;

        case QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM:
        {
            DEBUG_LOG("bredrScanManager_MessageHandler QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM");
            bredrScanManager_HandleQcomTruncatedScanEnableCfm((QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM_T*)msg);
        }
        break;

        default:
        {
            DEBUG_LOG("bredrScanManager_MessageHandler unexpected message 0x%x", id);
        }
        break;
    }
}
#else
static void bredrScanManager_MessageHandler(Task task, MessageId id, Message msg)
{
    UNUSED(task);

    switch (id)
    {
        case CL_DM_WRITE_SCAN_ENABLE_CFM:
        {
            const CL_DM_WRITE_SCAN_ENABLE_CFM_T *cfm = (CL_DM_WRITE_SCAN_ENABLE_CFM_T *)msg;
            DEBUG_LOG("bredrScanManager_MessageHandler CL_DM_WRITE_SCAN_ENABLE_CFM");
            if (cfm->status != hci_success)
            {
                Panic();
            }
            if (cfm->outstanding == 0)
            {
                bredrScanManager_ConnectionHandleClDmWriteScanEnableCfm();
            }
        }
        break;

        case CL_DM_LOCAL_NAME_COMPLETE:
        {
            CL_DM_LOCAL_NAME_COMPLETE_T * cfm = (CL_DM_LOCAL_NAME_COMPLETE_T *)msg;
            DEBUG_LOG("bredrScanManager_MessageHandler CL_DM_LOCAL_NAME_COMPLETE");
            scanManager_HandleClDmLocalNameComplete(cfm->local_name, cfm->size_local_name, (cfm->status == hci_success));
        }
        break;

        case QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM:
        {
            DEBUG_LOG("bredrScanManager_MessageHandler QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM");
            bredrScanManager_HandleQcomTruncatedScanEnableCfm((QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM_T*)msg);
        }
        break;

        case BREDR_SCAN_MANAGER_PAGE_SCAN_THROTTLED_IND:
        case BREDR_SCAN_MANAGER_PAGE_SCAN_UNTHROTTLED_IND:
            break;

        default:
        {
            DEBUG_LOG("bredrScanManager_MessageHandler unhandled message MESSAGE:0x%x", id);
            Panic();
        }
        break;
    }
}
#endif

bool ScanManager_ConfigureEirData(eir_setup_complete_callback_t callback_function)
{
    DEBUG_LOG("ScanManager_ConfigureEirData");

    if(!bredr_scan_manager.eir_setup_in_progress)
    {
        bredr_scan_manager.eir_setup_in_progress = TRUE;
        bredr_scan_manager.eir_setup_complete_callback = callback_function;
        /* Get device name so that we can initialise EIR response */
        ConnectionReadLocalName(&bredr_scan_manager.task_data);
        return TRUE;
    }
    return FALSE;
}

/*! \brief Determine if scanning is disabled in all scan contexts.
    \return TRUE if disabled.
*/
bool BredrScanManager_IsScanDisabled(void)
{
    return (TEST_BSM_STATE(bredr_scan_manager.page_scan.state, BSM_SCAN_DISABLED) &&
            TEST_BSM_STATE(bredr_scan_manager.inquiry_scan.state, BSM_SCAN_DISABLED) &&
            TEST_BSM_STATE(bredr_scan_manager.truncated_page_scan.state, BSM_SCAN_DISABLED));
}

void bredrScanManager_AdjustPageScanBandwidth(bool throttle_required)
{
    if (throttle_required)
    {
        DEBUG_LOG("bredrScanManager_AdjustPageScanBandwidth: Throttle scan requested");

        bredr_scan_manager.thorottle_scan_requested = TRUE;
    }
    else
    {
        DEBUG_LOG("bredrScanManager_AdjustPageScanBandwidth: Throttle scan released");

        bredr_scan_manager.thorottle_scan_requested = FALSE;
    }

    bredrScanManager_SetGoals();
}

#ifdef HOSTED_TEST_ENVIRONMENT
void BredrScanManager_TestPageScanAdjustBandwidth(bool throttle_required)
{
    bredrScanManager_AdjustPageScanBandwidth(throttle_required);
}
#endif
