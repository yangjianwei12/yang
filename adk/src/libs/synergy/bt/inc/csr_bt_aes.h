#ifndef CSR_BT_AES_H__
#define CSR_BT_AES_H__
/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #56 $
******************************************************************************/

/****************************************************************************
FILE
        csr_bt_aes.h  -  External header for the aes library


DESCRIPTION
    The Advanced Encryption Standard (AES) is implemented according to FIPs
    Pub # 197.  Where possible variables and #defs use the same nomenclature
    as in the standard.

    Limitations:
        Only 128 bit encryption is implemented.

*/

#include "csr_synergy.h"
#include "csr_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
NAME
        CsrBtAesEncrypt  - Encrypt a block of data using 128-bit AES.

FUNCTION
        Given 16 bytes of source data (data_in) and a 16-byte key (key),
        generates 16 bytes of encrypted data (data_out).
        data_in may be the same as data_out to encrypt in place.

        All inputs and outputs are little-endian.
*/
extern void CsrBtAesEncrypt (const CsrUint8 *data_in, const CsrUint8* key, CsrUint8 *data_out);

/****************************************************************************
NAME
        CsrBtAesCmac  - Calculate Cmac of a message.

FUNCTION
           AES-CMAC uses the Advanced Encryption Standard [NIST-AES] as a
           building block.  To generate a MAC, AES-CMAC takes a secret key, a
           message of variable length, and the length of the message in octets
           as inputs and returns a fixed-bit string called a MAC(16 bytes in this case).

        All inputs and outputs are little-endian.
*/

extern void CsrBtAesCmac(const CsrUint8* data_in, CsrUint16 data_length, CsrUint8* key, CsrUint8* data_out);

#ifdef __cplusplus
}
#endif
#endif /* CSR_BT_AES_H__ */

