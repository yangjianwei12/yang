/******************************************************************************
 Copyright (c) 2019-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_td_db_ps.h"
#include "csr_bt_panic.h"
#include "csr_bt_util.h"
#include "csr_bt_td_db_sc.h"
#ifdef INCLUDE_BT_WEARABLE_TD_DB_PS
#include "csr_log_text_2.h"
#endif /* INCLUDE_BT_WEARABLE_TD_DB_PS */

#ifdef CSR_LOG_ENABLE
/* Log Text Handle */
CSR_LOG_TEXT_HANDLE_DEFINE(TdDbPsLto);
#ifdef CSR_TARGET_PRODUCT_VM
#include "csr_bt_td_db_enum_dbg.h"
CSR_PRESERVE_GENERATED_ENUM(TdDbResultCode)
CSR_PRESERVE_GENERATED_ENUM(CsrBtTdDbSource)
CSR_PRESERVE_GENERATED_ENUM(CsrBtTdDbFilter)
#endif /*CSR_TARGET_PRODUCT_VM*/
#endif /* CSR_LOG_ENABLE */

/* Bit Mask for feature CSR_BT_LE_SIGNING_ENABLE. */
#define TD_DB_LE_SIGN_BIT_MASK      (1 << 1)

/* Global cache which stores compressed version of TdDbIndex */
static TdDbCacheInfo tdDbCache[CSR_BT_TD_DB_LIST_MAX];

CsrUint16 maxTdlDevices = CSR_BT_TD_DB_LIST_MAX;

#define CSR_BT_MAX_TRUSTED_DEVICES         maxTdlDevices

const CsrUint8 tdDbMap[] =
{
    CSR_BT_TD_DB_KEYCOUNT_GAP,
    CSR_BT_TD_DB_KEYCOUNT_SC,
    CSR_BT_TD_DB_KEYCOUNT_GATT,
    CSR_BT_TD_DB_KEYCOUNT_DIRECT,
};

CsrSize tdDbMapSize = CSR_ARRAY_SIZE(tdDbMap);

#ifdef INCLUDE_BT_WEARABLE_TD_DB_PS
uint16 PsStoreWearable(CsrUint16 key, const void *buff, CsrUint16 words, CsrUint16 const lineNum)
{
    uint16 resWords;
    resWords = qapi_PS_Store((SYNERGY_PS_BASE_MASK | (uint32)key), (uint16 *) buff, words);
    if(resWords == 0)
    {
        CSR_LOG_TEXT_CRITICAL((0, 0, "csr_bt_td_db_ps.c Line: %d PS Store for PS Key 0x%x Failed", lineNum, (SYNERGY_PS_BASE_MASK | (uint32)key)));
    }
    else
    {
        CSR_LOG_TEXT_DEBUG((0, 0, "csr_bt_td_db_ps.c Line: %d PS Store for PS Key 0x%x success", lineNum, (SYNERGY_PS_BASE_MASK | (uint32)key)));
    }
    return resWords;
}

uint16 PsRetrieveWearable(CsrUint16 key, const void *buff, CsrUint16 words, CsrUint16 const lineNum)
{
    uint16 resWords;
    resWords = qapi_PS_Retrieve((SYNERGY_PS_BASE_MASK | (uint32)key), (uint16 *) buff, words);
    if(resWords == 0)
    {
        CSR_LOG_TEXT_DEBUG((0, 0, "csr_bt_td_db_ps.c Line: %d PS Retrieve for PS Key 0x%x Failed", lineNum, (SYNERGY_PS_BASE_MASK | (uint32)key)));
    }
    else
    {
        CSR_LOG_TEXT_DEBUG((0, 0, "csr_bt_td_db_ps.c Line: %d PS Retrieve for PS Key 0x%x success", lineNum, (SYNERGY_PS_BASE_MASK | (uint32)key)));
    }

    return resWords;
}
#endif

static CsrBool tdDbMatchesCachedAddress(const CsrBtDeviceAddr *deviceAddr, TdDbCacheInfo *cacheInfo)
{
    return (cacheInfo->lap == deviceAddr->lap &&
            cacheInfo->nap == deviceAddr->nap &&
            cacheInfo->uap == deviceAddr->uap);
}

static void packAddr(TdDbIndexElem *td, CsrBtAddressType addressType, const CsrBtDeviceAddr *addr)
{
#ifdef INSTALL_EXTENDED_TD_DB
    td->deviceAddress[0] = addr->nap;
    td->deviceAddress[1] = (uint16)addr->uap << 8 | (uint16)((addr->lap & 0xFF0000) >> 16);
    td->deviceAddress[2] = (uint16)addr->lap;
#else
    td->deviceAddress = *addr;
#endif
    td->addressType = addressType & 0x3;
}

static void unpackAddr(CsrBtTypedAddr *devAddr, const TdDbIndexElem *td)
{
#ifdef INSTALL_EXTENDED_TD_DB
    devAddr->addr.nap = (uint16)td->deviceAddress[0];
    devAddr->addr.uap = (uint8)(td->deviceAddress[1] >> 8);
    devAddr->addr.lap = (uint24)(td->deviceAddress[1] & 0xFF) << 16 | td->deviceAddress[2];
#else
    devAddr->addr = td->deviceAddress;
#endif
    devAddr->type = td->addressType;
}

