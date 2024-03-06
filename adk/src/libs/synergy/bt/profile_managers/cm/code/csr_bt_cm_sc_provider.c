/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
 ******************************************************************************/

#include "csr_synergy.h"
#ifdef EXCLUDE_CSR_BT_SC_MODULE
#include "csr_bt_cm_main.h"
#include "csr_bt_cm_util.h"
#include "csr_bt_cm_dm.h"
#include "csr_bt_cm_callback_q.h"
#include "csr_bt_td_db_sc.h"
#include "csr_bt_gatt_private_lib.h"
#include "csr_bt_addr.h"
#include "td_db_main.h"

/* ToDo: Should be defined in user config */
#ifndef CSR_BT_SC_WRITE_AUTH_ENABLE
#define CSR_BT_SC_WRITE_AUTH_ENABLE DM_SM_WAE_ACL_OWNER_NONE
#endif

static void csrBtCmMapToDmKeyType(DM_SM_KEYS_IND_T *dmPrim)
{
    static const CsrUint8 keyTypeMapping[] =
    {
        DM_SM_LINK_KEY_LEGACY,                  /* HCI_COMBINATION_KEY */
        DM_SM_LINK_KEY_LEGACY,                  /* HCI_LOCAL_UNIT_KEY */
        DM_SM_LINK_KEY_LEGACY,                  /* HCI_REMOTE_UNIT_KEY */
        DM_SM_LINK_KEY_DEBUG,                   /* HCI_DEBUG_COMBINATION_KEY */
        DM_SM_LINK_KEY_UNAUTHENTICATED_P192,    /* HCI_UNAUTHENTICATED_COMBINATION_KEY */
        DM_SM_LINK_KEY_AUTHENTICATED_P192,      /* HCI_AUTHENTICATED_COMBINATION_KEY */
        DM_SM_LINK_KEY_LEGACY,                  /* HCI_CHANGED_COMBINATION_KEY */
        DM_SM_LINK_KEY_UNAUTHENTICATED_P256,    /* HCI_UNAUTHENTICATED_COMBINATION_KEY_P256 */
        DM_SM_LINK_KEY_AUTHENTICATED_P256       /* HCI_AUTHENTICATED_COMBINATION_KEY_P256 */
    };

    if (dmPrim->keys.u[0].enc_bredr->link_key_type < sizeof(keyTypeMapping))
    {
        dmPrim->keys.u[0].enc_bredr->link_key_type = keyTypeMapping[dmPrim->keys.u[0].enc_bredr->link_key_type];
    }
    else
    {
        dmPrim->keys.u[0].enc_bredr->link_key_type = DM_SM_LINK_KEY_NONE;
    }
}

static void csrBtCmDatabaseCfmSend(CsrSchedQid appHandle,
                                   CsrBtAddressType addressType,
                                   CsrBtDeviceAddr *deviceAddr,
                                   CsrUint8 opcode,
                                   CsrUint8 keyType,
                                   CsrBtCmKey *key,
                                   CsrBtResultCode resultCode,
                                   CsrBtSupplier supplier)
{
    CsrBtCmDatabaseCfm *cfm = (CsrBtCmDatabaseCfm*) CsrPmemAlloc(sizeof(*cfm));

    cfm->type = CSR_BT_CM_DATABASE_CFM;
    cfm->addressType = addressType;
    cfm->deviceAddr = *deviceAddr;
    cfm->opcode = opcode;
    cfm->resultCode = resultCode;
    cfm->resultSupplier = supplier;
    cfm->keyType = keyType;
    cfm->key = key;

    CsrBtCmPutMessage(appHandle, cfm);
}

static void csrBtCmDbAddKeys(cmInstanceData_t *cmData,
                             CsrBtAddressType addressType,
                             const CsrBtDeviceAddr *address,
                             const CsrBtTdDbBredrKey *bredrKey,
                             const CsrBtTdDbLeKeys *leKeys,
                             CsrBool replace)
{
    DM_SM_KEYS_T keys = {0};
    CsrBtTypedAddr addrt;
    CsrUint8 keyIndex = 0;
    CsrBool devicePrivacyMode = FALSE;
    CsrUint16 present = replace ? DM_SM_KEYS_REPLACE_EXISTING : DM_SM_KEYS_UPDATE_EXISTING;

    if (bredrKey && bredrKey->linkkeyType != DM_SM_LINK_KEY_NONE)
    {
        present |= DM_SM_KEY_ENC_BREDR << (keyIndex * DM_SM_NUM_KEY_BITS);
        keys.u[keyIndex].enc_bredr = (DM_SM_KEY_ENC_BREDR_T*) CsrPmemZalloc(sizeof(DM_SM_KEY_ENC_BREDR_T));
        keys.u[keyIndex].enc_bredr->link_key_type = bredrKey->linkkeyType;
        SynMemCpyS(&keys.u[keyIndex].enc_bredr->link_key,
                   SIZE_LINK_KEY,
                   &bredrKey->linkkey,
                   SIZE_LINK_KEY);
        keyIndex++;
    }

#ifdef CSR_BT_LE_ENABLE
    if (leKeys && leKeys->keyValid)
    {
        keys.encryption_key_size = leKeys->keySize;
        keys.security_requirements = leKeys->secReq;

        if (leKeys->keyValid & CSR_BT_TD_DB_LE_KEY_ENC_CENTRAL)
        {
            present |= DM_SM_KEY_ENC_CENTRAL << (keyIndex * DM_SM_NUM_KEY_BITS);
            keys.u[keyIndex].enc_central = (DM_SM_KEY_ENC_CENTRAL_T*) CsrPmemZalloc(sizeof(DM_SM_KEY_ENC_CENTRAL_T));
            SynMemCpyS(keys.u[keyIndex].enc_central,
                       sizeof(DM_SM_KEY_ENC_CENTRAL_T),
                       &leKeys->encCentral,
                       sizeof(DM_SM_KEY_ENC_CENTRAL_T));
            keyIndex++;
        }

        if (leKeys->keyValid & CSR_BT_TD_DB_LE_KEY_DIV)
        {
            present |= DM_SM_KEY_DIV << (keyIndex * DM_SM_NUM_KEY_BITS);
            keys.u[keyIndex].div = leKeys->div;
            keyIndex++;
        }

#ifdef CSR_BT_LE_SIGNING_ENABLE
        if (leKeys->keyValid & CSR_BT_TD_DB_LE_KEY_SIGN)
        {
            present |= DM_SM_KEY_SIGN << (keyIndex * DM_SM_NUM_KEY_BITS);
            keys.u[keyIndex].sign = (DM_SM_KEY_SIGN_T *) CsrPmemZalloc(sizeof(DM_SM_KEY_SIGN_T));
            SynMemCpyS(keys.u[keyIndex].sign,
                       sizeof(DM_SM_KEY_SIGN_T),
                       &leKeys->sign,
                       sizeof(DM_SM_KEY_SIGN_T));
            keyIndex++;
        }
#endif /* CSR_BT_LE_SIGNING_ENABLE */

        if (leKeys->keyValid & CSR_BT_TD_DB_LE_KEY_ID)
        {
            present |= DM_SM_KEY_ID << (keyIndex * DM_SM_NUM_KEY_BITS);
            keys.u[keyIndex].id = (DM_SM_KEY_ID_T*) CsrPmemZalloc(sizeof(DM_SM_KEY_ID_T));
            SynMemCpyS(keys.u[keyIndex].id,
                       sizeof(DM_SM_KEY_ID_T),
                       &leKeys->id,
                       sizeof(DM_SM_KEY_ID_T));
            keyIndex++;

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
            /* Set Privacy mode only for the devices having IRK stored. Below
             * check is to make sure that stored device is privacy enabled
             * device. */
            if (cmData->leVar.llFeaturePrivacy)
            {
                if (!leKeys->rpaOnlyPresent)
                { /* Device can use either RPA or IA */
                    devicePrivacyMode = TRUE;
                }
            }
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */
        }
    }

    addrt.type = addressType;
#else
    addrt.type = CSR_BT_ADDR_PUBLIC;
#endif /* CSR_BT_LE_ENABLE */

    addrt.addr = *address;
    keys.present = present;
    CsrBtCmDmSmAddDevice(&addrt, DM_SM_TRUST_UNCHANGED, &keys);

    if (devicePrivacyMode)
    {
        dm_hci_ulp_set_privacy_mode_req(&addrt,
                                        CSR_BT_DEVICE_PRIVACY_MODE,
                                        NULL);
    }
}

