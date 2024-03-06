#ifndef CSR_BT_AES_PRIVATE_H__
#define CSR_BT_AES_PRIVATE_H__
/******************************************************************************
 Copyright (c) 2020 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #1 $
******************************************************************************/

#include "csr_bt_aes.h"
#include "csr_synergy.h"


#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE 16
#define Nk (BLOCK_SIZE/4)
#define MAX_BLOCK 256
#define Nb 4   /* number uint32 comprising state */

/* for 128 bit key Nb(Nr+1) = 176 bytes 44 words */
#define EXPANDED_KEY_SIZE 176    

/* AES uses a fixed size of 4 bytes per int */
#define INT_SIZE_AES 4     

#ifdef __cplusplus
}
#endif
#endif  /* CSR_BT_AES_PRIVATE_H__ */