static CsrUint8 getDevice(CsrBtAddressType addressType,
                          const CsrBtDeviceAddr *deviceAddr,
                          CsrBool *add)
{
    CsrUint8 deviceIndex;
    CsrUint8 emptyIndex = CSR_BT_MAX_TRUSTED_DEVICES;

    /* We are checking if the device exists or not, if yes provide the device index on which its present. */
    for (deviceIndex = 0; deviceIndex < CSR_BT_MAX_TRUSTED_DEVICES; deviceIndex++)
    {
        if (tdDbCache[deviceIndex].rank != TD_DB_DEVICE_RANK_INVALID)
        {
            if (tdDbCache[deviceIndex].addressType == addressType &&
                tdDbMatchesCachedAddress(deviceAddr, &tdDbCache[deviceIndex]))
            {
                /* Device entry is found in cache, report respective deviceIndex. */
                break;
            }
        }
        else
        {
            if (emptyIndex == CSR_BT_MAX_TRUSTED_DEVICES)
            {
                /* Store the empty index in case we need to add the device. */
                emptyIndex = deviceIndex;
            }
        }
    }

    /* If device is not found and empty slot available, add this device to actual trusted device list. */
    if (add && *add &&
        deviceIndex >= CSR_BT_MAX_TRUSTED_DEVICES &&
        emptyIndex < CSR_BT_MAX_TRUSTED_DEVICES)
    {
        TdDbIndex *tdDbIndex = CsrPmemZalloc(sizeof(*tdDbIndex));

        if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                        tdDbIndex,
                        CSR_BT_TD_DB_SIZE_IN_WORDS(*tdDbIndex)))
        {
            if (tdDbIndex->count + 1 <= CSR_BT_MAX_TRUSTED_DEVICES)
            {
                /* New device can be added */
                CsrUint8 source, key;

                tdDbIndex->device[emptyIndex].rank = tdDbIndex->count;
                packAddr(&tdDbIndex->device[emptyIndex], addressType, deviceAddr);
                tdDbIndex->count++;

                (void)TdDbPsUpdateIndexData(tdDbIndex, TRUE);

                /* Ensure that all keys for this device are initialized to 16-bit zero value */
                for (source = 0; source < CSR_ARRAY_SIZE(tdDbMap); source++)
                {
                    for (key = 0; key < tdDbMap[source]; key++)
                    {
                        CsrUint16 psKey;
                        CsrUint16 emptyData = 0;

                        (void) TdDbPsFindPsKey(emptyIndex,
                                               source,
                                               key,
                                               &psKey,
                                               CSR_ARRAY_SIZE(tdDbMap),
                                               tdDbMap,
                                               CSR_BT_TD_DB_LIST_MAX,
                                               SYNERGY_PS_BASE_TD_MAX);
                        PsStore(psKey,
                                &emptyData,
                                CSR_BT_TD_DB_SIZE_IN_WORDS(emptyData));
                    }
                }

                deviceIndex = emptyIndex;
            }
            else
            {
                *add = FALSE;
            }
        }

        CsrPmemFree(tdDbIndex);
    }
    else if (add && *add)
    {
        /* Notify caller that add was unsuccessfull. */
        *add = FALSE;
    }

    return deviceIndex;
}

#define findDevice(_addrType, _addr)    getDevice(_addrType, _addr, NULL)

/* This function is used to improve the ranks of all devices which are below currentDeviceRank.
 * Since this modifies tdDbIndex stored in PS, TdDbPsUpdateIndexData function shall be used by the caller
 * of this function in order to update both PS and cache.
 */
static void tdDbImproveDeviceRank(TdDbIndex *tdDbIndex, CsrUint8 currentDeviceRank)
{
    CsrUint8 i;

    /* Improve the rank of all device records below this device */
    for (i = 0; i < CSR_BT_MAX_TRUSTED_DEVICES; i++)
    {
        if (tdDbIndex->device[i].rank != TD_DB_DEVICE_RANK_INVALID &&
            tdDbIndex->device[i].rank > currentDeviceRank)
        {
            tdDbIndex->device[i].rank--;
        }
    }
}

/* Check if a device identified by device address and address type is present in the cache or not. */
static CsrUint8 tdDbFindDeviceInCache(const CsrBtDeviceAddr *deviceAddr,
                                      CsrBtAddressType addressType)
{
    CsrUint8 index;

    /* Need to go till CSR_BT_MAX_TRUSTED_DEVICES, as tdDbIndex is updated with the same limit. */
    for (index = 0; index < CSR_BT_MAX_TRUSTED_DEVICES; index++)
    {
        if (tdDbCache[index].rank != TD_DB_DEVICE_RANK_INVALID)
        {
            /* Valid slot */
            if (tdDbCache[index].addressType == addressType &&
                tdDbMatchesCachedAddress(deviceAddr, &tdDbCache[index]))
            {
                /* Device is found, return the index. */
                break;
            }
        }
    }
    return index;
}

CsrBool TdDbRestructureData(CsrUint32 version, CsrBtTdDbSystemInfo *systemInfo)
{
#ifdef INSTALL_EXTENDED_TD_DB
    return TdDbPsExtendedRestructurePsData(version, systemInfo);
#else
    CSR_UNUSED(version);
    CSR_UNUSED(systemInfo);
    return FALSE;
#endif
}