static CsrBool csrBtCmDbRemoveKeys(cmInstanceData_t *cmData,
                                   CsrBtDeviceAddr deviceAddr,
                                   CsrBtAddressType addressType)
{
    CsrBtTypedAddr addrt;
    CsrBool deleted = FALSE;

    if (CsrBtBdAddrEqZero(&deviceAddr))
    {
        /* Remove all devices */
        CsrUint8 index, totalDevices = CsrBtTdDbCountDevices(), rank=0;

        /* Note that we are not sending individual debonds for deleting all non-priority devices,
         * a common debond is sent with bd address zero to indicate the same.*/
        CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                             (CSR_BT_TRANSPORT_TYPE_FLAG_BREDR |
                                              CSR_BT_TRANSPORT_TYPE_FLAG_LE),
                                             CSR_BT_ADDR_PUBLIC,
                                             &deviceAddr,
                                             CSR_BT_CM_SECURITY_EVENT_DEBOND);

        for (index = 0; index < totalDevices; index++)
        {
            if (CsrBtTdDbReadEntryByIndex(rank,
                                          CSR_BT_TD_DB_SOURCE_SC,
                                          CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                          NULL,
                                          NULL,
                                          &addrt,
                                          NULL) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
            {
                if (CsrBtTdDbDeleteDevice(addrt.type, &addrt.addr) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
                {
                    /* Device is deleted from PS, propagate same to the lower layers.*/
                    dm_hci_delete_stored_link_key(&deviceAddr, DELETE_BDADDR, NULL);
                    dm_sm_remove_device_req(CSR_BT_CM_IFACEQUEUE, &addrt, NULL);
                    /* Mark the delete flag to TRUE, this is to inform the caller that a device was actually deleted.*/
                    deleted = TRUE;
                }
                else
                {
                    /* This means that the device at the given rank cannot be deleted may be because its a priority device,
                     * increment the rank and continue deleting other devices.*/
                    rank++;
                }
            }
            else
            {
                /* For a given rank, device does not exist, stop the process.*/
                break;
            }
        }
    }
    else
    {
        addrt.addr = deviceAddr;
        addrt.type = addressType;

        if (CsrBtTdDbDeleteDevice(addrt.type, &addrt.addr) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            /* Propagate deletion to the caller.*/
            CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                                 (CSR_BT_TRANSPORT_TYPE_FLAG_BREDR |
                                                  CSR_BT_TRANSPORT_TYPE_FLAG_LE),
                                                 addrt.type,
                                                 &addrt.addr,
                                                 CSR_BT_CM_SECURITY_EVENT_DEBOND);

            /* Device is deleted from PS, propagate same to the lower layers.*/
            dm_hci_delete_stored_link_key(&deviceAddr, DELETE_BDADDR, NULL);
            dm_sm_remove_device_req(CSR_BT_CM_IFACEQUEUE, &addrt, NULL);
            /* Mark the delete flag to TRUE, this is to inform the caller that a device was actually deleted.*/
            deleted = TRUE;
        }
    }

    /* If there are no devices to be deleted,.this will act as a confirmation to the application that the API has completed.
     * In case when the devices are deleted, the confirmation received from the bluestack will be forwarded to application.*/
    if (!deleted)
    {
        CsrBtCmHciDeleteStoredLinkKeyCfm *cfm = CsrPmemZalloc(sizeof(*cfm));
        void *restoreMsg = cmData->recvMsgP;

        cfm->type               = CSR_BT_CM_HCI_DELETE_STORED_LINK_KEY_CFM;
        cfm->num_keys_deleted   = 0;
        cfm->status             = HCI_SUCCESS;
        cfm->phandle            = HCI_HANDLE_INVALID;

        cmData->recvMsgP = cfm;
        CsrBtCmHciMessagePut(cmData, CSR_BT_CM_HCI_DELETE_STORED_LINK_KEY_CFM);
        CsrPmemFree(cfm);

        cmData->recvMsgP = restoreMsg;
    }

    return deleted;
}

static CsrBtResultCode csrBtCmLeKeyUpdate(const CsrBtTypedAddr *addrt,
                                          const DM_SM_KEYS_T *keys)
{
    CsrUint8 i;
    CsrUint16 present;
    CsrBtTdDbLeKeys leKeys = { 0 };
    CsrBtResultCode result = CSR_BT_RESULT_CODE_CM_SUCCESS;

    /* Replace keys means delete all existing ones */
    if (keys->present & DM_SM_KEYS_UPDATE_EXISTING)
    {
        CsrBtTdDbGetLeKeys(addrt->type, &addrt->addr, &leKeys);
    }

    /* Store keys */
    present = keys->present;
    for (i = 0; i < DM_SM_MAX_NUM_KEYS; i++)
    {
        switch (present & DM_SM_KEY_MASK)
        {
            case DM_SM_KEY_NONE:
                /* no key, ignore */
                break;

            case DM_SM_KEY_ENC_BREDR:
                /* Handles in BR/EDR space, ignore here */
                break;

            case DM_SM_KEY_ENC_CENTRAL:
                if (keys->u[i].enc_central != NULL)
                {
                    leKeys.keyValid |= CSR_BT_TD_DB_LE_KEY_ENC_CENTRAL;
                    leKeys.keySize = keys->encryption_key_size;
                    leKeys.secReq = keys->security_requirements;
                    SynMemCpyS(&leKeys.encCentral,
                               sizeof(DM_SM_KEY_ENC_CENTRAL_T),
                               keys->u[i].enc_central,
                               sizeof(DM_SM_KEY_ENC_CENTRAL_T));
                }
                break;

            case DM_SM_KEY_DIV:
                leKeys.keyValid |= CSR_BT_TD_DB_LE_KEY_DIV;
                leKeys.div = keys->u[i].div;
                break;

#ifdef CSR_BT_LE_SIGNING_ENABLE
            case DM_SM_KEY_SIGN:
                if (keys->u[i].sign != NULL)
                {
                    leKeys.keyValid |= CSR_BT_TD_DB_LE_KEY_SIGN;
                    SynMemCpyS(&leKeys.sign,
                               sizeof(DM_SM_KEY_SIGN_T),
                               keys->u[i].sign,
                               sizeof(DM_SM_KEY_SIGN_T));
                }
                break;
#endif /* CSR_BT_LE_SIGNING_ENABLE */

            case DM_SM_KEY_ID:
                if (keys->u[i].id != NULL)
                {
                    leKeys.keyValid |= CSR_BT_TD_DB_LE_KEY_ID;
                    SynMemCpyS(&leKeys.id,
                               sizeof(DM_SM_KEY_ID_T),
                               keys->u[i].id,
                               sizeof(DM_SM_KEY_ID_T));
                }
                break;

            default:
                break;
        }

        present >>= DM_SM_NUM_KEY_BITS;
    }

    result = CsrBtTdDbSetLeKeys(addrt->type, &addrt->addr, &leKeys);

    return result;
}

