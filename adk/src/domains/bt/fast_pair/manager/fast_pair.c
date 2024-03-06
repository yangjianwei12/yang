/*!
\copyright  Copyright (c) 2008 - 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair.c
\brief      Fast Pair State Machine Task
*/


#include "fast_pair.h"
#include "fast_pair_events.h"
#include "fast_pair_gfps.h"
#include "fast_pair_pairing_if.h"
#include "fast_pair_advertising.h"
#include "fast_pair_session_data.h"
#include "fast_pair_null_state.h"
#include "fast_pair_idle_state.h"
#include "fast_pair_wait_aes_key_state.h"
#include "fast_pair_wait_pairing_request_state.h"
#include "fast_pair_wait_passkey_state.h"
#include "fast_pair_wait_account_key_state.h"
#include "fast_pair_battery_notifications.h"
#include "fast_pair_msg_stream.h"
#include "fast_pair_wait_additional_data_state.h"
#include "fast_pair_pname_state.h"
#include "fast_pair_pname_sync.h"
#include "fast_pair_msg_stream_dev_action.h"
#include "fast_pair_config.h"
#ifdef FAST_PAIR_TIME_PROFILER
#include "fast_pair_time_profiler.h"
#endif /* FAST_PAIR_TIME_PROFILER */
#include "fast_pair_bloom_filter.h"

#include "system_state.h"
#include "device_properties.h"
#include "user_accounts.h"
#include "user_accounts_sync.h"
#include "phy_state.h"

#include <connection_manager.h>
#ifndef USE_SYNERGY
#include <connection_message_dispatcher.h>
#endif /* !USE_SYNERGY */
#include <ps_key_map.h>

#include <panic.h>
#include <connection.h>
#include <ui.h>
#include <device.h>
#include <device_list.h>
#include <ps.h>
#include <string.h>
#include <cryptovm.h>
#include <stdio.h>

#include <cc_with_case.h>
#include "state_proxy.h"
#include <rsa_pss_constants.h>

LOGGING_PRESERVE_MESSAGE_TYPE(fast_pair_state_event_id)

/*!< Fast Pair task */
fastPairTaskData fast_pair_task_data;

/*! @brief Clear FP Session Information.
 */
static void fastPair_FreeSessionDataMemory(void)
{
    fastPairTaskData *theFastPair;
    theFastPair = fastPair_GetTaskData();

    if(theFastPair->session_data.private_key != NULL)
    {
        free(theFastPair->session_data.private_key);
        theFastPair->session_data.private_key= NULL;
    }
    
    if(theFastPair->session_data.public_key != NULL)
    {
        free(theFastPair->session_data.public_key);
        theFastPair->session_data.public_key= NULL;
    }

    if(theFastPair->session_data.encrypted_data != NULL)
    {
        free(theFastPair->session_data.encrypted_data);
        theFastPair->session_data.encrypted_data= NULL;
    }

    if(theFastPair->session_data.aes_key != NULL)
    {
        free(theFastPair->session_data.aes_key);
        theFastPair->session_data.aes_key= NULL;
    }

    theFastPair->session_data.kbp_action_request_data_id = 0xFF;
    theFastPair->session_data.kbp_pname_request_flag = FALSE;
}


/*! @brief Cancel FP Procedure and check for Repeated Invalid KbP Writes.
 */
static void fastPair_EnterIdle(fastPairTaskData *theFastPair)
{
    DEBUG_LOG("appFastPairEnterIdle");

    fastPair_StopTimer();
    fastPair_PairingReset();
    /* Set the pairing initiator flag to FALSE while moving to IDLE state*/
    theFastPair->provider_pairing_requested = FALSE;
    if(theFastPair->failure_count == FAST_PAIR_MAX_FAIL_ATTEMPTS)
    {
        theFastPair->failure_count = 0;

        /* Fail all new writes to KbP for next 5 minutes or till a Power Off/Power On event */ 
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_NULL);
        
        fastPair_StartTimer(TRUE);
    }
    /* Reset the handset public address upon entering into IDLE state */
    memset(&theFastPair->handset_bd_addr, 0x0, sizeof(bdaddr));

    fastPair_FreeSessionDataMemory();
}