static CsrBool tdDbVersionMatches(TdDbSystemInfo *systemInfo)
{
    if (systemInfo->version != CSR_BT_TD_DB_FILE_VERSION)
    {
        CsrUint32 versionChange = systemInfo->version ^ CSR_BT_TD_DB_FILE_VERSION;
        if (versionChange == TD_DB_LE_SIGN_BIT_MASK)
        {
            /* The only change is in bit CSR_BT_TD_DB_VER_BIT_SIGN, hence it can be ignored.
             * Write the new version to PS and make sure the version matches.*/
            systemInfo->version = CSR_BT_TD_DB_FILE_VERSION;
            PsStore(CSR_BT_PS_KEY_SYSTEM,
                    systemInfo,
                    CSR_BT_TD_DB_SIZE_IN_WORDS(*systemInfo));
        }
    }

    return (systemInfo->version == CSR_BT_TD_DB_FILE_VERSION);
}


static CsrBool tdDbRestructureLeSignKey(CsrBtTypedAddr *tAddr,
                                        CsrUint8 deviceIndex,
                                        CsrBtTdDbSource source,
                                        CsrUint16 key,
                                        CsrUint16 length,
                                        void *value,
                                        CsrUint16 lengthRead)
{
    if (source == CSR_BT_TD_DB_SOURCE_SC &&
            key == CSR_BT_TD_DB_SC_KEY_LE_KEYS &&
            (length == sizeof(CsrBtTdDbLeKeys)) &&
            (lengthRead + sizeof(CsrBtScLeKeySign) == length) &&
            value)
     {
         /* From ADK 22.4 onwards, CsrBtTdDbLeKeys structure will always contain CsrBtScLeKeySign member,
          * irrespective of whether LE Sign feature is enabled or disabled.
          * To ensure PS backward compatibilty with release earlier to ADK22.4, perform PS Write for the
          * new CsrBtTdDbLeKeys structure if older CsrBtTdDbLeKeys structure is detected in PS.
          */
          CsrBtTdDbLeKeys *leKeys = (CsrBtTdDbLeKeys *)value;
          CsrMemSet(&leKeys->sign, 0x0, sizeof(CsrBtScLeKeySign));

          CSR_LOG_TEXT_INFO((TdDbPsLto, 0 , "tdDbRestructureLeSignKey"));

          /* If tAddr is already present then it will contain a valid bd address and the same should be taken care by the caller. */
          if (!tAddr && deviceIndex < CSR_BT_MAX_TRUSTED_DEVICES)
          {
              CsrBtTypedAddr addr;
              tAddr = &addr;
              if (CsrBtTdDbReadEntryByIndex(deviceIndex,
                                            source,
                                            key,
                                            NULL,
                                            NULL,
                                            tAddr,
                                            NULL) != CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
              {
                  return FALSE;
              }
          }

          if (tAddr &&
                  (CsrBtTdDbWriteEntry(tAddr->type,
                                       &tAddr->addr,
                                       source,
                                       key,
                                       length,
                                       value) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS))
          {
              CSR_LOG_TEXT_INFO((TdDbPsLto, 0 , "Address %04x:%02x:%06x", tAddr->addr.nap, tAddr->addr.uap, tAddr->addr.lap));
              return TRUE;
          }
    }
    return FALSE;
}

void CsrBtTdDbInit(uint16 numTdlDevices)
{
    TdDbSystemInfo systemInfo = { 0 };
    maxTdlDevices = (numTdlDevices < maxTdlDevices ? numTdlDevices : maxTdlDevices);

    CSR_LOG_TEXT_REGISTER(&TdDbPsLto, "BT_PS", 0, NULL);

    /* Invalidate version. */
    systemInfo.version = TD_DB_INVALID_VERSION;

    /* Reset trusted device cache. */
    CsrMemSet(&tdDbCache, 0x0, sizeof(tdDbCache));

    if (PsRetrieve(CSR_BT_PS_KEY_SYSTEM,
                   &systemInfo,
                   CSR_BT_TD_DB_SIZE_IN_WORDS(systemInfo)) &&
        tdDbVersionMatches(&systemInfo))
    {
        /* Existing database created by same version */
        TdDbIndex *tdDbIndex = CsrPmemZalloc(sizeof(*tdDbIndex));

        if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                        tdDbIndex,
                        CSR_BT_TD_DB_SIZE_IN_WORDS(*tdDbIndex)))
        {
            CsrUint8 i;

            for (i = 0; i < CSR_BT_TD_DB_LIST_MAX; i++)
            {
                if (tdDbIndex->device[i].rank >= tdDbIndex->count)
                {
                    /* Invalidate the rank of this device. */
                    tdDbIndex->device[i].rank = TD_DB_DEVICE_RANK_INVALID;
                }
                else if (tdDbIndex->device[i].rank == 0)
                {
                    CsrBtTypedAddr devAddr;
                    unpackAddr(&devAddr, &tdDbIndex->device[i]);
                    if(CsrBtBdAddrEqZero(&devAddr.addr))
                    {
                        tdDbIndex->device[i].rank = TD_DB_DEVICE_RANK_INVALID;
                    }
                }
            }

            (void)TdDbPsUpdateIndexData(tdDbIndex, TRUE);
        }

        CsrPmemFree(tdDbIndex);
    }
    else
    {
        /* Stored version is not matching the current version. This could happen in
         * cases when either its a fresh boot (no version) or the tddb layout is changed because of DFU. */
        if (!TdDbRestructureData(systemInfo.version, &systemInfo.systemInfo))
        {
            /* Restructuring is not done, continue with a clean slate. */
            CsrMemSet(&systemInfo, 0, sizeof(systemInfo));

            /* Write new version and blank system information. */
            systemInfo.version = CSR_BT_TD_DB_FILE_VERSION;
            PsStore(CSR_BT_PS_KEY_SYSTEM,
                    &systemInfo,
                    CSR_BT_TD_DB_SIZE_IN_WORDS(systemInfo));

            /* Remove existing records */
            CsrBtTdDbDeleteAll(CSR_BT_TD_DB_FILTER_EXCLUDE_NONE);
        }
    }

    CSR_LOG_TEXT_INFO((TdDbPsLto, 0 , "CsrBtTdDbInit devices %d version %08x", maxTdlDevices, systemInfo.version));
}