static void csrBtCmBredrKeyUpdate(cmInstanceData_t *cmData, DM_SM_KEYS_IND_T *dmPrim)
{
    CsrBtAddressType addrType;
    CsrBtDeviceAddr *addr;
    CsrBtTdDbBredrKey bredrKey = { 0 };
    CsrBtResultCode result = CSR_BT_RESULT_CODE_CM_SUCCESS;
    CsrBool added = FALSE;

    CSR_UNUSED(cmData);

#ifdef CSR_BT_LE_ENABLE
    if (!CsrBtBdAddrEqZero(&dmPrim->id_addrt.addr))
    { /* Non-zero ID address available */
        addr = &dmPrim->id_addrt.addr;
        addrType = dmPrim->id_addrt.type;
    }
    else
#endif
    {
        addr = &dmPrim->addrt.addr;
        addrType = dmPrim->addrt.type;
    }

    bredrKey.linkkeyLen = SIZE_LINK_KEY;
    bredrKey.linkkeyType = (CsrUint8) dmPrim->keys.u[0].enc_bredr->link_key_type;

    SynMemCpyS(&bredrKey.linkkey,
               SIZE_LINK_KEY,
               &dmPrim->keys.u[0].enc_bredr->link_key,
               SIZE_LINK_KEY);
    bredrKey.authorised = FALSE;

    result = CsrBtTdDbSetBredrKey(addrType, addr, &bredrKey);

    if (result == CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE)
    {
        CsrBtTypedAddr addrt;
        CsrBool isPriorityDevice;
        CsrUint8 index;

        for(index = CsrBtTdDbCountDevices() - 1; index >= 0; index--)
        {
            CsrBtTdDbReadEntryByIndex(index,
                                      CSR_BT_TD_DB_SOURCE_SC,
                                      CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                      NULL,
                                      NULL,
                                      &addrt,
                                      &isPriorityDevice);

            if(!isPriorityDevice)
            {
                (void)csrBtCmDbRemoveKeys(cmData, addrt.addr, addrt.type);
                result = CsrBtTdDbSetBredrKey(addrType, addr, &bredrKey);
                added = TRUE;
                break;
            }
        }

        if(!added)
        {
            /* we didn't find any devices to replace, send failure */
            CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                                 CSR_BT_TRANSPORT_TYPE_FLAG_BREDR,
                                                 addrType,
                                                 addr,
                                                 CSR_BT_CM_SECURITY_EVENT_DEVICE_UPDATE_FAILED);
        }
    }
    
    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
         /* make the entry most recently used */
         CsrBtTdDbPrioritiseDevice(addrType, addr, CSR_BT_TD_DB_UPDATE_FLAG_MRU);
    }
}

static CsrBtResultCode csrBtCmDbWriteKeys(cmInstanceData_t *cmData,
                                          const CsrBtDeviceAddr *deviceAddr,
                                          CsrBtAddressType addressType,
                                          CsrBtCmKeyType keyType,
                                          const void *keys)
{
    CsrBtResultCode result = CSR_BT_RESULT_CODE_CM_SUCCESS;
    CsrBtTdDbBredrKey *bredrKey = NULL;
    CsrBtTdDbLeKeys *leKeys = NULL;

    if (keyType == CSR_BT_TD_DB_SC_KEY_BREDR_KEY)
    {
        result = CsrBtTdDbSetBredrKey(addressType,
                                      deviceAddr,
                                      keys);
        bredrKey = (CsrBtTdDbBredrKey *)keys;
    }
#ifdef CSR_BT_LE_ENABLE
    else if (keyType == CSR_BT_TD_DB_SC_KEY_LE_KEYS)
    {
        result = CsrBtTdDbSetLeKeys(addressType,
                                    deviceAddr,
                                    keys);
        leKeys = (CsrBtTdDbLeKeys *)keys;
    }
#endif
    else
    {
        result = CSR_BT_RESULT_CODE_TD_DB_INVALID_KEY;
    }

    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        csrBtCmDbAddKeys(cmData,
                         addressType,
                         deviceAddr,
                         bredrKey,
                         leKeys,
                         FALSE);
    }

    return result;
}

static CsrBtResultCode csrBtCmDbReplaceKeys(cmInstanceData_t *cmData,
                                            const CsrBtDeviceAddr *deviceAddr,
                                            CsrBtAddressType addressType,
                                            CsrBtCmKeyType keyType,
                                            const void *keys)
{
    CsrBtResultCode result = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    CsrBtTypedAddr addrt;
    CsrBool isPriorityDevice, added = FALSE;
    CsrUint8 index;

    for(index = CsrBtTdDbCountDevices() - 1; index >= 0; index--)
    {
        CsrBtTdDbReadEntryByIndex(index,
                                  CSR_BT_TD_DB_SOURCE_SC,
                                  CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                  NULL,
                                  NULL,
                                  &addrt,
                                  &isPriorityDevice);

        if(!isPriorityDevice)
        {
            (void)csrBtCmDbRemoveKeys(cmData, addrt.addr, addrt.type);
            result = csrBtCmDbWriteKeys(cmData,
                                        deviceAddr,
                                        addressType,
                                        keyType,
                                        keys);
            added = TRUE;
            break;
        }
    }

    if(!added)
    {
        /* we didn't find any devices to replace, send failure */
        CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                             CSR_BT_TRANSPORT_TYPE_FLAG_BREDR,
                                             addressType,
                                             deviceAddr,
                                             CSR_BT_CM_SECURITY_EVENT_DEVICE_UPDATE_FAILED);
    }
    return result;
}

static CsrBtResultCode csrBtCmDatabaseWrite(cmInstanceData_t *cmData,
                                            const CsrBtDeviceAddr *deviceAddr,
                                            CsrBtAddressType addressType,
                                            CsrBtCmKeyType keyType,
                                            const void *keys)
{
    CsrBtResultCode result;

    result = csrBtCmDbWriteKeys(cmData,
                                deviceAddr,
                                addressType,
                                keyType,
                                keys);

    if (result == CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE)
    {
        result = csrBtCmDbReplaceKeys(cmData,
                                      deviceAddr,
                                      addressType,
                                      keyType,
                                      keys);
    }

    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        /* make the entry most recently used */
        CsrBtTdDbPrioritiseDevice(addressType, deviceAddr, CSR_BT_TD_DB_UPDATE_FLAG_MRU);
    }
    
    return result;
}

static void csrBtCmEventDispatch(cmInstanceData_t *cmData,
                                 CsrSchedQid appHandle,
                                 void *pContext1,
                                 void *pContext2)
{
    CSR_UNUSED(cmData);
    CsrBtCmPutMessage(appHandle, CsrMemDup(pContext1, (CsrUint8)((CsrUint32)pContext2 & 0xFF)));
}