/*! @brief FP Procedure starting. Enter Wait state for AES key to be established.
 */
static void fastPair_EnterWaitAESKey(void)
{
    DEBUG_LOG("fastPair_EnterWaitAESKey");
}

/*! @brief Enter Wait state for Pairing request from FP seeker.
 */
static void fastPair_EnterWaitPairingRequest(void)
{
    DEBUG_LOG("fastPair_EnterWaitPairingRequest");

    fastPair_StartTimer(FALSE);
}

/*! @brief Enter Wait state for Passkey to be written by FP Seeker.
 */
static void fastPair_EnterWaitPasskey(void)
{
    DEBUG_LOG("fastPair_EnterWaitPasskey");
    
    fastPair_StartTimer(FALSE);
}

/*! @brief Enter Wait state for Account key to be written by FP Seeker.
 */
static void fastPair_EnterWaitAccountKey(void)
{
    DEBUG_LOG("fastPair_EnterWaitAccountKey");
    
    fastPair_StartTimer(FALSE);
}

/*! @brief Enter Wait state for Account key to be written by FP Seeker.
 */
static void fastPair_EnterWaitAdditionalData(void)
{
    DEBUG_LOG("fastPair_EnterWaitAdditionalData");

    fastPair_StartTimer(FALSE);
}

static void fastPair_EnterPName(void)
{
    DEBUG_LOG("fastPair_EnterPName");
}

/*! @brief Initialize Session data to NULL.
 */
static void fastPair_InitSessionData(void)
{
    fastPairTaskData *theFastPair = fastPair_GetTaskData();
    theFastPair->session_data.private_key = NULL;
    theFastPair->session_data.public_key = NULL;
    theFastPair->session_data.encrypted_data = NULL;
    theFastPair->session_data.aes_key = NULL;
}

/*! @brief Handle Power ON/OFF system state change events
 */
static void fastPair_HandleSystemStateChange(SYSTEM_STATE_STATE_CHANGE_T *msg)
{
    fastPairTaskData *theFastPair;
    theFastPair = fastPair_GetTaskData();

    DEBUG_LOG("fastPair_HandleSystemStateChange old state 0x%x, new state 0x%x", msg->old_state, msg->new_state);

    if(msg->old_state == system_state_powering_off && msg->new_state == system_state_limbo)
    {
        uint16* ring_stop = PanicUnlessMalloc(sizeof(uint16));
            
        *ring_stop = FP_STOP_RING_CURRENT;
        DEBUG_LOG("fastPair_HandleSystemStateChange. Set FP state to NULL");
        MessageSend(fpRingDevice_GetTask(), fast_pair_ring_stop_event, ring_stop);
        fastPair_SetState(theFastPair, FAST_PAIR_STATE_NULL);
    }

    else if(msg->old_state == system_state_powering_on && msg->new_state == system_state_active)
    {
        /*! Set the fast pair state to idle state only if the current state is NULL*/
        if(theFastPair->state == FAST_PAIR_STATE_NULL)
        {
            fastPair_SetState(theFastPair, FAST_PAIR_STATE_IDLE);
        }
        else
        {
            DEBUG_LOG("fastPair_HandleSystemStateChange. Already powered ON");
        }
    }
}

bool FastPair_IsBusyWithHandset(device_t device)
{
    bdaddr handset_addr = DeviceProperties_GetBdAddr(device);

    fastPairTaskData *theFastPair = fastPair_GetTaskData();

    for (unsigned index = 0; index < MAX_BLE_CONNECTIONS; index++)
    {
        if(BdaddrIsSame(&theFastPair->peer_bd_addr[index], &handset_addr))
        {
            return !fastPair_IsIdle();
        }
    }
    return FALSE;
}

