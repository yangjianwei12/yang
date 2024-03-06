/****************************************************************************
Copyright (c) 2014 - 2023 Qualcomm Technologies International, Ltd.


FILE NAME
    upgrade_psstore.h
    
DESCRIPTION
    The upgrade library needs to be able to keep track of upgrades and the
    state of the device.

    This header file contains ALL functions that access the persistent storage.
    Other header files may manage specific functionality that records state
    in the persistent storage.


    There are two sets of APIs.

    Generic access to upgrade library PSKEY storage
    This provides access to the UPGRADE_LIB_PSKEY structure defined in
    upgrade_private.h. This includes functions to commit the storage.


    Access to the file system table (FSTAB).
    NOTE The original API for this was written for a Zarkov demo which 
    supported VM app upgrade only. This needs to be more complex to deal 
    with upgrading additional file partitions.
*/
#ifndef UPGRADE_PSSTORE_H_
#define UPGRADE_PSSTORE_H_

#include <csrtypes.h>
#include <message.h>
#include "upgrade_private.h"
#include "upgrade_ctx.h"

/*! Number of PSKEY writes we should be able to make to permit a critical
 * operation 
 */
#define UPGRADE_PS_WRITES_FOR_CRITICAL_OPERATIONS   2

/*!
    @brief UpgradeSavePSKeys

    Writes the local copy of PSKEY information to the PS Store. 
    Note that this API will not panic, even if no PS storage has been
    made available.
*/
void UpgradeSavePSKeys(void);

/*!
    @brief UpgradeLoadPSStore

    Initialisation function to load the pskey information upon 
    init of the upgrade library.

    @param dataPskey Number of the user pskey to use for ugprade data.
    @param dataPskeyStart Offset in dataPskey from which the upgrade library can use.

*/
void UpgradeLoadPSStore(uint16 dataPskey,uint16 dataPskeyStart);

/*!
    @brief UpgradePSSpaceForCriticalOperations

    Checks whether there appears to be sufficient free space in the PSSTORE
    to allow upgrade PSKEY operations to complete.

    The upgrade process needs to remember when a partition has been closed
    and without this an attempt to resume an upgrade will fail.

    @note The upgrade library does not know how many more operations are 
    required to complete an upgrade so it is possible that false will be 
    returned even though this is the last partition and we could safely
    write and reboot.

    @returns false if it is considered risky to continue
    
*/
bool UpgradePSSpaceForCriticalOperations(void);

/*!
    @brief UpgradePsRunningNewApplication
    
    Query if we are running an upgraded application, but it hasn't been
    committed yet.

    Note: It is only valid to call this function during the early
    initialisation of the application, before UpgradeInit has been called.

    @param dataPskey Number of the user pskey to use for ugprade data.
    @param dataPskeyStart Offset in dataPskey from which the upgrade library can use.
    
    @return TRUE if partway through an upgrade and running the upgraded
            application; FALSE otherwise.
*/
bool UpgradePsRunningNewApplication(uint16 dataPskey, uint16 dataPskeyStart);

/*!
    @brief UpgradePsGetResumePoint
    
    Get the resume point directly from the PSStore. This function is useful before
    the PSStore gets loaded in the RAM

    @param dataPskey Number of the user pskey to use for ugprade data.
    @param dataPskeyStart Offset in dataPskey from which the upgrade library can use.
    
    @return Value of the resume point.
*/
uint16 UpgradePsGetResumePoint(uint16 dataPskey, uint16 dataPskeyStart);

/*!
    @brief Clear all the PSKEYs which are used to store the DFU headers
    @param none
    
    @return none
*/
void UpgradeClearHeaderPSKeys(void);

/*!
    @brief Store the DFU headers in PSKEYs.
    @param data Pointer to data to be strored.
    @param len Length data to be strored.
    @return Upgrade library error code.
*/
UpgradeHostErrorCode UpgradeSaveHeaderInPSKeys(const uint8 *data, uint16 data_size);

/*!
    @brief Compare a string with data at the beginning of header PSkey.
    @param Identifier for DFU file header ID to search
*/
bool UpgradeSearchIDInHeaderPSKeys(char *identifier);

/*!
    @brief It will save the offset for hash table in stored in PsKey.
    @param length Based on length offset and PsKey is calculated.
*/
void UpgradeSaveHashTableOffset(uint16 length);

/*!
    @brief Returns last stored bytes in PsKey using hashTable PsKey and offset 
    @param buffer data to be read is stored into buffer
    @param reqSize Number of bytes to read
*/
bool UpgradeRequestLastPsKeyBytes(uint8 *buffer, uint16 reqSize);

/*!
    @brief Read the complete upgrade header from PsKey ID
    @param psKeyID PsKey from which the data is to be read
    @psKeyOffset Offset from which the data is to be read
    @param len2Read number of bytes to read from PsKey
    @buff Returns read data into this buffer 
*/
bool UpgradeReadPsKeyData(uint16 psKeyID, uint16 psKeyOffset, uint16 len2Read, uint8 *buff);

/*!
    @brief UpgradeUpdateDfuHashTablePsKey
    Update the dfuHashTablePsKey and dfuHashTableOffset by number of bytes 
    @param updateLen number of bytes by which pskey need to update
*/
void UpgradeIncreaseDfuHashTableCursor(uint16 updateLen);

#endif /* UPGRADE_PSSTORE_H_ */