static CsrBtResultCode csrBtAddDeviceByIndex(cmInstanceData_t *cmData,
                                             CsrUint8 deviceIndex,
                                             CsrBool replace)
{
    CsrBtTypedAddr addrt;

    if (deviceIndex < CSR_BT_TD_DB_LIST_MAX &&
        CsrBtTdDbReadEntryByIndex(deviceIndex,
                                  CSR_BT_TD_DB_SOURCE_SC,
                                  CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    { /* Device record found */
        CsrBtTdDbBredrKey bredrKey = { 0 };
#ifdef CSR_BT_LE_ENABLE
        CsrBtTdDbLeKeys leKeys = { 0 };
#endif

        (void) CsrBtTdDbGetEntryByIndex(deviceIndex,
                                        CSR_BT_TD_DB_SOURCE_SC,
                                        CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                        sizeof(bredrKey),
                                        &bredrKey,
                                        &addrt);
#ifdef CSR_BT_LE_ENABLE
        (void) CsrBtTdDbGetEntryByIndex(deviceIndex,
                                        CSR_BT_TD_DB_SOURCE_SC,
                                        CSR_BT_TD_DB_SC_KEY_LE_KEYS,
                                        sizeof(leKeys),
                                        &leKeys,
                                        &addrt);
#endif /* CSR_BT_LE_ENABLE */

        if (!bredrKey.linkkeyLen)
        {
            bredrKey.linkkeyType = DM_SM_LINK_KEY_NONE;
        }

        /* Send device record to Bluestack */
        csrBtCmDbAddKeys(cmData,
                         addrt.type,
                         &addrt.addr,
                         &bredrKey,
#ifdef CSR_BT_LE_ENABLE
                         &leKeys,
#else
                         NULL,
#endif
                         replace);

        return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
    }

    return CSR_BT_RESULT_CODE_TD_DB_TASK_FAILED;
}

static CsrBtResultCode cmDbRemoveDeviceExt(cmInstanceData_t *cmData,
                                           CsrBtDeviceAddr *deviceAddr,
                                           CsrBtAddressType addressType,
                                           CsrUint8 *numDeletedDevices)
{
    CsrBtTypedAddr addrt;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    CsrUint8 deleteCounter = 0;

    /* Application wants to delete device from Bluestack SM DB as well as PS */
    if (CsrBtBdAddrEqZero(deviceAddr))
    {
        /* Remove all non priority devices */
        CsrUint8 index, totalDevices = CsrBtTdDbCountDevices(), rank=0;

        for (index = 0; index < totalDevices; index++)
        {
            if (CsrBtTdDbReadEntryByIndex(rank,
                                          CSR_BT_TD_DB_SOURCE_SC,
                                          CSR_BT_TD_DB_SC_KEY_BREDR_KEY,
                                          NULL,
                                          NULL,
                                          &addrt,
                                          NULL) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
            {
                resultCode =  CsrBtTdDbDeleteDevice(addrt.type, &addrt.addr);
                if (resultCode == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
                {
                    /* Send individual debond event on deleting device from TD DB */
                    CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                                        (CSR_BT_TRANSPORT_TYPE_FLAG_BREDR |
                                                         CSR_BT_TRANSPORT_TYPE_FLAG_LE),
                                                         addrt.type,
                                                         &addrt.addr,
                                                         CSR_BT_CM_SECURITY_EVENT_DEBOND);

                    /* Delete the device from SM DB.*/
                    dm_sm_remove_device_req(CSR_BT_CM_IFACEQUEUE, &addrt, NULL);

                    /* Increment the deviceIndex counter to keep track of number of remove device requests being sent to Bluestack */
                    deleteCounter++;
                }
                else
                {
                    /* This means that the device at the given rank cannot be deleted may be because its a priority device,
                     * increment the rank and continue deleting other devices.*/
                    rank++;
                }
            }
            else
            {
                /* For a given rank, device does not exist, stop the process.*/
                break;
            }
        }
    }
    else
    {
        addrt.addr = (*deviceAddr);
        addrt.type = addressType;
        
        resultCode =  CsrBtTdDbDeleteDevice(addrt.type, &addrt.addr);
        if (resultCode == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            /* Send debond event on deleting device from TD DB */
            CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                                 (CSR_BT_TRANSPORT_TYPE_FLAG_BREDR |
                                                  CSR_BT_TRANSPORT_TYPE_FLAG_LE),
                                                 addrt.type,
                                                 &addrt.addr,
                                                 CSR_BT_CM_SECURITY_EVENT_DEBOND);

            /* Delete the device from SM DB.*/
            dm_sm_remove_device_req(CSR_BT_CM_IFACEQUEUE, &addrt, NULL);
            
            /* Increment the deviceIndex counter to keep track of number of remove device requests being sent to Bluestack */
            deleteCounter++;
         }
     }

     if(numDeletedDevices)
     {
         (*numDeletedDevices) = deleteCounter;
     }

    return resultCode;
}

void CsrBtCmPropgateSecurityEventIndEvent(cmInstanceData_t *cmData,
                                          CsrBtTransportMask transportMask,
                                          CsrBtAddressType addressType,
                                          const CsrBtDeviceAddr *deviceAddr,
                                          CsrBtCmSecurityEvent event)
{
    CsrBtCmSecurityEventInd ind;

    ind.type = CSR_BT_CM_SECURITY_EVENT_IND;
    ind.transportMask = transportMask;
    ind.addrt.type = addressType;
    ind.addrt.addr = *deviceAddr;
    ind.event = event;

    CsrBtCmPropgateEvent(cmData,
                         csrBtCmEventDispatch,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_SECURITY_EVENT_IND,
                         HCI_SUCCESS,
                         &ind,
                         (void *) sizeof(ind));
}

void CsrBtCmLeUpdateLocalDbKeys(DM_SM_INIT_CFM_T *init)
{
    CsrBtTdDbSystemInfo systemInfo = { 0 };

    CsrBtTdDbGetSystemInfo(&systemInfo);

    systemInfo.div = init->sm_div_state;
    systemInfo.signCounter = init->sm_sign_counter;
    SynMemCpyS(&systemInfo.er, sizeof(systemInfo.er), init->sm_key_state.er, sizeof(systemInfo.er));
    SynMemCpyS(&systemInfo.ir, sizeof(systemInfo.ir), init->sm_key_state.ir, sizeof(systemInfo.ir));

    CsrBtTdDbSetSystemInfo(&systemInfo);
}

void CsrBtCmDmSmAddDevice(CsrBtTypedAddr *addrt,
                          DM_SM_TRUST_T trust,
                          DM_SM_KEYS_T *keys)
{
    dm_sm_add_device_req(NULL,
                         CSR_BT_CM_IFACEQUEUE,
                         addrt,
                         trust,
                         keys->security_requirements,
                         keys->encryption_key_size,
                         keys->present,
                         keys->u[0],
                         keys->u[1],
                         keys->u[2],
                         keys->u[3],
                         keys->u[4]);
}

void CsrBtCmSmDbAddDeviceIndex(cmInstanceData_t *cmData, CsrUint8 deviceIndex)
{
    if(csrBtAddDeviceByIndex(cmData, deviceIndex, TRUE) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        /* Save the device index being added */
        cmData->scVar.deviceIndex = deviceIndex;
    }
    else
    {
        /* CM init sequence completed */
        CmInitSequenceHandler(cmData,
                              CM_INIT_SM_ADD_DEVICE_COMPLETE,
                              CSR_BT_RESULT_CODE_CM_SUCCESS,
                              CSR_BT_SUPPLIER_CM);
    }
}

void CsrBtCmDmSmInit(cmInstanceData_t *cmData)
{
    CsrUint16 options, config;
    CsrBtTdDbSystemInfo systemInfo = { 0 };

    CSR_UNUSED(cmData);
    config = DM_SM_SEC_MODE_CONFIG_LEGACY_AUTO_PAIR_MISSING_LINK_KEY;

    /* VM products prefer auto accepting pairing */
#ifndef CSR_TARGET_PRODUCT_VM
    config |= DM_SM_SEC_MODE_CONFIG_NEVER_AUTO_ACCEPT;
#endif

#ifdef CSR_BT_LE_ENABLE
    {
        const CsrUint8 nullBuf[sizeof(CsrBtTdDbSystemInfo)] = { 0 };

        CsrBtTdDbGetSystemInfo(&systemInfo);
        if (CsrMemCmp(&systemInfo, &nullBuf, sizeof(systemInfo)))
        {
            /* Root Keys are available, set DM_SM_INIT_SM_STATE bit */
            options = DM_SM_INIT_ALL_SC;
        }
        else
        {
            /* Root keys don't exists, request to generate root keys (ER and IR) */
            options = DM_SM_INIT_ALL_SC_AND_RESET_ROOT_KEYS;
        }
    }
#else
    options = DM_SM_INIT_ALL_SC_BREDR;
#endif

    if (cmData->options & CM_SECURITY_CONFIG_OPTION_SC_ONLY)
    {
        CSR_MASK_SET(options, DM_SM_INIT_CONFIG);
        config |= DM_SM_SEC_MODE_CONFIG_SC_ONLY_MODE;

        /* Secure connections needs to be enabled when the SC only flag is set. */
        CSR_MASK_SET(options, DM_SM_INIT_SECURE_CONNECTIONS);
    }
    else
    {
        /* Application can set the security options with the help of
         * API CmScSetSecurityConfigReq, see CmSecurityConfigOptions
         * in csr_bt_cm_prim.h for more information on other options. */
        if (cmData->options & CM_SECURITY_CONFIG_OPTION_SECURE_CONNECTIONS)
        {
            /* Enable secure connections as indicated by application. */
            CSR_MASK_SET(options, DM_SM_INIT_SECURE_CONNECTIONS);
        }
        else
        {
            /* Application has not indicated support for secure connections,
             * hence disable it. The same can be eanbled with the use of 
             * DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ primitive, refer to dm_prim.h */
            CSR_MASK_UNSET(options, DM_SM_INIT_SECURE_CONNECTIONS);
        }
    }

    if (cmData->options & CM_SECURITY_CONFIG_OPTION_DISABLE_CTKD)
    {
        CSR_MASK_SET(options, DM_SM_INIT_CONFIG);
        config |= DM_SM_SEC_MODE_CONFIG_DISABLE_CTKD;
    }

    if (cmData->options & CM_SECURITY_CONFIG_OPTION_SELECTIVE_CTKD)
    {
        CSR_MASK_SET(options, DM_SM_INIT_CONFIG);
        config |= DM_SM_SEC_MODE_CONFIG_SELECTIVE_CTKD;
    }

    /* Set default security level, mode and other bits and bobs */
    dm_sm_init_req_le(options,
                      SEC_MODE4_SSP,
                      0, /* default security level, 0 because of SDP */
                      config,
                      CSR_BT_SC_WRITE_AUTH_ENABLE,
                      CSR_BT_DEFAULT_ENC_MODE3,
                      (DM_SM_KEY_STATE_T *) &systemInfo, /* sm_key_state */
                      systemInfo.div,
                      systemInfo.signCounter,
                      NULL);
}

void CsrBtCmLeKeysHandler(cmInstanceData_t *cmData)
{
    DM_SM_KEYS_IND_T *dmPrim = (DM_SM_KEYS_IND_T*) cmData->recvMsgP;
    CsrBtTypedAddr *peerIdAddrt = NULL;
    CsrBtResultCode result = CSR_BT_RESULT_CODE_CM_SUCCESS;
    CsrBtTypedAddr *peerAddrt = &dmPrim->addrt;
    CsrBtTypedAddr *addrt = &dmPrim->addrt;
    CsrBool added = FALSE;

    /* Check whether valid ID address also available */
    if (!CsrBtBdAddrEqZero(&dmPrim->id_addrt.addr))
    {
        peerIdAddrt = &dmPrim->id_addrt;
        addrt = peerIdAddrt;
    }

    /* If peer's device address is NRPA or if it is RPA but no corresponding ID
     * address present, do not create SCDB record for it */
    if (CsrBtAddrIsPrivateNonresolvable(peerAddrt))
    {
        /* Do not create TD_DB record for the device using NRPA */
    }
    else if (CsrBtAddrIsPrivateResolvable(peerAddrt) &&
             (!peerIdAddrt || CsrBtBdAddrEqZero(&peerIdAddrt->addr)))
    {
        /* If peer address is RPA but no IA received or received but all fields
         * are zeros, no point of creating TD_DB record */
    }
    else
    { /* Else update everything.This is the case for pulic, RPA or static
       * address */
        result = csrBtCmLeKeyUpdate(addrt, &dmPrim->keys);

        if (result == CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE)
        {
            CsrBtTypedAddr removeAddrt;
            CsrBool isPriorityDevice;
            CsrUint8 index;

            for(index = CsrBtTdDbCountDevices() - 1; index >= 0; index--)
            {
                CsrBtTdDbReadEntryByIndex(index,
                                          CSR_BT_TD_DB_SOURCE_SC,
                                          CSR_BT_TD_DB_SC_KEY_LE_KEYS,
                                          NULL,
                                          NULL,
                                          &removeAddrt,
                                          &isPriorityDevice);
                if(!isPriorityDevice)
                {
                    (void)csrBtCmDbRemoveKeys(cmData, removeAddrt.addr, removeAddrt.type);
                    result = csrBtCmLeKeyUpdate(addrt, &dmPrim->keys);
                    added = TRUE;
                    break;
                }
            }

            if(!added)
            {
                /* we didn't find any devices to replace, send failure */
                CsrBtCmPropgateSecurityEventIndEvent(cmData,
                                                     CSR_BT_TRANSPORT_TYPE_FLAG_BREDR,
                                                     addrt->type,
                                                     &addrt->addr,
                                                     CSR_BT_CM_SECURITY_EVENT_DEVICE_UPDATE_FAILED);
            }
        }

        if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            /* make the entry most recently used */
            CsrBtTdDbPrioritiseDevice(addrt->type, &addrt->addr, CSR_BT_TD_DB_UPDATE_FLAG_MRU);
        }
    }
}

void CsrBtCmBredrKeysHandler(cmInstanceData_t *cmData)
{
    DM_SM_KEYS_IND_T *dmPrim = (DM_SM_KEYS_IND_T*) cmData->recvMsgP;

    /* An all-zero address is debug info from
     * the DM/SM - discard of those */
    if (CsrBtBdAddrEqZero(&dmPrim->addrt.addr))
    {
        return;
    }

    csrBtCmMapToDmKeyType(dmPrim);
    csrBtCmBredrKeyUpdate(cmData, dmPrim);
}

void CsrBtCmDatabaseReqHandler(cmInstanceData_t *cmData)
{
    CsrBool unlock = TRUE;
    CsrUint8 *keys = NULL;
    CsrBtResultCode result = CSR_BT_RESULT_CODE_CM_SUCCESS;
    CsrBtSupplier supplier = CSR_BT_SUPPLIER_CM;
    CsrBtCmDatabaseReq *req = (CsrBtCmDatabaseReq*) cmData->recvMsgP;

    if (req->opcode == CSR_BT_CM_DB_OP_WRITE)
    {
        result = csrBtCmDatabaseWrite(cmData,
                                      &req->deviceAddr,
                                      req->addressType,
                                      req->keyType,
                                      req->key);

        if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            /* Wait for DM_SM_ADD_DEVICE_CFM to unlock the DM queue */
            unlock = FALSE;
        }
        else
        {
            supplier = CSR_BT_SUPPLIER_TD_DB;
        }
    }
    else if (req->opcode == CSR_BT_CM_DB_OP_READ)
    {
        if (CsrBtTdDbDeviceExists(req->addressType, &req->deviceAddr))
        {
            if (req->keyType == CSR_BT_TD_DB_SC_KEY_BREDR_KEY)
            {
                keys = CsrPmemZalloc(sizeof(CsrBtCmKey));
                result = CsrBtTdDbGetBredrKey(req->addressType,
                                              &req->deviceAddr,
                                              keys);
            }
#ifdef CSR_BT_LE_ENABLE
            else if (req->keyType == CSR_BT_TD_DB_SC_KEY_LE_KEYS)
            {
                keys = CsrPmemZalloc(sizeof(CsrBtCmKey));
                result = CsrBtTdDbGetLeKeys(req->addressType,
                                            &req->deviceAddr,
                                            keys);
            }
#endif
            else
            {
                result = CSR_BT_RESULT_CODE_TD_DB_INVALID_KEY;
            }

            if (result != CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
            {
                supplier = CSR_BT_SUPPLIER_TD_DB;
            }
        }
        else
        {
            result = CSR_BT_RESULT_CODE_CM_UNKNOWN_DEVICE;
        }
    }

    /* ToDo: Confirmation should be sent after the device have added
     * in case of write operation */
    csrBtCmDatabaseCfmSend(req->appHandle,
                           req->addressType,
                           &req->deviceAddr,
                           req->opcode,
                           req->keyType,
                           (CsrBtCmKey*) keys,
                           result,
                           supplier);

    if (unlock)
    {
        CsrBtCmDmLocalQueueHandler();
    }
    else
    {
        /* Mark this as an external add device request, which will require unlocking of dm queue
         * at the time of handling the confirmation. */
        cmData->scVar.locked = TRUE;
    }
}

void CsrBtCmSmRemoveDeviceReqHandler(cmInstanceData_t *cmData)
{
    CsrBtCmSmRemoveDeviceReq *cmPrim = (CsrBtCmSmRemoveDeviceReq*) cmData->recvMsgP;

    if (!csrBtCmDbRemoveKeys(cmData, cmPrim->deviceAddr, cmPrim->addressType))
    {
        /* Since no devices are deleted we need to free the dm queue here.*/
        CsrBtCmDmLocalQueueHandler();
    }
}

#ifdef CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT
static void csrBtCmGattReadRemoteRpaOnlyCharCfmHandler(cmInstanceData_t *cmData)
{
    CsrBtTdDbLeKeys leKeys;
    CsrBtGattReadRemoteRpaOnlyCharCfm *prim;
    CsrBtPrivacyMode mode;
    CsrBtTypedAddr addrt;
    CsrBool present = FALSE;

    prim = (CsrBtGattReadRemoteRpaOnlyCharCfm *) cmData->recvMsgP;

    if (prim->resultCode == CSR_BT_GATT_RESULT_SUCCESS &&
        prim->resultSupplier == CSR_BT_SUPPLIER_GATT)
    { /* RPA only characteristic of peer device read successfully */
        if (!prim->rpaOnlyValue)
        { /* prim->rpaOnlyValue = 0; peer will only use RPA as local
           * addresses after bonding so set the privacy mode as network */
            mode = CSR_BT_NETWORK_PRIVACY_MODE;
            present = TRUE;
        }
        else
        { /* else prim->rpaOnlyValue = 1 - 255; reserved for future use */
            /* Cannot assume that device would always use RPA,
             * so set it to device privacy mode */
            mode = CSR_BT_DEVICE_PRIVACY_MODE;
        }
    }
    else
    {
        /* RPA Only characteristic is not present, so it cannot be assumed
         * that only RPA will be used over the air so set the privacy mode
         * as device */
        mode = CSR_BT_DEVICE_PRIVACY_MODE;
    }

    /* Always use the IA to set the privacy mode */
    addrt.addr = prim->address.addr;
    addrt.type = prim->address.type;

    if (CsrBtTdDbGetLeKeys(addrt.type,
                           &addrt.addr,
                           &leKeys) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        /* Keep the RPA only support info stored in SC DB */
        leKeys.rpaOnlyPresent = present;
        (void) CsrBtTdDbSetLeKeys(addrt.type, &addrt.addr, &leKeys);

        /* Set the privacy mode in controller */
        CsrBtCmLeSetPrivacyModeReqSend(CSR_BT_CM_IFACEQUEUE, addrt, mode);
    }
}

void CsrBtCmGattArrivalHandler(cmInstanceData_t *cmData)
{
    CsrBtGattPrim *prim = (CsrBtGattPrim *) cmData->recvMsgP;

    switch (*prim)
    {
        case CSR_BT_GATT_READ_REMOTE_RPA_ONLY_CHAR_CFM:
            csrBtCmGattReadRemoteRpaOnlyCharCfmHandler(cmData);
            break;

        default:
            /* Log exception */
            break;
    }

    CsrBtGattFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, cmData->recvMsgP);
    CsrBtGattPrivateFreeUpstreamMessageContents(CSR_BT_GATT_PRIM, cmData->recvMsgP);
}
#endif /* CSR_BT_INSTALL_LE_PRIVACY_1P2_SUPPORT */

