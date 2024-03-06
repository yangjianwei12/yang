/****************************************************************************
Copyright (c) 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_psstore_priv.h
    
DESCRIPTION
    Definition of FSTAB datatype.
*/

#ifndef UPGRADE_PSSTORE_PRIV_H_
#define UPGRADE_PSSTORE_PRIV_H_

typedef struct {
    uint16 length;
    uint16 ram_copy[PSKEY_MAX_STORAGE_LENGTH];
} FSTAB_COPY;

/* DFU headers will be stored on PSKEYs.
   DFU_HEADER_PSKEY_START: First PSKEY number where DFU headers will be stored.
   DFU_HEADER_NUM_PSKEY: Number of PSKEYs which will be used to store the headers.
   Upgrade Header Size: 12 + 36 = 48 bytes
   Partition Header (plus first word) Size: N * 20; where max N is 13 (num partition), so 13 * 20 = 260 bytes
   Footer Size: 12 + 256 = 268
   Total 576 bytes. So using 10 PSKEYs (10 * 64 = 640 bytes)
   
   ***********TODO:New CDA2 Header Size****************** 
   Generic first part and the header part =                     48 bytes
   Table of hashes for 32 partition data - 32*48 bytes =        1536 bytes
   Signature size =                                             96 bytes
   Any extra metadata for the table of hashes and signature*=   16 bytes    * for future use case
   ------------------------------------------------------------------------------
   Total local memory requirement =                             1696 bytes (close to 1700 bytes) 
   
   Total memory requirement for upgrade header =                1696 bytes (close to 1700 bytes)
   Partition header memory requirement for 32 partitions =      32 * (16+48)bytes = 2048 bytes
   ------------------------------------------------------------------------------
   Total PSKEY memory requirement =                             3748 bytes (close to 3750 bytes) 
   
   Additional 50 PSKeys required other than the already available 10 PSKeys making a total of 60 PSKeys
*/
#define DFU_HEADER_PSKEY_START  510 /* Reserved PSKEY for upgrade DFU_HEADER_PSKEY_START */
#define DFU_HEADER_PSKEY_LEN    10
#define DFU_HEADER_PSKEY_END    (DFU_HEADER_PSKEY_START + DFU_HEADER_PSKEY_LEN - 1)

#endif /* UPGRADE_PSSTORE_PRIV_H_ */
