/******************************************************************************
 Copyright (c) 2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #58 $
******************************************************************************/

#include "csr_synergy.h"
#include "csr_bt_td_db_ps.h"
#include "csr_bt_td_db_sc.h"
#include "csr_bt_td_db_gatt.h"
#include "csr_bt_util.h"
#include "csr_bt_cm_private_prim.h"

#ifdef INSTALL_EXTENDED_TD_DB
/* This is the value of CSR_BT_TD_DB_LIST_MAX in old image. */
#define CSR_BT_TD_DB_LIST_MAX_LEGACY        8
/* This is the value of CSR_BT_TD_DB_KEYCOUNT_GAP in old image. */
#define CSR_BT_TD_DB_KEYCOUNT_GAP_LEGACY    2
/* This is the value of (CSR_BT_TD_DB_BASE_DIRECT + (CSR_BT_TD_DB_KEYCOUNT_DIRECT * CSR_BT_TD_DB_LIST_MAX) - 1) in old image. */
#define SYNERGY_PS_BASE_TD_MAX_LEGACY       149

/* Legacy structures */
typedef struct
{
    CsrBtDeviceAddr         deviceAddress;
    CsrBtAddressType        addressType;
    CsrUint8                rank;
    CsrBool                 priorityDevice;
} TdDbIndexElemLegacy;

typedef struct
{
    CsrUint16               count;
    TdDbIndexElemLegacy     device[CSR_BT_TD_DB_LIST_MAX_LEGACY];
} TdDbIndexLegacy;

static const CsrUint8 tdDbMapLegacy[] =
{
    CSR_BT_TD_DB_KEYCOUNT_GAP_LEGACY,
    CSR_BT_TD_DB_KEYCOUNT_SC,
    CSR_BT_TD_DB_KEYCOUNT_GATT,
    CSR_BT_TD_DB_KEYCOUNT_DIRECT,
};

static void tdDbPsExtendedWriteSystemInfo(CsrBtTdDbSystemInfo *systemInfo)
{
    TdDbSystemInfo tdDbSysInfo;

    /* Store the database version */
    tdDbSysInfo.version = CSR_BT_TD_DB_FILE_VERSION;
    tdDbSysInfo.systemInfo = *systemInfo;

    PsStore(CSR_BT_PS_KEY_SYSTEM,
            &tdDbSysInfo,
            CSR_BT_TD_DB_SIZE_IN_WORDS(tdDbSysInfo));
}

static CsrBool tdDbPsExtendedRestructureIndex(TdDbIndexLegacy *tdDbIndexLegacy, TdDbIndex *tdDbIndex)
{
    CsrUintFast8 index;

    /* Initialize new TdDbIndex */
    for (index = 0; index < CSR_BT_TD_DB_LIST_MAX; index++)
    {
        tdDbIndex->device[index].rank = TD_DB_DEVICE_RANK_INVALID;
    }

    /* Populate New TdDbIndex with the legacy data. */
    for (index = 0; index < CSR_BT_TD_DB_LIST_MAX_LEGACY; index++)
    {
        if (tdDbIndexLegacy->device[index].rank != TD_DB_DEVICE_RANK_INVALID &&
            tdDbIndexLegacy->device[index].rank < tdDbIndexLegacy->count &&
            (tdDbIndexLegacy->device[index].rank != 0 || !CsrBtBdAddrEqZero(&tdDbIndexLegacy->device[index].deviceAddress)))
        {
            tdDbIndex->device[index].deviceAddress[0] = tdDbIndexLegacy->device[index].deviceAddress.nap;
            tdDbIndex->device[index].deviceAddress[1] = (uint16)tdDbIndexLegacy->device[index].deviceAddress.uap << 8 | 
                                                        (uint16)((tdDbIndexLegacy->device[index].deviceAddress.lap & 0xFF0000) >> 16);
            tdDbIndex->device[index].deviceAddress[2] = (uint16)tdDbIndexLegacy->device[index].deviceAddress.lap;

            tdDbIndex->device[index].addressType = tdDbIndexLegacy->device[index].addressType & 0x3;
            tdDbIndex->device[index].priorityDevice = tdDbIndexLegacy->device[index].priorityDevice & 0x01;
            tdDbIndex->device[index].rank = tdDbIndexLegacy->device[index].rank & 0x0F;

            tdDbIndex->count++;
        }
    }

    return TdDbPsUpdateIndexData(tdDbIndex, TRUE);
}

static CsrUint16 tdDbPsExtendedGetKeyDataSize(CsrUint8 deviceIndex, CsrBtTdDbSource source, CsrUint16 key)
{
    CsrUint16 psKey, words = 0;

    if (TdDbPsFindPsKey(deviceIndex,
                         source,
                         key,
                         &psKey,
                         CSR_ARRAY_SIZE(tdDbMapLegacy),
                         tdDbMapLegacy,
                         CSR_BT_TD_DB_LIST_MAX_LEGACY,
                         SYNERGY_PS_BASE_TD_MAX_LEGACY) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        words = PsRetrieve(psKey, NULL, 0);
    }

    return CSR_BT_TD_DB_WORD_TO_BYTE_SIZE(words);
}