CsrUint8 CsrBtTdDbListDevices(CsrUint8 count, CsrBtTypedAddr *addr)
{
    CsrUint8 totalDevices = 0;

    CSR_LOG_TEXT_DEBUG((TdDbPsLto, 0 , "CsrBtTdDbListDevices count %d", count));

    if (count && addr)
    {
        /* Caller wants to fetch complete address into the address list pointer provided in the parameter. */
        TdDbIndex *tdDbIndex = CsrPmemZalloc(sizeof(*tdDbIndex));

        if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                        tdDbIndex,
                        CSR_BT_TD_DB_SIZE_IN_WORDS(*tdDbIndex)))
        {
            CsrUint8 index;

            count = (tdDbIndex->count < count ? tdDbIndex->count : count);
            for (index = 0; index < CSR_BT_MAX_TRUSTED_DEVICES; index++)
            {
                if (tdDbIndex->device[index].rank < count)
                {
                    unpackAddr(&addr[tdDbIndex->device[index].rank], &tdDbIndex->device[index]);
                }
            }
            totalDevices = tdDbIndex->count;
        }
        else
        {
            CSR_LOG_TEXT_ERROR((TdDbPsLto, 0 , "CsrBtTdDbListDevices Ps Retrieve Failed!"));
        }
        CsrPmemFree(tdDbIndex);
    }
    else
    {
        /* Caller wants to find number of devices in the trusted device list. */
        CsrUint8 index;

        for (index = 0; index < CSR_BT_MAX_TRUSTED_DEVICES; index++)
        {
            if (tdDbCache[index].rank != TD_DB_DEVICE_RANK_INVALID)
            {
                totalDevices++;
            }
        }
    }

    /* Total number of valid devices in the trusted device list. */
    return totalDevices;
}

CsrBtResultCode CsrBtTdDbWriteEntry(CsrBtAddressType addressType,
                                    const CsrBtDeviceAddr *deviceAddr,
                                    CsrBtTdDbSource source,
                                    CsrUint16 key,
                                    CsrUint16 length,
                                    const void *value)
{
    CsrBtResultCode result;
    CsrBool deviceAdded = TRUE;

#ifdef INSTALL_EXTENDED_TD_DB
    if(source == CSR_BT_TD_DB_SOURCE_GAP)
    {
        result = CSR_BT_RESULT_CODE_TD_DB_INVALID_SUPPLIER;
        CSR_LOG_TEXT_ERROR((TdDbPsLto, 0 , "CsrBtTdDbWriteEntry enum:TdDbResultCode:%d", result));
        return result;
    }
#endif

    CsrUint8 deviceIndex = getDevice(addressType, deviceAddr, &deviceAdded);

    if (deviceIndex < CSR_BT_MAX_TRUSTED_DEVICES)
    {
        CsrUint16 psKey;

        result = TdDbPsFindPsKey(deviceIndex,
                                 source,
                                 key,
                                 &psKey,
                                 CSR_ARRAY_SIZE(tdDbMap),
                                 tdDbMap,
                                 CSR_BT_TD_DB_LIST_MAX,
                                 SYNERGY_PS_BASE_TD_MAX);

        if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            CsrUint16 words = PsStore(psKey,
                                      value,
                                      (CsrUint16)CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(length));

            if (CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(length) == words)
            {
                CSR_LOG_TEXT_DEBUG((TdDbPsLto, 0 , "CsrBtTdDbWriteEntry enum:TdDbResultCode:%d addr %04x:%02x:%06x enum:CsrBtTdDbSource:%d key %d length %d", result, deviceAddr->nap, deviceAddr->uap, deviceAddr->lap, source, key, length));
                return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
            }

            result = CSR_BT_RESULT_CODE_TD_DB_WRITE_FAILED;
        }

        if (deviceAdded)
        {
            CsrBtTdDbDeleteDevice(addressType, deviceAddr);
        }
    }
    else
    {
        result = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    }

    CSR_LOG_TEXT_ERROR((TdDbPsLto, 0 , "CsrBtTdDbWriteEntry enum:TdDbResultCode:%d addr %04x:%02x:%06x enum:CsrBtTdDbSource:%d key %d length %d deviceAdded %d", result, deviceAddr->nap, deviceAddr->uap, deviceAddr->lap, source, key, length, deviceAdded));

    return result;
}

