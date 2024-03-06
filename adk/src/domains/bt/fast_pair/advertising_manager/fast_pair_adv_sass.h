/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       fast_pair_adv_sass.h
\brief      Handles Advertising Data for SASS feature
*/

#ifndef FAST_PAIR_ADV_SASS_H_
#define FAST_PAIR_ADV_SASS_H_

#ifndef DISABLE_FP_SASS_SUPPORT

#include <task_list.h>
#include <connection_manager.h>

typedef void (*fastPair_SassUpdateAdvPayloadCompleteCallback)(bool success);

/*! @brief Private API to get the SASS advertising data size

    \returns current random resolvable data size
 */
uint8 fastPair_SASSGetAdvDataSize(void);

/*! @brief Private API to get the advertising data for SASS feature

    This will go into the fastpair advertisement data

    \param  void

    \returns pointer to random resolvable data
 */
uint8 *fastPair_SASSGetAdvData(void);

/*! @brief Private API to get pointer to in use account Key. 

    The account key filter will be calcualted after modifying the first byte of in-use account key
    
    \param  bool* is_in_use_account_key_handset_connected
    
    \returns  Pointer to in use account key 
 */

uint8* fastPair_SASSGetInUseAccountKey(bool* is_in_use_account_key_handset_connected);

/*! @brief API to update the in use account Key. 

    In use account key would be updated whenever the MRU device gets updated.
    
    \param  const bdaddr* bd_addr bd address of MRU device
    
    \returns  void
 */
void FastPair_SASSUpdateInUseAccountKeyForMruDevice(const bdaddr *bd_addr);

/*! @brief API to get in use account key active flag. 

    \param  void
 
    \returns In Use Account Key Active Flag 
 */
bool FastPair_SASSGetInUseAccountKeyActiveFlag(void);

/*! @brief API to check whether a device is the in use device. 

    \param  bd_addr to check

    \returns TRUE if the address belongs to the in use device, else FALSE
 */
bool FastPair_SassIsDeviceTheCurrentInUseDevice(const bdaddr * bd_addr);

/*! @brief API to notify SASS advertising of an in use device disconnect. 
 */
void FastPair_SassUpdateOnInUseDeviceDisconnect(void);

/*! @brief  API to trigger update of SASS adv payload. 

    \param  uint16 salt used as input for generating SASS random resolvable data
    \param  done_callback for the caller to receive notification that SASS advertising data is generated
 
    \returns None 
 */
void fastPair_SASSUpdateAdvPayload(uint16 salt, fastPair_SassUpdateAdvPayloadCompleteCallback done_callback);

/*! Private API to initialise Fast Pair SASS advertising module

    Called from Fast Pair advertising to initialise Fast Pair SASS advertising module
 */
void fastPair_SetUpSASSAdvertising(void);

#else
typedef void (*fastPair_SassUpdateAdvPayloadCompleteCallback)(bool success);
#define fastPair_SASSGetAdvDataSize() 0
#define fastPair_SASSGetAdvData() NULL
#define fastPair_SASSGetInUseAccountKey(x) (UNUSED(x),NULL)
#define FastPair_SASSUpdateInUseAccountKeyForMruDevice(x) UNUSED(x)
#define FastPair_SASSGetInUseAccountKeyActiveFlag() FALSE
#define FastPair_SassIsDeviceTheCurrentInUseDevice(x) (UNUSED(x),FALSE)
#define FastPair_SassUpdateOnInUseDeviceDisconnect()
#define fastPair_SASSUpdateAdvPayload(x, done_callback) (UNUSED(x),done_callback(TRUE))
#define fastPair_SetUpSASSAdvertising()
#endif

#endif /* FAST_PAIR_ADV_SASS_H_ */