void CsrBtCmDatabaseReqSend(CsrSchedQid appHandle,
                            CsrBtAddressType addressType,
                            const CsrBtDeviceAddr *deviceAddr,
                            CsrUint8 opcode,
                            CsrBtCmKeyType keyType,
                            CsrBtCmKey *key)
{
    CsrBtCmDatabaseReq *msg = (CsrBtCmDatabaseReq *) CsrPmemAlloc(sizeof(*msg));

    msg->type = CSR_BT_CM_DATABASE_REQ;
    msg->appHandle = appHandle;
    msg->addressType = addressType;
    msg->deviceAddr = *deviceAddr;
    msg->opcode = opcode;
    msg->keyType = keyType;
    msg->key = key;

    CsrBtCmMsgTransport(msg);
}

void CsrBtScMapSecInLevel(CsrUint16 secInput, CsrUint16 *secOutput)
{
    *secOutput = SECL4_IN_SSP | /* Always set SSP. This is ignored if we're not in mode 4 */
        ((secInput & CSR_BT_SEC_AUTHENTICATION) ? SECL_IN_AUTHENTICATION : 0) |
        ((secInput & CSR_BT_SEC_AUTHORISATION)  ? SECL_IN_AUTHORISATION : 0) |
        ((secInput & CSR_BT_SEC_ENCRYPTION)     ? SECL_IN_ENCRYPTION: 0) |
        ((secInput & CSR_BT_SEC_MITM)           ? SECL4_IN_MITM : 0) |
        ((secInput & CSR_BT_SEC_SC)             ? SECL4_IN_P256_ONLY: 0);
}