CsrBtResultCode CsrBtTdDbReadEntry(CsrBtAddressType addressType,
                                   const CsrBtDeviceAddr *deviceAddr,
                                   CsrBtTdDbSource source,
                                   CsrUint16 key,
                                   CsrUint16 *length,
                                   void *value)
{
    CsrBtResultCode result;
    CsrUint8 deviceIndex = findDevice(addressType, deviceAddr);

    if (deviceIndex < CSR_BT_MAX_TRUSTED_DEVICES)
    {
        CsrUint16 psKey;

        if (source == 0 && key == 0 && length == NULL && value == NULL)
        {
            /*Call from CsrBtTdDbDeviceExists() function*/
            return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
        }

        result = TdDbPsFindPsKey(deviceIndex,
                                 source,
                                 key,
                                 &psKey,
                                 CSR_ARRAY_SIZE(tdDbMap),
                                 tdDbMap,
                                 CSR_BT_TD_DB_LIST_MAX,
                                 SYNERGY_PS_BASE_TD_MAX);

        if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
        {
            void *buffer = NULL;
            CsrUint16 words = 0;

            if (length && *length && value)
            {
                buffer = value;
                words = CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(*length);
            }
            else if (value)
            {
                result = CSR_BT_RESULT_CODE_TD_DB_INVALID_PARAMS;
            }

            if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
            {
                words = PsRetrieve(psKey, buffer, words);
                CSR_LOG_TEXT_DEBUG((TdDbPsLto, 0 , "CsrBtTdDbReadEntry enum:TdDbResultCode:%d addr %04x:%02x:%06x enum:CsrBtTdDbSource:%d key %d words %d", result, deviceAddr->nap, deviceAddr->uap, deviceAddr->lap, source, key, words));

                if (length)
                {
                    *length = CSR_BT_TD_DB_WORD_TO_BYTE_SIZE(words);
                }
                return result;
            }
        }
    }
    else
    {
        result = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    }

    CSR_LOG_TEXT_INFO((TdDbPsLto, 0 , "CsrBtTdDbReadEntry enum:TdDbResultCode:%d addr %04x:%02x:%06x enum:CsrBtTdDbSource:%d key %d", result, deviceAddr->nap, deviceAddr->uap, deviceAddr->lap, source, key));

    return result;
}

CsrBtResultCode CsrBtTdDbGetEntry(CsrBtAddressType addressType,
                                  const CsrBtDeviceAddr *deviceAddr,
                                  CsrBtTdDbSource source,
                                  CsrUint16 key,
                                  CsrUint16 length,
                                  void *value)
{
    CsrBtResultCode result;
    CsrUint16 lengthRead = length;
    result = CsrBtTdDbReadEntry(addressType,
                                deviceAddr,
                                source,
                                key,
                                &lengthRead,
                                value);

    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        if (CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(lengthRead) != CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(length))
        {
            result = CSR_BT_RESULT_CODE_TD_DB_READ_FAILED;
            if (deviceAddr)
            {
                CsrBtTypedAddr tAddr;
                tAddr.addr = (*deviceAddr);
                tAddr.type = addressType;
                if (tdDbRestructureLeSignKey(&tAddr, TD_DB_DEVICE_RANK_INVALID, source, key, length, value, lengthRead))
                {
                    lengthRead = length;
                    result = CsrBtTdDbReadEntry(addressType,
                                                deviceAddr,
                                                source,
                                                key,
                                                &lengthRead,
                                                value);
                }
            }
        }
    }
    
    return result;
}

CsrBtResultCode CsrBtTdDbReadEntryByIndex(CsrUint8 deviceIndex,
                                          CsrBtTdDbSource source,
                                          CsrUint16 key,
                                          CsrUint16 *length,
                                          void *value,
                                          CsrBtTypedAddr *addr,
                                          CsrBool *isPriority)
{
    CsrBtResultCode result = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;

    if (deviceIndex < CSR_BT_MAX_TRUSTED_DEVICES)
    {
        CsrUint16 psKey;
        CsrUint8 index;

        for (index = 0; index < CSR_BT_MAX_TRUSTED_DEVICES; index++)
        {
            if (tdDbCache[index].rank == deviceIndex)
            {
                break;
            }
        }

        if (index < CSR_BT_MAX_TRUSTED_DEVICES)
        {
            /* The device is present in the trusted device list. */
            if (isPriority)
            {
                *isPriority = tdDbCache[index].priorityDevice;
            }

            result = TdDbPsFindPsKey(index,
                                     source,
                                     key,
                                     &psKey,
                                     CSR_ARRAY_SIZE(tdDbMap),
                                     tdDbMap,
                                     CSR_BT_TD_DB_LIST_MAX,
                                     SYNERGY_PS_BASE_TD_MAX);

            if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
            {
                CsrUint16 words = 0;
                void *val = NULL;

                if (addr)
                {
                    /* Value of address is required, need to fetch this information from PS. */
                    TdDbIndex *tdDbIndex = CsrPmemZalloc(sizeof(*tdDbIndex));
                    if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                                    tdDbIndex,
                                    CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(sizeof(*tdDbIndex))))
                    {
                        /* Correct index is already known from the cache. */
                        unpackAddr(addr, &tdDbIndex->device[index]);
                        CSR_LOG_TEXT_DEBUG((TdDbPsLto, 0 , "CsrBtTdDbReadEntryByIndex addr %04x:%02x:%06x", addr->addr.nap, addr->addr.uap, addr->addr.lap));
                    }
                    CsrPmemFree(tdDbIndex);
                }

                if (length && *length && value)
                {
                    words = CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(*length);
                    val = value;
                }
                else if (value)
                {
                    result = CSR_BT_RESULT_CODE_TD_DB_INVALID_PARAMS;
                }

                if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
                {
                    words = PsRetrieve(psKey, val, words);
                    CSR_LOG_TEXT_DEBUG((TdDbPsLto, 0 , "CsrBtTdDbReadEntryByIndex words %d", words));

                    if (length)
                    {
                        *length = CSR_BT_TD_DB_WORD_TO_BYTE_SIZE(words);
                    }
                }
            }
        }
    }

    if (result == CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE || result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        CSR_LOG_TEXT_DEBUG((TdDbPsLto, 0 , "CsrBtTdDbReadEntryByIndex enum:TdDbResultCode:%d deviceindex %d enum:CsrBtTdDbSource:%d key %d", result, deviceIndex, source, key));
    }
    else
    {
        CSR_LOG_TEXT_ERROR((TdDbPsLto, 0 , "CsrBtTdDbReadEntryByIndex enum:TdDbResultCode:%d deviceindex %d enum:CsrBtTdDbSource:%d key %d", result, deviceIndex, source, key));
    }
    return result;
}

