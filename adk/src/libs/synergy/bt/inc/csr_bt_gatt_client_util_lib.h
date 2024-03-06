#ifndef CSR_BT_GATT_CLIENT_UTIL_LIB_H__
#define CSR_BT_GATT_CLIENT_UTIL_LIB_H__
/******************************************************************************
 Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_bt_gatt_client_util_prim.h"


#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_BTCONNID 0x0000

typedef struct
{
    uint16 size;
    bool   error;
    uint8* dataStart;
    uint8* data;
} GattBuffIterator;

typedef struct
{
    const CsrBtGattDbAccessWriteInd* accessInd;
    uint8  bufferIndex;
    uint16 offset;
    uint8 error;
} GattAccessWriteIndIterator;

#define GATT_BUFF_ITERATOR_ERROR_DETECTED(iter)       ((iter)->error)
#define GATT_BUFF_ITERATOR_GET_CURRENT_DATA_PTR(iter) ((iter)->data)
#define GATT_BUFF_ITERATOR_SET_ERROR(iter)           ((iter)->error = TRUE)

/* --------------------------------------------------------------------
   Name:
   GattAccessIndIteratorInitialise

   Description:
   This API is used by the profiles in order to intitialise an access 
   iterator

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    CsrBtGattDbAccessWriteInd     accessInd  - access write indication
   -------------------------------------------------------------------- */
void   GattAccessIndIteratorInitialise(GattAccessWriteIndIterator* iter,
                                       const CsrBtGattDbAccessWriteInd* accessInd);

/* --------------------------------------------------------------------
   Name:
   GattAccessIndIteratorRead8

   Description:
   This API is used by the profiles in order to get the next octet
   from CsrBtGattDbAccessWriteInd. 
   
   Note : 
   The data buffer in CsrBtGattDbAccessWriteInd can be fragmented. This function
   will always perform a boundary check and iterate through the fragmented 
   buffers in order to find the next available octet/s. 

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
   -------------------------------------------------------------------- */
uint8  GattAccessIndIteratorRead8(GattAccessWriteIndIterator* iter);

/* --------------------------------------------------------------------
   Name:
   GattAccessIndIteratorRead16

   Description:
   This API is used by the profiles in order to get the next 2 octets
   from CsrBtGattDbAccessWriteInd. 

   Note : 
   The data buffer in CsrBtGattDbAccessWriteInd can be fragmented. This function
   will always perform a boundary check and iterate through the fragmented 
   buffers in order to find the next available octet/s. 

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
   -------------------------------------------------------------------- */
uint16 GattAccessIndIteratorRead16(GattAccessWriteIndIterator* iter);

/* --------------------------------------------------------------------
   Name:
   GattAccessIndIteratorRead24

   Description:
   This API is used by the profiles in order to get the next 3 octets
   from CsrBtGattDbAccessWriteInd. Ind buffer can be fragmented in  multiple
   buffers.

   Note : 
   The data buffer in CsrBtGattDbAccessWriteInd can be fragmented. This function
   will always perform a boundary check and iterate through the fragmented 
   buffers in order to find the next available octet/s. 

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
   -------------------------------------------------------------------- */
uint32 GattAccessIndIteratorRead24(GattAccessWriteIndIterator* iter);

/* --------------------------------------------------------------------
   Name:
   GattAccessIndIteratorRead32

   Description:
   This API is used by the profiles in order to get the next 4 octets
   from CsrBtGattDbAccessWriteInd. Ind buffer can be fragmented in  multiple
   buffers.

   Note : 
   The data buffer in CsrBtGattDbAccessWriteInd can be fragmented. This function
   will always perform a boundary check and iterate through the fragmented 
   buffers in order to find the next available octet/s. 

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
   -------------------------------------------------------------------- */
uint32 GattAccessIndIteratorRead32(GattAccessWriteIndIterator* iter);

/* --------------------------------------------------------------------
   Name:
   GattAccessIndIteratorReadMultipleOctets

   Description:
   This API is used by the profiles in order to get the next <N> octets
   from CsrBtGattDbAccessWriteInd. Ind buffer can be fragmented in  multiple
   buffers.

   Note : 
   The data buffer in CsrBtGattDbAccessWriteInd can be fragmented. This function
   will always perform a boundary check and iterate through the fragmented 
   buffers in order to find the next available octet/s. 

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    uint8                         numOctets - number of octets.
   -------------------------------------------------------------------- */
uint8* GattAccessIndIteratorReadMultipleOctets(GattAccessWriteIndIterator* iter,
                                                uint8 numOctets);

/* --------------------------------------------------------------------
   Name:
   GattBuffIteratorInitialise

   Description:
   This API is used by the profiles in order to intitialise a buffer Iterator.
   Buffer iterators are used to write data in a buffer

  Parameters:
    GattBuffIterator    iter      - iterator.
    uint8*              buffer    - pointer to the buffer
    uint16              size      - buffer size
   -------------------------------------------------------------------- */
void GattBuffIteratorInitialise(GattBuffIterator* iter, uint8* buffer, uint16 size);

