/* Copyright (c) 2020 Qualcomm Technologies International, Ltd. */
/*  */

#include <stdio.h>
#include <stdlib.h>

#include "gatt_csis_server_private.h"

#include <crypto.h>
#include <util.h>

static uint32 UtilRandom32(void);


/* Return a 32 bit random number */
static uint32 UtilRandom32(void)
{
    return ((uint32)UtilRandom() << 16) | (uint32)UtilRandom();
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

    if (CryptoAes128Cbc(TRUE, csis_server->data.sirk, zeros_16, 0, input, sizeof(input), aes128, sizeof(aes128)))
    {
        rsi = (uint8 *) PanicUnlessMalloc(GATT_CSIS_RSI_DATA_LENGTH);
        rsi[0] = aes128 [15];
        rsi[1] = aes128 [14];
        rsi[2] = aes128 [13];
        rsi[5] = input[13];
        rsi[4] = input[14];
        rsi[3] = input [15];
    }
    else
    {
       GATT_CSIS_SERVER_DEBUG_INFO(("CSIS: RSI data generation failed \n"));
    }

    return rsi;
}