/*! \brief Delete the account keys.
 */
void FastPair_DeleteAccountKeys(void)
{
    UserAccounts_DeleteAllAccountKeys();
#ifdef USE_FAST_PAIR_ACCOUNT_DATA_V0
    /* Notify LE advertising manager regarding the change in FP adverts */
    if(fastPair_AdvNotifyDataChange())
    {
        DEBUG_LOG("FastPair_DeleteAccountKeys. Notified LE advertising manager after the data change.");
    }
    else
    {
        DEBUG_LOG("FastPair_DeleteAccountKeys. Couldn't notify LE advertising manager after the change in FP adverts.");
    }
#else
    fastPair_AccountKeysModified();
#endif
}

uint8 * fastPair_GetOwnerAccountKey(void)
{
    /* Very first account key to be written is considered as owner account key */
    #define OWNER_USER_ACCOUNT_KEY_INDEX                (0)

    uint8 *keys = NULL;
    uint8 *account_key = NULL;
    uint8 num_account_keys = UserAccounts_GetAccountKeys(&keys, user_account_type_fast_pair);

    DEBUG_LOG("fastPair_GetOwnerAccountKey. Number of account keys = %d", num_account_keys);

    /* Return account key at index 0 when there are account keys present on device, NULL otherwise*/
    if(num_account_keys == 0)
    {
        free(keys);
        return NULL;
    }

    /* Memory allocation for the account key to be returned, this should be freed by the caller of this function after its usage */
    account_key = (uint8*)PanicUnlessMalloc(FAST_PAIR_ACCOUNT_KEY_LEN);
    /* Copy the account key present at index 0 */
    memcpy(account_key, &keys[OWNER_USER_ACCOUNT_KEY_INDEX], FAST_PAIR_ACCOUNT_KEY_LEN);
    free(keys);

    return account_key;
}

static void fastPair_UnscrambleAspk(uint16 * aspk)
{
    const uint16 seed[FAST_PAIR_PRIVATE_KEY_LEN/2] = 
    {
        0x11ac, 0x5a6e, 0x0e49, 0x5aa3, 0xe3e0, 0xbb20, 0xac0e, 0xf136, 0x5dfb, 0x5282, 0x002b, 0x37f2, 0x28f1, 0xd18c, 0xa613, 0x8de2
    };
    uint16* unscrambled_aspk = PanicUnlessMalloc(FAST_PAIR_PRIVATE_KEY_LEN);
    for(uint16 i=0, j=FAST_PAIR_PRIVATE_KEY_LEN/2; i <(FAST_PAIR_PRIVATE_KEY_LEN/2) && j > 0; i++,j--)
    {
		/* Use last 16 words of M array in rsa_decrypt_constant_mod structure */
        unscrambled_aspk[i] = aspk[i] ^ rsa_decrypt_constant_mod.M[RSA_SIGNATURE_SIZE - j] ^ seed[i];
    }
    ByteUtilsMemCpyUnpackString((uint8 *)aspk, (const uint16 *)&unscrambled_aspk[0], FAST_PAIR_PRIVATE_KEY_LEN);
    free(unscrambled_aspk);
}