/* Security Action Mask*/
#define SEC_ACTION_MASK     (CSR_BT_SEC_DEFAULT | CSR_BT_SEC_MANDATORY | CSR_BT_SEC_SPECIFY)

CsrBtResultCode CsrBtScSetSecInLevel(CsrUint16 *secInLevel, CsrUint16 secLevel,
                                     CsrUint16 secManLevel, CsrUint16 secDefLevel,
                                     CsrBtResultCode successCode, CsrBtResultCode errorCode)
{
    CsrBtResultCode rval;
    CsrUint8 action;

    action = secLevel & SEC_ACTION_MASK;

    switch(action)
    {
        case CSR_BT_SEC_DEFAULT:
            CsrBtScMapSecInLevel(secDefLevel, secInLevel);
            rval = successCode;
            break;
        case CSR_BT_SEC_MANDATORY:
            CsrBtScMapSecInLevel(secManLevel, secInLevel);
            rval = successCode;
            break;
        case CSR_BT_SEC_SPECIFY:
            if ((secLevel & secManLevel) == secManLevel)
            {
                CsrBtScMapSecInLevel(secLevel, secInLevel);
                rval = successCode;
                break;
            }
            /* !! FALL THROUGH !! */
        default:
            /* Use mandatory settings if invalid action is requested */
            CsrBtScMapSecInLevel(secManLevel, secInLevel);
            rval = errorCode;
            break;
    }

    return rval;
}

