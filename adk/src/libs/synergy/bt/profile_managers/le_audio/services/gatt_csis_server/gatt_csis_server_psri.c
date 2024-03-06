/******************************************************************************
 Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "gatt_csis_server_private.h"
#include "gatt_csis_server_debug.h"
#include "csr_random.h"

#ifdef BUILD_FOR_ADK
#include <crypto.h>
#else
#include "csr_bt_aes.h"
#endif

static uint32 UtilRandom32(void);

void Aes128Encrypt(const uint8* key, const uint8* initial_vector,const uint8 *data_in, size_t input_size, uint8 *data_out, size_t output_size);

/* Return a 32 bit random number */
static uint32 UtilRandom32(void)
{
	/* Get the pointer to seed for generating random number and generate random number */
    void *ptr = CsrRandomSeed();
    uint32 random = CsrRandom(ptr);

    CsrPmemFree(ptr);
    return random;
}


void Aes128Encrypt(const uint8* key,
        const uint8* initial_vector,
        const uint8 *data_in,
        size_t input_size,
        uint8 *data_out,
        size_t output_size)
{
#ifdef BUILD_FOR_ADK
    CryptoAes128Cbc(TRUE, key, initial_vector, 0, data_in, input_size, data_out, output_size);
#else
    CsrBtAesEncrypt(data_in, key, data_out);
    CSR_UNUSED(initial_vector);
    CSR_UNUSED(input_size);
    CSR_UNUSED(output_size);
#endif
}

uint8 *GattCsisServerGetRsi(CsisServerServiceHandleType handle)
{
    uint8 aes128[16];
    uint8 input[16] = {0};
    uint8 zeros_16[16] = {0};    /*! 16 bytes of zeros */
    uint32 rand32 = UtilRandom32(); /* Spec Test Value: 0x69f563*/
    uint8 *rsi = NULL;
    GCSISS_T *csis_server = (GCSISS_T *) ServiceHandleGetInstanceData(handle);

    if (csis_server == NULL)
        return rsi;

    input[15] =rand32 & 0xFF; /* lsb */
    input[14] =(rand32 >> 8) & 0xFF;
    /* This is as per CSIS Spec
     * The two most significant bits of prand shall be equal to 0 and 1
     */
    input[13] =((rand32 >>16) & 0x7F) | (0x40); /* msb */

    Aes128Encrypt(csis_server->data.sirk, zeros_16, input, sizeof(input), aes128, sizeof(aes128));
    rsi = (uint8 *) CsrPmemAlloc(GATT_CSIS_RSI_DATA_LENGTH);
    rsi[0] = aes128 [15];
    rsi[1] = aes128 [14];
    rsi[2] = aes128 [13];
    rsi[5] = input[13];
    rsi[4] = input[14];
    rsi[3] = input [15];

    return rsi;
}