/*! \brief Get Fast Pair Model ID and ASPK
*/
static void fastPair_GetModelIdAspk(void)
{
    DEBUG_LOG("fastPair_GetModelIdAspk");
    uint8 fp_config_model_id[FAST_PAIR_CONFIG_MODEL_ID_LEN];
    if(!PsRetrieveReadOnlyKey(PS_KEY_READ_ONLY_FAST_PAIR_MODEL_ID, fp_config_model_id, FAST_PAIR_CONFIG_MODEL_ID_LEN))
    {
        PsRetrieve(PS_KEY_LEGACY_FAST_PAIR_MODEL_ID, fp_config_model_id, FAST_PAIR_CONFIG_MODEL_ID_LEN);
        DEBUG_LOG("fastPair_GetModelIdAspk model id legacy");
    }

    static uint8 fp_config_aspk[FAST_PAIR_CONFIG_ASPK_LEN];
    if(!PsRetrieveReadOnlyKey(PS_KEY_READ_ONLY_FAST_PAIR_ASPK, fp_config_aspk, FAST_PAIR_CONFIG_ASPK_LEN))
    {
        PsRetrieve(PS_KEY_LEGACY_FAST_PAIR_SCRAMBLED_ASPK, fp_config_aspk, FAST_PAIR_CONFIG_ASPK_LEN);
        DEBUG_LOG("fastPair_GetModelIdAspk aspk legacy");
        fastPair_UnscrambleAspk((uint16 *)fp_config_aspk);
        
    }
    DEBUG_LOG_DATA(fp_config_aspk, FAST_PAIR_PRIVATE_KEY_LEN);

    fastPair_SetPrivateKey((const uint16 *)fp_config_aspk, sizeof(fp_config_aspk));
    fastPair_SetModelId(fp_config_model_id);
}

#ifdef USE_SYNERGY
/*! \brief Initialize fast pair related functionalities
    In case of synergy builds, all the initialization which is done
    as a part of fast pair needs to be done inside this function.
    The order of initialization needs to be same as of FastPair_Init function.
*/
static void fastpair_SynergyInit(void)
{
    fastPairTaskData *theFastPair = fastPair_GetTaskData();
    /* Initialize the Fast Pair Pairing Interface */
    fastPair_PairingInit();

#ifdef INCLUDE_TWS
    /* Register for state proxy battery voltage (local & remote) events */
    StateProxy_EventRegisterClient(&theFastPair->task, state_proxy_event_type_battery_voltage);
    /* Read initial battery value */
    FastPair_LocalBatteryValInit();
#endif

    /* Get Model ID and ASPK from PS */
    fastPair_GetModelIdAspk();

    /* Initialize the Fast Pair Advertising Interface */
    fastPair_SetUpAdvertising();

    fastPair_InitSessionData();

    /* Initialize the Account Key Sync Interface */
    UserAccountsSync_Init();

    /* Initialize the Fast Pair Personalized Name Sync Interface */
    fastPair_PNameSync_Init();

    /* Initialize message stream */
    fastPair_MsgStreamInit();

    /* Ready to start Fast Pair. Move to Idle state */
    theFastPair->state = FAST_PAIR_STATE_IDLE;
}

/*! \brief Message Handler to handle CL messages coming from the application
*/
static void fastPair_GattFPServerInitInd(const GATT_FAST_PAIR_SERVER_INIT_IND_T *init_ind)
{
    DEBUG_LOG("fastPair_GattFPServerInitInd. sts:%d", init_ind->status);

    if (gatt_fast_pair_server_init_success != init_ind->status)
    {
        DEBUG_LOG("fastPair_GattFPServerInitInd. Server init failed");
        Panic();
    }

    fastpair_SynergyInit();
}