void CsrBtScMapSecOutLevel(CsrUint16 secInput, CsrUint16 *secOutput)
{
    *secOutput = SECL4_OUT_SSP | /* Always set SSP. This is ignored if we're not in mode 4 */
        ((secInput & CSR_BT_SEC_AUTHENTICATION) ? SECL_OUT_AUTHENTICATION : 0) |
        ((secInput & CSR_BT_SEC_AUTHORISATION)  ? SECL_OUT_AUTHORISATION : 0) |
        ((secInput & CSR_BT_SEC_ENCRYPTION)     ? SECL_OUT_ENCRYPTION: 0) |
        ((secInput & CSR_BT_SEC_MITM)           ? SECL4_OUT_MITM : 0) |
        ((secInput & CSR_BT_SEC_SC)             ? SECL4_OUT_P256_ONLY : 0);
}

CsrBtResultCode CsrBtScSetSecOutLevel(CsrUint16 *secOutLevel,
                                      CsrUint16 secLevel,
                                      CsrUint16 secManLevel,
                                      CsrUint16 secDefLevel,
                                      CsrBtResultCode successCode,
                                      CsrBtResultCode errorCode)
{
    CsrBtResultCode rval;
    CsrUint8 action;

    action = secLevel & SEC_ACTION_MASK;

    switch(action)
    {
        case CSR_BT_SEC_DEFAULT:
            CsrBtScMapSecOutLevel(secDefLevel, secOutLevel);
            rval = successCode;
            break;
        case CSR_BT_SEC_MANDATORY:
            CsrBtScMapSecOutLevel(secManLevel, secOutLevel);
            rval = successCode;
            break;
        case CSR_BT_SEC_SPECIFY:
            if ((secLevel & secManLevel) == secManLevel)
            {
                CsrBtScMapSecOutLevel(secLevel, secOutLevel);
                rval = successCode;
                break;
            }
            /* !! FALL THROUGH !! */
        default:
            /* Use mandatory settings if invalid action is requested */
            CsrBtScMapSecOutLevel(secManLevel, secOutLevel);
            rval = errorCode;
            break;
    }

    return rval;
}

CsrBool CsrBtScDeviceAuthorised(CsrBtAddressType addrType,
                                const CsrBtDeviceAddr *addr)
{
    CsrBtTdDbBredrKey bredrKey;
    CsrBool authorised = FALSE;

    bredrKey.authorised = FALSE;

    if (CsrBtTdDbGetBredrKey(addrType,
                             addr,
                             &bredrKey) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        authorised = bredrKey.authorised;
    }

    return authorised;
}

/* Updates application as default handler for SC messages */
void CsrBtScActivateReqSend(CsrSchedQid appHandle)
{
    csrBtCmData.scHandle = appHandle;
}

CsrBtResultCode CsrBtCmDatabaseReqSendNow(const CsrBtDeviceAddr *deviceAddr,
                                          CsrBtAddressType addressType,
                                          CsrBtCmKeyType keyType,
                                          const void *keys)
{
    return csrBtCmDatabaseWrite(CmGetInstanceDataPtr(),
                                deviceAddr,
                                addressType,
                                keyType,
                                keys);
}

void CsrBtCmScRefreshAllDevicesNow(void)
{
    CsrUint8 i;

    for(i = 0; i < CSR_BT_TD_DB_LIST_MAX; i++)
    {
        /* The device indicies which are present, wil get updated in bluestack */
        csrBtAddDeviceByIndex(CmGetInstanceDataPtr(), i, FALSE);
    }
}

void CmScSetSecurityConfigReq(CmSecurityConfigOptions securityConfigOptions)
{
    csrBtCmData.options = securityConfigOptions;
}

#ifdef CSR_BT_LE_SIGNING_ENABLE
/* Signing counter update for local or remote device. Must write to
 * SC_DB immediately */
void CmDmSmCsrkCounterChangeIndHandler(cmInstanceData_t *cmData)
{
    DM_SM_CSRK_COUNTER_CHANGE_IND_T *csrk = (DM_SM_CSRK_COUNTER_CHANGE_IND_T *) cmData->recvMsgP;

    if (csrk->local_csrk)
    {
        CsrBtTdDbSystemInfo systemInfo;

        if (CsrBtTdDbGetSystemInfo(&systemInfo) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            systemInfo.signCounter = csrk->counter;
            CsrBtTdDbSetSystemInfo(&systemInfo);
        }
    }
    else
    {
        CsrBtTdDbLeKeys leKeys;

        if (CsrBtTdDbGetLeKeys(csrk->addrt.type,
                               &csrk->addrt.addr,
                               &leKeys) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            leKeys.sign.counter = csrk->counter;
            CsrBtTdDbSetLeKeys(csrk->addrt.type, &csrk->addrt.addr, &leKeys);
        }
    }
}
#endif /* CSR_BT_LE_SIGNING_ENABLE */

void CmDmRemoveDeviceKeyConfirmSend(cmInstanceData_t *cmData,
                                    CsrBtDeviceAddr *deviceAddr,
                                    CsrBtAddressType addressType,
                                    CsrBtResultCode resultCode,
                                    CsrBtSupplier resultSupplier)
{
    CmDmRemoveDeviceKeyCfm *cfm = (CmDmRemoveDeviceKeyCfm *)CsrPmemZalloc(sizeof(*cfm));

    cfm->type           = CM_DM_REMOVE_DEVICE_KEY_CFM;
    cfm->deviceAddr     = *deviceAddr;
    cfm->addressType    = addressType;
    cfm->resultCode     = resultCode;
    cfm->resultSupplier = resultSupplier;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
}