CsrBtResultCode CsrBtTdDbGetEntryByIndex(CsrUint8 deviceIndex,
                                         CsrBtTdDbSource source,
                                         CsrUint16 key,
                                         CsrUint16 length,
                                         void *value,
                                         CsrBtTypedAddr *addr)
{
    CsrUint16 lengthRead = length;
    CsrBtResultCode result = CsrBtTdDbReadEntryByIndex(deviceIndex,
                                                       source,
                                                       key,
                                                       &lengthRead,
                                                       value,
                                                       addr,
                                                       NULL);

    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        if (CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(lengthRead) != CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(length))
        {
            result = CSR_BT_RESULT_CODE_TD_DB_READ_FAILED;
            if (tdDbRestructureLeSignKey(addr, deviceIndex, source, key, length, value, lengthRead))
            {
                lengthRead = length;
                result = CsrBtTdDbReadEntryByIndex(deviceIndex,
                                                   source,
                                                   key,
                                                   &lengthRead,
                                                   value,
                                                   addr,
                                                   NULL);
            }
        }
    }
    else
    {
        result = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    }

    return result;
}

CsrBtResultCode CsrBtTdDbDeleteDevice(CsrBtAddressType addressType,
                                      const CsrBtDeviceAddr *deviceAddr)
{
    CsrBtResultCode result = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    CsrUint8 deviceIndex = tdDbFindDeviceInCache(deviceAddr, addressType);

    if (deviceIndex < CSR_BT_MAX_TRUSTED_DEVICES)
    {
        result = CSR_BT_RESULT_CODE_TD_DB_DELETE_FAILED;

        if (!tdDbCache[deviceIndex].priorityDevice)
        {
            /* Its not a priority device, hence can be deleted. */
            TdDbIndex *tdDbIndex = CsrPmemZalloc(sizeof(*tdDbIndex));;

            if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                            tdDbIndex,
                            CSR_BT_TD_DB_SIZE_IN_WORDS(*tdDbIndex)))
            {
                tdDbIndex->count--;

                /* Imrove the rank of all the devices which are below this device. */
                CSR_LOG_TEXT_INFO((TdDbPsLto, 0 , "CsrBtTdDbDeleteDevice addr %04x:%02x:%06x rank %d new_count %d", deviceAddr->nap, deviceAddr->uap, deviceAddr->lap, tdDbIndex->device[deviceIndex].rank, tdDbIndex->count));
                tdDbImproveDeviceRank(tdDbIndex, tdDbIndex->device[deviceIndex].rank);
                tdDbIndex->device[deviceIndex].rank = TD_DB_DEVICE_RANK_INVALID;

                (void)TdDbPsUpdateIndexData(tdDbIndex, TRUE);
                result = CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
            }
            CsrPmemFree(tdDbIndex);
        }
    }

    CSR_LOG_TEXT_INFO((TdDbPsLto, 0 , "CsrBtTdDbDeleteDevice enum:TdDbResultCode:%d", result));

    return result;
}

void CsrBtTdDbDeleteAll(CsrBtTdDbFilter filter)
{
    CsrUint8 i;
    TdDbIndex *tdDbIndex = CsrPmemZalloc(sizeof(*tdDbIndex));
    CsrBtResultCode result = CSR_BT_RESULT_CODE_TD_DB_SUCCESS;

    if (filter != CSR_BT_TD_DB_FILTER_EXCLUDE_NONE)
    {
        /* As some of the devices may not get removed, we need to get
         * the device details in order to apply filter. */
        if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                        tdDbIndex,
                        CSR_BT_TD_DB_SIZE_IN_WORDS(*tdDbIndex)))
        {
            for (i = 0; i < CSR_BT_TD_DB_LIST_MAX; i++)
            {
                if (tdDbIndex->device[i].rank != TD_DB_DEVICE_RANK_INVALID &&
                    ((filter & CSR_BT_TD_DB_FILTER_EXCLUDE_PRIORITY) && !tdDbIndex->device[i].priorityDevice))
                {
                    /* Improve the rank of all the devices which are below this device.
                     * This is done in order to maintain the list sorted. */
                    tdDbImproveDeviceRank(tdDbIndex, tdDbIndex->device[i].rank);

                    /* Invalidate the rank of this device to mark it deleted. */
                    tdDbIndex->device[i].rank = TD_DB_DEVICE_RANK_INVALID;
                    tdDbIndex->count--;
                }
            }
        }
        else
        {
            /* Snice the read has failed, no need to update the PS. */
            result = CSR_BT_RESULT_CODE_TD_DB_READ_FAILED;
        }
    }
    else
    {
        /* For this case the tdDbIndex.count will remain zero, as we have just created
         * a blank tdDbIndex and storing it to the PS. */
         for (i = 0; i < CSR_BT_TD_DB_LIST_MAX; i++)
         {
            /* Invalidate the rank. */
            tdDbIndex->device[i].rank = TD_DB_DEVICE_RANK_INVALID;
         }
    }

    if (result == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        (void)TdDbPsUpdateIndexData(tdDbIndex, TRUE);
    }

    CsrPmemFree(tdDbIndex);

    CSR_LOG_TEXT_INFO((TdDbPsLto, 0 , "CsrBtTdDbDeleteAll enum:CsrBtTdDbFilter:%d enum:TdDbResultCode:%d", filter, result));
}

