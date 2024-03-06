/* Copyright (c) 2022 Qualcomm Technologies International, Ltd. */
/* %%version */

#include "csip_debug.h"
#include "csip_private.h"
#include "csip.h"

#ifdef CSR_TARGET_PRODUCT_VM
#include "crypto.h"
#else
#include "csr_bt_aes.h"
#endif
#include "csr_bt_cm_lib.h"
#include "csr_bt_addr.h"
#include "csr_bt_gatt_client_util_lib.h"

#define AD_TYPE_RSI_DATA   (0x2E)

#ifdef CSR_TARGET_PRODUCT_VM
extern bool CryptoAes128Cbc(bool encrypt, const uint8 * key,
       uint8 * nonce, uint16 flags, const uint8 * source_data,
       uint16 source_data_len_bytes, uint8 * dest_data, uint16 dest_data_len_bytes);
#endif

GattCsisClientDeviceData *CsipGetAttributeHandles(CsipProfileHandle profileHandle)
{
    Csip *csipInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);

    if (csipInst)
    {
        return GattCsisClientGetHandlesReq(csipInst->csisSrvcHdl);
    }
    return NULL;
}

bool CsipGetRsiFromAdvData(uint8 *data, uint8 dataLen, uint8 *rsi)
{
    uint8 ad_type;
    uint16 ad_data_size;
    uint16 i;

    /* Loop through the advert data, and extrat each ad-type to parse */
    while(data && dataLen)
    {
        /* first octet has the data size of the ad_type */
        ad_data_size = data[0];
        if(ad_data_size)
        {
            /* The next octet has the ad_type */
            ad_type = data[1];
            switch(ad_type)
            {
                case AD_TYPE_RSI_DATA:
                {
                    if(ad_data_size == CSIP_RSI_SIZE  + 1)
                    {
                        for(i=0; i< CSIP_RSI_SIZE; i++)
                        {
                            *(rsi + i ) = data[2+i];
                        }

                        return TRUE;
                    }
                }
                break;
                /* Ignore */
                default:
                break;
            }
        }

        /* Could be a superfluous data_length! */
        if((ad_data_size + 1) <=  dataLen)
        {
            /* Increase the pointer to the next ad_type. Please note we need to
                add 1 to consider the placeholder of data_size */
            data += (ad_data_size + 1);
            /* At the same time need to decrease the length as we don't expect to
                read more than required */
            dataLen -= (ad_data_size + 1);
        }
        else
        {
            /* Looks like a invalid packet, just igore it*/
            dataLen = 0;
        }
    }

    rsi = NULL;
    return FALSE;

}

static void aes128Encrypt(const uint8* key,
        uint8* initial_vector,
        const uint8 *data_in,
        uint16 input_size,
        uint8 *data_out,
        uint16 output_size)
{
#ifdef CSR_TARGET_PRODUCT_VM
    CryptoAes128Cbc(TRUE, key, initial_vector, 0, data_in, input_size, data_out, output_size);
#else
    CsrBtAesEncrypt(data_in, key, data_out);
    CSR_UNUSED(initial_vector);
    CSR_UNUSED(input_size);
    CSR_UNUSED(output_size);
#endif
}

static bool compareHash(uint8 *rsi, uint8 *localHash, uint8 size)
{
    bool result = TRUE;
    uint8 i;

    for(i = 0 ; i < size ; i++)
    {
        result &= ((localHash[i] == rsi[i]) ? TRUE : FALSE);
    }
    
    return  result;

}

bool CsipIsSetMember(uint8 *rsi,uint8 rsiSize, uint8 *sirk, uint8 sirkSize)
{
    bool resolved = FALSE;
    uint8 i;
    uint8 input[16] = {0};
    uint8 output[16] = {0};
    uint8 localhash[16] = {0};
    uint8 zeros_16[16] = {0};    /*! 16 bytes of zeros */
    uint8 sirkFlip[CSIP_SIRK_SIZE] = {0};

    if (rsi == NULL || sirk == NULL ||
        rsiSize != CSIP_RSI_SIZE || sirkSize != CSIP_SIRK_SIZE )
       return FALSE;

    /* prand */
    input[15] = rsi[3];
    input[14] = rsi[4];
    input[13] = rsi[5];

    for(i =0; i < CSIP_SIRK_SIZE; i++)
        sirkFlip[i] = sirk[CSIP_SIRK_SIZE - 1 -i];

    /* Generate localHash hash = sih(SIRK, prand) 
     * the algo implementation takes SIRK and RSI input as MSB->LSB,
     * so we are flipping the SIRK received from the user which
     * is LSB->MSB
     */
    aes128Encrypt(&sirkFlip[0], zeros_16, input, sizeof(input), output, sizeof(output));

    localhash[0] = output [15];
    localhash[1] = output [14];
    localhash[2] = output [13];

    /* The localHash value is then compared with the hash value extracted from
     * RSI. If the localHash value matches the extracted hash value,
     * then the RSI has been resolved.
     */
    if (compareHash(rsi, localhash, CSIP_RSI_SIZE/2))
    {
        resolved = TRUE;
    }
    else
    {
        CSIP_ERROR("CsipIsSetMember : RSI (hash) Not Matched\n");
    }

    return resolved;
}

void CsipDecryptSirk(CsipProfileHandle profileHandle, const uint8 *sirk)
{
    Csip *csipInst = ServiceHandleGetInstanceData((ServiceHandle) profileHandle);
    CsrBtTypedAddr typedAddr;

    if (csipInst &&
           CsrBtGattClientUtilFindAddrByConnId(csipInst->cid, &typedAddr))
    {
#ifdef CSR_TARGET_PRODUCT_VM
         uint8 i;
         uint8 sirkFlip[CSIP_SIRK_SIZE] = {0};
         CsrBtTpdAddrT tpAddr;
         tpAddr.tp_type = CSR_BT_TRANSPORT_LE;

         CsrBtAddrCopy(&(tpAddr.addrt), &typedAddr);

         for(i =0; i < CSIP_SIRK_SIZE; i++)
             sirkFlip[i] = sirk[CSIP_SIRK_SIZE - 1 -i];

         CsrBtCmLeSirkOperationReqSend(csipInst->lib_task,
                                       tpAddr,
                                       0,
                                       sirkFlip);
#else
       /* TBD */
       CSR_UNUSED(sirk);
#endif
    }
    else
    {
        CSIP_ERROR("Invalid profile_handle or addr not found \n");
    }
}