void CmDmRemoveDeviceOptionsConfirmSend(cmInstanceData_t *cmData,
                                        CsrBtDeviceAddr *deviceAddr,
                                        CsrBtAddressType addressType,
                                        CsrBtResultCode resultCode,
                                        CsrBtSupplier resultSupplier)
{
    CmDmRemoveDeviceOptionsCfm *cfm = (CmDmRemoveDeviceOptionsCfm *)CsrPmemZalloc(sizeof(*cfm));

    cfm->type           = CM_DM_REMOVE_DEVICE_OPTIONS_CFM;
    cfm->deviceAddr     = *deviceAddr;
    cfm->addressType    = addressType;
    cfm->resultCode     = resultCode;
    cfm->resultSupplier = resultSupplier;

    CsrBtCmPutMessage(cmData->dmVar.appHandle, cfm);
}


void CmDmRemoveDeviceKeyReqHandler(cmInstanceData_t *cmData)
{
    CmDmRemoveDeviceKeyReq *req = (CmDmRemoveDeviceKeyReq *) cmData->recvMsgP;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_CM_SUCCESS;
    CsrBtSupplier resultSupplier = CSR_BT_SUPPLIER_CM;

    /* Storing the app handle for sending confirmations.*/
    cmData->dmVar.appHandle = req->appHandle;

    if (req->keyType == CSR_BT_CM_KEY_TYPE_LE)
    {
        CsrBtTdDbLeKeys leKey;
        CsrBool leKeyPresent = (CsrBtTdDbGetLeKeys(req->addressType, &req->deviceAddr, &leKey) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS);

        if (leKeyPresent)
        {
            CsrUint16 emptyData = 0;

            resultCode = CsrBtTdDbWriteEntry(req->addressType,
                                             &req->deviceAddr,
                                             CSR_BT_TD_DB_SOURCE_SC,
                                             CSR_BT_CM_KEY_TYPE_LE,
                                             2,
                                             (void *)&emptyData);

            if (resultCode == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
            {
                CsrBtTdDbBredrKey bredrKey;
                CsrBool bredrKeyPresent = (CsrBtTdDbGetBredrKey(req->addressType, &req->deviceAddr, &bredrKey) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS);

                if (!bredrKeyPresent)
                {
                    CsrBtTypedAddr addrt;
                    addrt.addr = req->deviceAddr;
                    addrt.type = req->addressType;

                    /* Only LE Keys are present, remove device from bluestack.*/
                    dm_sm_remove_device_req(CSR_BT_CM_IFACEQUEUE, &addrt, NULL);
                }
                else
                {
                    /* BREDR keys are present, remove LE Keys from SM DB.*/
                    csrBtCmDbAddKeys(CmGetInstanceDataPtr(),
                                     req->addressType,
                                     &req->deviceAddr,
                                     &bredrKey,
                                     NULL,
                                     TRUE);
                }

                /* SM DB operation for updating the key is in progress, wait for it to finish.*/
                return;
            }
            else
            {
                /* TDDB operation has failed, inform this to caller.*/
                resultSupplier = CSR_BT_SUPPLIER_TD_DB;
            }
        }
        else
        {
            /* Either No keys are present or only BREDR key is present, mark the API success.*/
        }
    }
    else
    {
        /* Currently removal of BREDR key information is unsupported.*/
        resultCode = CSR_BT_RESULT_CODE_CM_UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE;
    }

    /* Either the API has failed or no SM DB operations are required, send confirmation and free the DM queue.*/
    CmDmRemoveDeviceKeyConfirmSend(cmData,
                                   &req->deviceAddr,
                                   req->addressType,
                                   resultCode,
                                   resultSupplier);
    CsrBtCmDmLocalQueueHandler();
}

void CmDmRemoveDeviceOptionsReqHandler(cmInstanceData_t *cmData)
{
    CmDmRemoveDeviceOptionsReq *req = (CmDmRemoveDeviceOptionsReq *) cmData->recvMsgP;
    CsrBtResultCode resultCode = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    CsrBtSupplier resultSupplier = CSR_BT_SUPPLIER_TD_DB;
    CsrBtTypedAddr addrt;
    CsrUint8 removeDeviceReq = 0;

    /* Storing the app handle and devixeAddr for sending confirmations.*/
    cmData->dmVar.appHandle = req->appHandle;
    cmData->dmVar.operatingBdAddr = req->deviceAddr;

    /* Initialise addrt with address and address type  If deviceAddr is set to all 0s, all devices should be removed so
     * set the address as Invalid.
     */
    if (CsrBtBdAddrEqZero(&req->deviceAddr))
    {
        addrt.type = TBDADDR_INVALID;
        SetBdAddrInvalid(&addrt.addr);
    }
    else
    {
        addrt.addr = req->deviceAddr;
        addrt.type = req->addressType;
    }

    /* Application wants to delete device from both PS and Bluestack SM DB */
    if (req->option == CM_REMOVE_DEVICE_OPTION_DEFAULT)
    {
        resultCode = cmDbRemoveDeviceExt(cmData, &req->deviceAddr, req->addressType, &removeDeviceReq);
    }
    else if (req->option == CM_REMOVE_DEVICE_OPTION_SMDB_ONLY)
    {
        /* Application wants to delete device only from Bluestack SM DB */
        dm_sm_remove_device_req(CSR_BT_CM_IFACEQUEUE, &addrt, NULL);

        /* Set the removeDeviceReq to 1. This is used to keep track of number of remove device requests sent to Bluestack */
        removeDeviceReq = 1;
    }

    /* Store the number of remove device requests sent. */
    cmData->scVar.deviceIndex = removeDeviceReq;

    /* If there were no devices to be deleted,.then deviceIndex counter will remain 0. 
     * This will act as a confirmation to the application that the API has completed.
     * In case when the devices are deleted, the confirmation received from the bluestack will be forwarded to application.
     */
    if (removeDeviceReq == 0)
    {
        /* Either the API has failed or no SM DB operations are required, send confirmation and free the DM queue.*/
        CmDmRemoveDeviceOptionsConfirmSend(cmData,
                                           &req->deviceAddr,
                                           req->addressType,
                                           resultCode,
                                           resultSupplier);
        CsrBtCmDmLocalQueueHandler();
    }
}

void CmPropgateAddressMappedIndEvent(cmInstanceData_t *cmData,
                                     const CsrBtDeviceAddr *randomAddr,
                                     const CsrBtTypedAddr *idAddrt)
{
    CsrBtCmLeAddressMappedInd ind;

    ind.type = CSR_BT_CM_LE_ADDRESS_MAPPED_IND;
    CsrBtBdAddrCopy(&ind.randomAddr, randomAddr);
    CsrBtAddrCopy(&ind.idAddr, idAddrt);

    CsrBtCmPropgateEvent(cmData,
                         csrBtCmEventDispatch,
                         CSR_BT_CM_EVENT_MASK_SUBSCRIBE_ADDR_MAPPED_IND,
                         HCI_SUCCESS,
                         &ind,
                         (void *) sizeof(ind));
}
#endif /* EXCLUDE_CSR_BT_SC_MODULE */