static void appHandleCmPrim(Message message)
{
    CsrBtCmPrim *prim = (CsrBtCmPrim *) message;

    switch (*prim)
    {
        case CSR_BT_CM_LE_READ_RANDOM_ADDRESS_CFM:
            fastPair_CacheRandomAddressCfm((CsrBtCmLeReadRandomAddressCfm *)message);
        break;

        case CSR_BT_CM_CRYPTO_HASH_CFM:
            DEBUG_LOG("FastPair_HandleMessage appHandleCmPrim - CSR_BT_CM_CRYPTO_HASH_CFM");
            fastPair_HashCfm((CsrBtCmCryptoHashCfm *)message);
        break;

        case CSR_BT_CM_CRYPTO_ENCRYPT_CFM:
            DEBUG_LOG("FastPair_HandleMessage appHandleCmPrim - CSR_BT_CM_CRYPTO_ENCRYPT_CFM");
            fastPair_EncryptCfm((CsrBtCmCryptoEncryptCfm *)message);
        break;

        case CSR_BT_CM_CRYPTO_DECRYPT_CFM:
            DEBUG_LOG("FastPair_HandleMessage appHandleCmPrim - CSR_BT_CM_CRYPTO_DECRYPT_CFM");
            fastPair_DecryptCfm((CsrBtCmCryptoDecryptCfm *)message);
        break;

        case CSR_BT_CM_CRYPTO_AES_CTR_CFM:
            DEBUG_LOG("FastPair_HandleMessage appHandleCmPrim - CSR_BT_CM_CRYPTO_AES_CTR_CFM");
            fastPair_AesCtrCfm((CsrBtCmCryptoAesCtrCfm *)message);
        break;

        default:
            DEBUG_LOG("FastPair_HandleMessage appHandleCmPrim, unexpected CM prim 0x%04x", *prim);
            break;
    }
    CmFreeUpstreamMessageContents(message);
}
#else
/*! \brief Message Handler to handle CL messages coming from the application
*/
bool FastPair_HandleConnectionLibraryMessages(MessageId id, Message message, bool already_handled)
{
    switch(id)
    {
        case CL_SM_AUTHENTICATE_CFM:
            DEBUG_LOG("FastPair_HandleConnectionLibraryMessages. CL_SM_AUTHENTICATE_CFM");
            fastPair_AuthenticateCfm((CL_SM_AUTHENTICATE_CFM_T *)message);
        break;

        default:
        break;
    }
    return already_handled;
}
#endif /* USE_SYNERGY */