static CsrBool tdDbPsExtendedReadDataFromKey(CsrUint8 deviceIndex,
                                             CsrBtTdDbSource source,
                                             CsrUint16 key,
                                             void *buffer,
                                             CsrUint16 bufferSize)
{
    CsrUint16 psKey, words;

    /* Data is present on the key. */
    if (TdDbPsFindPsKey(deviceIndex,
                         source,
                         key,
                         &psKey,
                         CSR_ARRAY_SIZE(tdDbMapLegacy),
                         tdDbMapLegacy,
                         CSR_BT_TD_DB_LIST_MAX_LEGACY,
                         SYNERGY_PS_BASE_TD_MAX_LEGACY) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        words = PsRetrieve(psKey,
                           buffer,
                           (CsrUint16)CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(bufferSize));

        if (CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(bufferSize) == words)
        {
            return TRUE;
        }
    }

    return FALSE;
}

static void tdDbPsExtendedWriteDataToKey(CsrUint8 deviceIndex,
                                         CsrBtTdDbSource source,
                                         CsrUint16 key,
                                         void *buffer,
                                         CsrUint16 bufferSize)
{
    CsrUint16 psKey=0;

    if (TdDbPsFindPsKey(deviceIndex,
                        source,
                        key,
                        &psKey,
                        tdDbMapSize,
                        tdDbMap,
                        CSR_BT_TD_DB_LIST_MAX,
                        SYNERGY_PS_BASE_TD_MAX) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)
    {
        PsStore(psKey,
                buffer,
                (CsrUint16)CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(bufferSize));
    }
}

static void tdDbPsExtendedCopyKeyData(CsrUint8 deviceIndex,
                                      CsrBtTdDbSource source,
                                      CsrUint16 key,
                                      void *buffer,
                                      CsrUint16 bufferSize)
{
    /* Read the legacy data from the PS */
    if (tdDbPsExtendedReadDataFromKey(deviceIndex, source, key, buffer, bufferSize))
    {
        tdDbPsExtendedWriteDataToKey(deviceIndex, source, key, buffer, bufferSize);
    }
}

static void tdDbPsExtendedRestructureSource(TdDbIndex *tdDbIndex)
{
    CsrUintFast8 source, key;
    CsrUint8 *buffer = NULL;
    CsrUint16 bufferSize = 0;

    /* Check for each <SOURCE, KEY> we need to update the PS, if yes copy
     * the data from old PS structure to the new PS structure. Since Extended TDDB has removed 
     * source CSR_BT_TD_DB_SOURCE_GAP, we are starting from CSR_BT_TD_DB_SOURCE_SC.
     */
    for (source = CSR_BT_TD_DB_SOURCE_SC; source < CSR_BT_TD_DB_SOURCE_MAX; source++)
    {
        for (key = 0; key < tdDbMap[source]; key++)
        {
            CsrUintFast8 deviceIndex;

            for (deviceIndex = 0; deviceIndex < CSR_BT_TD_DB_LIST_MAX; deviceIndex++)
            {
                if (tdDbIndex->device[deviceIndex].rank != TD_DB_DEVICE_RANK_INVALID)
                {
                    CsrUint8 keySize = tdDbPsExtendedGetKeyDataSize(deviceIndex, source, key);

                    if (keySize > 0)
                    {
                        /* Some data is present on the key, check if buffer needs to be re-allocated or not. */
                        if (buffer)
                        {
                            /* buffer is already allocated, check for the size, and allocate the buffer only when required.*/
                            if (bufferSize < keySize)
                            {
                                /* Re-allocate as a bigger buffer is required for the key. */
                                bufferSize = keySize;
                                CsrPmemFree(buffer);
                                buffer = CsrPmemAlloc(bufferSize);
                            }
                        }
                        else
                        {
                            /* buffer is not allocated, allocate it with the key size. */
                            bufferSize = keySize;
                            buffer = CsrPmemAlloc(bufferSize);
                        }

                        /* Clear buffer for a fresh read/write. */
                        CsrMemSet(buffer, 0x00, bufferSize);
                        tdDbPsExtendedCopyKeyData(deviceIndex,
                                                  source,
                                                  key,
                                                  buffer,
                                                  keySize);
                    }
                }
            }
        }
    }

    if (buffer)
    {
        CsrPmemFree(buffer);
    }
}

CsrBool TdDbPsExtendedRestructurePsData(CsrUint32 version, CsrBtTdDbSystemInfo *systemInfo)
{
    CsrBool restructComplete = FALSE;

    if (version != TD_DB_INVALID_VERSION &&
        version != CSR_BT_TD_DB_FILE_VERSION)
    {
        /* This means there is a change in version, check if do need to restructure
         * in this case and call the appropriate restructuring function. Restructuring is
         * required in case of extended TD DB as it changes the data strutures and PS key layout.*/
        if (!(version & TD_DB_VER_BIT_EXTENDED_TD_DB) &&
            (CSR_BT_TD_DB_FILE_VERSION & TD_DB_VER_BIT_EXTENDED_TD_DB))
        {
            /* There is a version change and the new version has extended TDDB supported, this requires restructuring. */
            TdDbIndexLegacy tdDbIndexLegacy = { 0 };
            TdDbIndex tdDbIndex = { 0 };

            /* Write the new version and existing ER, IR keys to the PS. */
            tdDbPsExtendedWriteSystemInfo(systemInfo);

            /* Restructuring TdDbIndex with the legacy information. */
            if (PsRetrieve(CSR_BT_PS_KEY_INDEX,
                            &tdDbIndexLegacy,
                            CSR_BT_TD_DB_SIZE_IN_WORDS(tdDbIndexLegacy)))
            {
                if (tdDbPsExtendedRestructureIndex(&tdDbIndexLegacy, &tdDbIndex))
                {
                    tdDbPsExtendedRestructureSource(&tdDbIndex);
                    restructComplete = TRUE;
                }
            }
        }
    }

    return restructComplete;
}
#endif /* INSTALL_EXTENDED_TD_DB */