CsrBtResultCode CsrBtTdDbPrioritiseDevice(CsrBtAddressType addressType,
                                          const CsrBtDeviceAddr *deviceAddr,
                                          CsrBtTdDbUpdateFlag options)
{
    CsrBtResultCode result = CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE;
    CsrUint8 deviceIndex;

    CSR_LOG_TEXT_DEBUG((TdDbPsLto, 0 , "CsrBtTdDbPrioritiseDevice addr %04x:%02x:%06x options %d", deviceAddr->nap, deviceAddr->uap, deviceAddr->lap, options));

    if ((options & CSR_BT_TD_DB_UPDATE_FLAG_PRIORITISE) && (options & CSR_BT_TD_DB_UPDATE_FLAG_DEPRIORITISE))
    {
        /* both flags cannot be set */
        return CSR_BT_RESULT_CODE_TD_DB_INVALID_PARAMS;
    }

    /* Check if the device is present in the cache or not. */
    deviceIndex = tdDbFindDeviceInCache(deviceAddr, addressType);

    if (deviceIndex < CSR_BT_MAX_TRUSTED_DEVICES)
    { /* Device found */
        TdDbIndex *tdDbIndex;

        if ((options & CSR_BT_TD_DB_UPDATE_FLAG_MRU) &&
            ((options & CSR_BT_TD_DB_UPDATE_FLAG_PRIORITISE) != CSR_BT_TD_DB_UPDATE_FLAG_PRIORITISE) &&
            ((options & CSR_BT_TD_DB_UPDATE_FLAG_DEPRIORITISE) != CSR_BT_TD_DB_UPDATE_FLAG_DEPRIORITISE) &&
            tdDbCache[deviceIndex].rank == 0)
        {
            /* If caller is doing MRU only then check if the device is already MRU,
             * in which case we will return from here. */
            return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
        }

        /* Device is present and needs to be updated, fetch the complete tdDbIndex data from PS. */
        tdDbIndex = CsrPmemZalloc(sizeof(*tdDbIndex));
        if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                        tdDbIndex,
                        CSR_BT_TD_DB_SIZE_IN_WORDS(*tdDbIndex)))
        {
            CsrUint8 i, rank;

            if (options & (CSR_BT_TD_DB_UPDATE_FLAG_PRIORITISE | CSR_BT_TD_DB_UPDATE_FLAG_DEPRIORITISE))
            {
                tdDbIndex->device[deviceIndex].priorityDevice =
                    (options & CSR_BT_TD_DB_UPDATE_FLAG_PRIORITISE) ? TRUE : FALSE;
            }

            /* Mark the device most recently used */
            rank = tdDbIndex->device[deviceIndex].rank;

            /* Reduce the rank of all device records above this device */
            for (i = 0; i < CSR_BT_MAX_TRUSTED_DEVICES; i++)
            {
                if (tdDbIndex->device[i].rank != TD_DB_DEVICE_RANK_INVALID &&
                    tdDbIndex->device[i].rank < rank)
                {
                    tdDbIndex->device[i].rank++;
                }
            }

            /* Set this device as top ranked */
            tdDbIndex->device[deviceIndex].rank = 0;

            /* Deprioritized device will be placed just above the MRU non-priority device.
             * "rank" can be used to determine the MRU non-priority device. When non-priority
             * devices are not present, deprioritized device will be placed just below the LRU
             * priority device, so "priRank" can be used to determine the LRU priority device.
             */
            if (options & CSR_BT_TD_DB_UPDATE_FLAG_DEPRIORITISE)
            {
                /* This is used to find out LRU priority device for the cases where there are
                 * no non-priority devices is present in the list.*/
                CsrUint8 priRank=0;

                /* Here rank is used to determine the MRU non-priority device.*/
                rank = TD_DB_DEVICE_RANK_INVALID;
                for (i = 0; i < CSR_BT_MAX_TRUSTED_DEVICES; i++)
                {
                    if (i != deviceIndex &&
                        tdDbIndex->device[i].rank != TD_DB_DEVICE_RANK_INVALID)
                    {
                        if (!tdDbIndex->device[i].priorityDevice &&
                            tdDbIndex->device[i].rank < rank)
                        {
                            rank = tdDbIndex->device[i].rank;
                        }
                        else if (tdDbIndex->device[i].priorityDevice &&
                                 tdDbIndex->device[i].rank > priRank)
                        {
                            /* Capture the lowest rank of priority device, in case there are no non-priority devices.*/
                            priRank = tdDbIndex->device[i].rank;
                        }
                    }
                }

                /* If a non-priority device is present,then set the rank of the device to one rank ahead of MRU
                 * non-priority device (rank -1) otherwise use the rank of LRU priority device (priRank).*/
                tdDbIndex->device[deviceIndex].rank = (rank != TD_DB_DEVICE_RANK_INVALID ? rank-1: priRank);

                /* Adjust the rank of priority devices coming till the rank. */
                for (i = 0; i < CSR_BT_MAX_TRUSTED_DEVICES; i++)
                {
                    if (i != deviceIndex &&
                        tdDbIndex->device[i].rank != TD_DB_DEVICE_RANK_INVALID &&
                        tdDbIndex->device[i].rank <= tdDbIndex->device[deviceIndex].rank)
                    {
                        if (tdDbIndex->device[i].priorityDevice)
                        {
                            tdDbIndex->device[i].rank--;
                        }
                    }
                }
            }

            (void)TdDbPsUpdateIndexData(tdDbIndex, TRUE);
            result = CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
        }

        CsrPmemFree(tdDbIndex);
    }

    return result;
}