/*! \brief Message Handler

    This function is the main message handler for the fast pair module.
*/
void FastPair_HandleMessage(Task task, MessageId id, Message message)
{

    if((id >= GATT_FAST_PAIR_SERVER_MESSAGE_BASE) && (id < GATT_FAST_PAIR_SERVER_MESSAGE_TOP))
    {
#ifdef USE_SYNERGY
        if(id == GATT_FAST_PAIR_SERVER_INIT_IND)
        {
            fastPair_GattFPServerInitInd((const GATT_FAST_PAIR_SERVER_INIT_IND_T *)message);
        }
        else
#endif /* USE_SYNERGY */
        {
            fastPair_GattFPServerMsgHandler(task, id, message);
        }
        return;
    }    

    UNUSED(task);

    switch (id)
    {
#ifdef USE_SYNERGY
        case CM_PRIM:
            appHandleCmPrim(message);
            break;
#endif /* USE_SYNERGY */

        case CON_MANAGER_TP_CONNECT_IND:
            fastPair_ConManagerConnectInd((CON_MANAGER_TP_CONNECT_IND_T *)message);
        break;

        case CON_MANAGER_TP_DISCONNECT_IND:
            fastPair_ConManagerDisconnectInd((CON_MANAGER_TP_DISCONNECT_IND_T *)message);
        break;

        case CON_MANAGER_HANDSET_CONNECT_ALLOW_IND:
            fastPair_ConManagerHandsetConnectAllowInd();
        break;

        case CON_MANAGER_HANDSET_CONNECT_DISALLOW_IND:
            fastPair_ConManagerHandsetConnectDisallowInd();
        break;

#ifndef USE_SYNERGY
        case CL_SM_BLE_READ_RANDOM_ADDRESS_CFM:
            fastPair_CacheRandomAddressCfm((const CL_SM_BLE_READ_RANDOM_ADDRESS_CFM_T *)message);
        break;
#endif /* !USE_SYNERGY */

        case CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM:
            fastPair_SharedSecretCfm((CL_CRYPTO_GENERATE_SHARED_SECRET_KEY_CFM_T *)message);
        break;

#ifndef USE_SYNERGY
        case CL_CRYPTO_HASH_CFM:
            fastPair_HashCfm((CL_CRYPTO_HASH_CFM_T *)message);
        break;

        case CL_CRYPTO_ENCRYPT_CFM:
            fastPair_EncryptCfm((CL_CRYPTO_ENCRYPT_CFM_T *)message);
        break;

        case CL_CRYPTO_DECRYPT_CFM:
            fastPair_DecryptCfm((CL_CRYPTO_DECRYPT_CFM_T *)message);
        break;

        case CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM:
            fastPair_AesCtrCfm((CL_CRYPTO_ENCRYPT_DECRYPT_AES_CTR_CFM_T *)message);
        break;
#endif /* !USE_SYNERGY */

        case fast_pair_state_event_timer_expire:
            fastPair_TimerExpired();
        break;

        case PAIRING_ACTIVITY:
            fastPair_PairingActivity((PAIRING_ACTIVITY_T*)message);
        break;
        

        case PAIRING_STOP_CFM:
            DEBUG_LOG("FastPair_HandleMessage, PAIRING_STOP_CFM");
            if(fastPair_IsProviderPairingRequested())
            {
                fastPair_ProviderInitiatePairing();
            }
        break;

        case PHY_STATE_CHANGED_IND:
        {
            uint16* ring_stop = PanicUnlessMalloc(sizeof(uint16));
            
            *ring_stop = FP_STOP_RING_CURRENT;
            PHY_STATE_CHANGED_IND_T* msg = (PHY_STATE_CHANGED_IND_T *)message;
            if(msg->new_state == PHY_STATE_IN_CASE)
            {
                MessageSend(fpRingDevice_GetTask(), fast_pair_ring_stop_event, ring_stop);
                fastPair_PowerOff();
            }
            else if(msg->new_state == PHY_STATE_IN_EAR)
            {
                MessageSend(fpRingDevice_GetTask(), fast_pair_ring_stop_event, ring_stop);
            }
            else
            {
                /* Free the message as it won't be sent as PHY state is neither In case nor In ear */ 
                free(ring_stop);
            }
        }
        break;

        case SYSTEM_STATE_STATE_CHANGE:
            fastPair_HandleSystemStateChange((SYSTEM_STATE_STATE_CHANGE_T *)message);
        break;

#ifdef INCLUDE_CASE_COMMS
        case CASE_LID_STATE:
            fastPair_BatteryHandleCaseLidState((const CASE_LID_STATE_T *)message);
        break;
        case CASE_POWER_STATE:
            fastPair_BatteryHandleCasePowerState((const CASE_POWER_STATE_T *)message);
        break;
#endif
#ifdef INCLUDE_TWS
        case STATE_PROXY_EVENT:
            fastPair_HandleStateProxyEvent((const STATE_PROXY_EVENT_T*)message);
        break;
#endif
        default:
            DEBUG_LOG("Unhandled MessageID = MSG:fast_pair_state_event_id:0x%d", id);
        break;
    }
}


/******************************************************************************/