/* --------------------------------------------------------------------
   Name:
   GattBuffIteratorSkipOctets

   Description:
   This API is used by the profiles in order to skip <N> amount of octets
   in a buffer. The contents of the skipped octets will remain unchanged.

   Note : 
   This function will always perform a boundary check to ensure we are not
   overflowing the destination buffer

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    uint8                         numOctets - Number of octets to be skipped
   -------------------------------------------------------------------- */
void GattBuffIteratorSkipOctets(GattBuffIterator* iter, uint8 numOctets);

/* --------------------------------------------------------------------
   Name:
   GattBuffIteratorWrite8

   Description:
   This API is used by the profiles in order to write 1 octet in a buffer.

   Note : 
   This function will always perform a boundary check to ensure we are not
   overflowing the destination buffer

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    uint8                         value     - Value to be written in the buffer
   -------------------------------------------------------------------- */
bool GattBuffIteratorWrite8(GattBuffIterator* iter, uint8 value);

/* --------------------------------------------------------------------
   Name:
   GattBuffIteratorWrite16

   Description:
   This API is used by the profiles in order to write 2 octets in a buffer.

   Note : 
   This function will always perform a boundary check to ensure we are not
   overflowing the destination buffer

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    uint8                         value     - Value to be written in the buffer
   -------------------------------------------------------------------- */
bool GattBuffIteratorWrite16(GattBuffIterator* iter, uint16 value);

/* --------------------------------------------------------------------
   Name:
   GattBuffIteratorWrite24

   Description:
   This API is used by the profiles in order to write 3 octets in a buffer.

   Note : 
   This function will always perform a boundary check to ensure we are not
   overflowing the destination buffer

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    uint8                         value     - Value to be written in the buffer
   -------------------------------------------------------------------- */
bool GattBuffIteratorWrite24(GattBuffIterator* iter, uint32 value);

/* --------------------------------------------------------------------
   Name:
   GattBuffIteratorWrite32

   Description:
   This API is used by the profiles in order to write 4 octets in a buffer.

   Note : 
   This function will always perform a boundary check to ensure we are not
   overflowing the destination buffer

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    uint8                         value     - Value to be written in the buffer
   -------------------------------------------------------------------- */
bool GattBuffIteratorWrite32(GattBuffIterator* iter, uint32 value);

/* --------------------------------------------------------------------
   Name:
   GattBuffIteratorWriteMultipleOctets

   Description:
   This API is used by the profiles in order to write <N> octets in a buffer.

   Note : 
   This function will always perform a boundary check to ensure we are not
   overflowing the destination buffer

  Parameters:
    GattAccessWriteIndIterator    iter      - iterator.
    uint8*                         value    - pointer to an source buffer
    uint8                         numOctets - Number of octets to be copied
   -------------------------------------------------------------------- */
bool GattBuffIteratorWriteMultipleOctets(GattBuffIterator* iter, uint8* value,
                                          uint8 numOctets);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattClientUtilFindAddrByConnId

   Description:
   This API is used by an application/module for finding the Bluetooth
   device address corresponding to a particular btConnId.

   There is no separate confirmation for this, the address ,if found,
   will be copied to the parameter (addr) passed by the application
   and TRUE will be returned.
   In case of any failure, FALSE will be returned.

   Note : The requesting app is the owner of the memory "addr" and not
          gatt_client_util so it should be freed by the app only.

  Parameters:
    CsrBtConnId             _btConnId      - Connection identifier.
    CsrBtTypedAddr         *_addr          - An allocated pointer to the
                                             address.
   -------------------------------------------------------------------- */
CsrBool CsrBtGattClientUtilFindAddrByConnId(CsrBtConnId btConnId, CsrBtTypedAddr *addr);

/* --------------------------------------------------------------------
   Name:
   CsrBtGattClientUtilFindConnIdByAddr

   Description:
   This API is used by an application/module for finding the Connection ID (btConnId)
   corresponding to a particular typed BT device address of a connected device.

   There is no separate confirmation for this, the conn id ,if found,
   will be returned to the corresponding module.
   In case of any failure, INVALID_BTCONNID will be returned.

  Parameters:
    CsrBtTypedAddr         *_addr          - Pointer to the typed device
                                             address.
  Returns :
    CsrBtConnId             _btConnId      - Connection identifier.
   -------------------------------------------------------------------- */
CsrBtConnId CsrBtGattClientUtilFindConnIdByAddr(CsrBtTypedAddr *addr);

/* --------------------------------------------------------------------
Name:
CsrBtGattClientUtilFIndEncryptionStatus

Description:
This API is used by an application/module for finding the encryption status
corresponding to a particular typed BT device address of a connected device.

Parameters:
CsrBtTypedAddr         *_addr          - Pointer to the typed device
address.
Returns :
CsrBool             _encryption_status      - Encryption status
-------------------------------------------------------------------- */
#ifdef CSR_BT_GATT_CLIENT_UTIL_TRACK_ENCRYPTION
CsrBool CsrBtGattClientUtilFIndEncryptionStatus(CsrBtTypedAddr *addr);
#endif
void *GetGattManagerMsgFromGattMsg(void *message, CsrBtGattPrim *id);
void *GetServiceClientByGattMsg(CsrCmnList_t *list, void *msg);
void *GetServiceClientByServiceHandle(void *msg);

#ifdef __cplusplus
}
#endif

#endif /* CSR_BT_GATT_CLIENT_UTIL_LIB_H__ */