CsrBtResultCode CsrBtTdDbSetSystemInfo(const CsrBtTdDbSystemInfo *systemInfo)
{
    TdDbSystemInfo info;

    info.version = CSR_BT_TD_DB_FILE_VERSION;
    info.systemInfo = *systemInfo;

    if (PsStore(CSR_BT_PS_KEY_SYSTEM,
                &info,
                CSR_BT_TD_DB_SIZE_IN_WORDS(info)) == CSR_BT_TD_DB_SIZE_IN_WORDS(info))
    {
        return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
    }
    else
    {
        CSR_LOG_TEXT_ERROR((TdDbPsLto, 0 , "CsrBtTdDbSetSystemInfo TD DB write failed!"));
        return CSR_BT_RESULT_CODE_TD_DB_WRITE_FAILED;
    }
}

CsrBtResultCode CsrBtTdDbGetSystemInfo(CsrBtTdDbSystemInfo *systemInfo)
{
    TdDbSystemInfo info;

    if (PsRetrieve(CSR_BT_PS_KEY_SYSTEM,
                   &info,
                   CSR_BT_TD_DB_SIZE_IN_WORDS(info)) == CSR_BT_TD_DB_SIZE_IN_WORDS(info))
    {
        *systemInfo = info.systemInfo;
        return CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
    }
    else
    {
        CSR_LOG_TEXT_ERROR((TdDbPsLto, 0 , "CsrBtTdDbGetSystemInfo TD DB read failed!"));
        return CSR_BT_RESULT_CODE_TD_DB_READ_FAILED;
    }
}

CsrBtResultCode TdDbPsFindPsKey(CsrUint8 deviceIndex,
                                CsrBtTdDbSource source,
                                CsrUint16 key,
                                CsrUint16 *psKey,
                                CsrUint8 mapSize,
                                const CsrUint8 *map,
                                CsrUint8 listMax,
                                CsrUint16 keyMax)
{
    CsrBtResultCode result;

    if (source < mapSize)
    {
        if (key < map[source])
        {
            CsrUint8 i;
            CsrUint16 tmpPsKey = CSR_BT_TD_DB_BASE;

            for (i = 0; i < source; i++)
            {
                tmpPsKey += map[i] * listMax;
            }
            tmpPsKey += (key * listMax);
            tmpPsKey +=  deviceIndex;

            if (tmpPsKey <= keyMax)
            {
                *psKey = tmpPsKey;
                result = CSR_BT_RESULT_CODE_TD_DB_SUCCESS;
            }
            else
            {
                result = CSR_BT_RESULT_CODE_TD_DB_INVALID_PARAMS;
            }
        }
        else
        {
            result = CSR_BT_RESULT_CODE_TD_DB_INVALID_SUPPLIER;
        }
    }
    else
    {
        result = CSR_BT_RESULT_CODE_TD_DB_INVALID_KEY;
    }

    return result;
}

/* Update the cache from the Trusted device information and store the same in PS if instructed by caller. */
CsrBool TdDbPsUpdateIndexData(TdDbIndex *tdDbIndex, CsrBool updatePs)
{
    CsrBtTypedAddr tAddr;
    CsrUint8 index;

    /* Need to go till CSR_BT_MAX_TRUSTED_DEVICES, as tdDbIndex is updated with the same limit. */
    for (index = 0; index < CSR_BT_TD_DB_LIST_MAX; index++)
    {
        unpackAddr(&tAddr, &tdDbIndex->device[index]);
        tdDbCache[index].lap = tAddr.addr.lap;
        tdDbCache[index].uap = tAddr.addr.uap;
        tdDbCache[index].nap = tAddr.addr.nap;
        tdDbCache[index].rank = tdDbIndex->device[index].rank;
        tdDbCache[index].addressType = tAddr.type & 0x1; /* 0x1 mask is for klocwork. */
        tdDbCache[index].priorityDevice = tdDbIndex->device[index].priorityDevice;
    }

    if (updatePs)
    {
        if (!PsStore(CSR_BT_PS_KEY_INDEX,
                      tdDbIndex,
                      CSR_BT_TD_DB_SIZE_IN_WORDS(*tdDbIndex)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