/*! \brief Set Fast Pair FSM state */
bool fastPair_StateMachineHandleEvent(fast_pair_state_event_t event)
{
    bool ret_val = FALSE;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

#ifdef FAST_PAIR_TIME_PROFILER
    fastpair_TimeProfiler(event.id);
#endif
    switch(fastPair_GetState(theFastPair))
    {
        case FAST_PAIR_STATE_NULL:
            ret_val = fastPair_StateNullHandleEvent(event);
        break;
        
        case FAST_PAIR_STATE_IDLE:
            ret_val = fastPair_StateIdleHandleEvent(event);
        break;
        
        case FAST_PAIR_STATE_WAIT_AES_KEY:
            ret_val = fastPair_StateWaitAESKeyHandleEvent(event);
        break;

        case FAST_PAIR_STATE_WAIT_PAIRING_REQUEST:
            ret_val = fastPair_StateWaitPairingRequestHandleEvent(event);
        break;

        case FAST_PAIR_STATE_WAIT_PASSKEY:
            ret_val = fastPair_StateWaitPasskeyHandleEvent(event);
        break;
   
        case FAST_PAIR_STATE_WAIT_ACCOUNT_KEY:
            ret_val = fastPair_StateWaitAccountKeyHandleEvent(event);
        break;

        case FAST_PAIR_STATE_WAIT_ADDITIONAL_DATA:
            ret_val = fastPair_StateWaitAdditionalDataHandleEvent(event);
        break;
        case FAST_PAIR_STATE_PNAME:
            ret_val = fastPair_StatePNameHandleEvent(event);
        break;


        default:
            DEBUG_LOG("Unhandled event\n");
        break;
    }
    return ret_val;
}
/*! \brief Set Fast Pair State
    Called to change state.  Handles calling the state entry and exit
    functions for the new and old states.
*/
void fastPair_SetState(fastPairTaskData *theFastPair, fastPairState state)
{
    DEBUG_LOG("fastPair_SetState(%d)", state);

    theFastPair->prev_state = theFastPair->state;

    /* Set new state */
    theFastPair->state = state;

    /* Handle state entry functions */
    switch (state)
    {
        case FAST_PAIR_STATE_IDLE:
            fastPair_EnterIdle(theFastPair);
            break;

        case FAST_PAIR_STATE_WAIT_AES_KEY:
            fastPair_EnterWaitAESKey();
            break;

        case FAST_PAIR_STATE_WAIT_PAIRING_REQUEST:
            fastPair_EnterWaitPairingRequest();
            break;

        case FAST_PAIR_STATE_WAIT_PASSKEY:
            fastPair_EnterWaitPasskey();
            break;

        case FAST_PAIR_STATE_WAIT_ACCOUNT_KEY:
            fastPair_EnterWaitAccountKey();
            break;

        case FAST_PAIR_STATE_WAIT_ADDITIONAL_DATA:
            fastPair_EnterWaitAdditionalData();
            break;

        case FAST_PAIR_STATE_PNAME:
            fastPair_EnterPName();
            break;

        default:
            break;
    }
}


/*! \brief Get Fast Pair FSM state

Returns current state of the Fast Pair FSM.
*/
fastPairState fastPair_GetState(fastPairTaskData *theFastPair)
{
    return theFastPair->state;
}


/*! Get pointer to Fast Pair data structure */
fastPairTaskData* fastPair_GetTaskData(void)
{
    return (&fast_pair_task_data);
}

void fastPair_StartTimer(bool isQuarantine)
{
    uint16 timeout_s = 0;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
    
    timeout_s = (isQuarantine==TRUE)?FAST_PAIR_QUARANTINE_TIMEOUT:FAST_PAIR_STATE_TIMEOUT;

    DEBUG_LOG("fastPair_StartTimer timeout=[%u s]\n", timeout_s);

    /* Make sure any pending messages are cancelled */
    MessageCancelAll(&theFastPair->task, fast_pair_state_event_timer_expire);

    /* Start Fast Pair timer */
    MessageSendLater(&theFastPair->task, fast_pair_state_event_timer_expire, 0, D_SEC(timeout_s));
}

void fastPair_StopTimer(void)
{
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();

    DEBUG_LOG("fastPair_StopTimer \n");

    /* Make sure any pending messages are cancelled */
    MessageCancelAll(&theFastPair->task, fast_pair_state_event_timer_expire);
}

void fastPair_SetRetroactivelyWritingAccountKeyFlag(bool flag_value)
{
    fastPairTaskData *theFastPair;
    theFastPair = fastPair_GetTaskData();

    DEBUG_LOG("fastPair_SetRetroactivelyWritingAccountKeyFlag. Flag: %d", flag_value);
    theFastPair->retroactively_writing_account_key = flag_value;
}

void fastPair_StartTimerForWritingAccountKeyRetroactively(void)
{
    uint16 timeout_s = 0;
    fastPairTaskData *theFastPair;

    theFastPair = fastPair_GetTaskData();
    timeout_s = FAST_PAIR_RETROACTIVELY_WRITING_ACCOUNT_KEY_TIMEOUT;

    DEBUG_LOG("fastPair_StartTimerForWritingAccountKeyRetroactively. Timeout=[%u s]\n",timeout_s);
    /* Start the timer */
    MessageSendLater(&theFastPair->task, fast_pair_state_event_timer_expire, 0, D_SEC(timeout_s));
}

void fastPair_ConvertEndiannessFormat(uint8 *input_array, uint8 array_size, uint8 *output_array)
{
    for(uint8 index = 0; index < array_size; index++)
    {
        output_array[index] = input_array[array_size - index - 1];
    }
}

uint8* FastPair_GenerateRandomNonce(void)
{
    unsigned valid_nonce_idx,i;
    uint8 rand1;
    uint8* nonce = (uint8*)PanicUnlessMalloc(FAST_PAIR_RANDOM_NONCE_LEN);

    nonce[0] = UtilRandom()&0xFF;
    valid_nonce_idx = 1;
    while(valid_nonce_idx < FAST_PAIR_RANDOM_NONCE_LEN)
    {
        rand1 = UtilRandom()&0xFF;
        /* Check for duplicates */
        for(i=0;i<valid_nonce_idx;i++)
        {
            if(rand1 == nonce[i])
                 break;
        }
        /* No duplicates found*/
        if(i == valid_nonce_idx)
        {
            nonce[valid_nonce_idx] = rand1;
            valid_nonce_idx++;
        }
    }

    return nonce;
}

bool FastPair_Init(Task init_task)
{
    bool status = FALSE;
    
    fastPairTaskData *theFastPair = fastPair_GetTaskData();

    UNUSED(init_task);

    DEBUG_LOG("FastPair_Init");

    memset(theFastPair, 0, sizeof(*theFastPair));

    /* Set up task handler */
    theFastPair->task.handler = FastPair_HandleMessage;

    /* Initialise state */
    theFastPair->state = FAST_PAIR_STATE_NULL;

    fastPair_SetState(theFastPair, FAST_PAIR_STATE_NULL);

    /* Initialise retroactively writing account key flag to FALSE */
    fastPair_SetRetroactivelyWritingAccountKeyFlag(FALSE);

    /* Register with Connection Manager as observer to know BLE connections are made/destroyed */
    ConManagerRegisterTpConnectionsObserver(cm_transport_ble, &theFastPair->task);

    /* Register with Connection Manager as observer to know if handset connections are allowed or not */
    ConManagerRegisterAllowedConnectionsObserver(&theFastPair->task);

    /* Register with Physical state as observer to know if there are any physical state changes */
    appPhyStateRegisterClient(&theFastPair->task);

    /* Register for system state change indications */
    SystemState_RegisterForStateChanges(&theFastPair->task);

    /* Init the GATT Fast Pair Server library */
    status = fastPair_GattFPServerInitialize(&theFastPair->task);

#ifdef INCLUDE_CASE_COMMS
    /* register for case lid and battery notifications */
    CcWithCase_RegisterStateClient(&theFastPair->task);
#endif

#ifndef USE_SYNERGY
    /* Initialize the Fast Pair Pairing Interface */
    fastPair_PairingInit();


#ifdef INCLUDE_TWS
    /* Register for state proxy battery voltage (local & remote) events */
    StateProxy_EventRegisterClient(&theFastPair->task, state_proxy_event_type_battery_voltage);
    /* Read initial battery value */
    FastPair_LocalBatteryValInit();
#endif

    /* Get Model ID and ASPK from PS */
    fastPair_GetModelIdAspk();

    /* Initialize message stream */
    fastPair_MsgStreamInit();

    /* Initialize the Fast Pair Advertising Interface */
    fastPair_SetUpAdvertising();

    fastPair_InitSessionData();

    /* Initialize the Account Key Sync Interface */
    UserAccountsSync_Init();

    /* Initialize the Fast Pair Personalized Name Sync Interface */
    fastPair_PNameSync_Init();

    /* Ready to start Fast Pair. Move to Idle state */
    theFastPair->state = FAST_PAIR_STATE_IDLE;
#endif /* !USE_SYNERGY */

    return status;
}

