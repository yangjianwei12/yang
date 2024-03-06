/******************************************************************************
 Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
 ******************************************************************************/
#ifndef __CSR_BT_TD_DB_H_
#define __CSR_BT_TD_DB_H_

#include "csr_synergy.h"
#include "csr_types.h"
#include "csr_macro.h"
#include "csr_bt_util.h"
#include "csr_bt_addr.h"
#include "csr_bt_result.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Number of octets which can be stored per key, this is platform limitation
 * Todo: this needs to be moved to proper location.
 */
#define CSR_BT_TD_DB_BYTES_PER_KEY      128
#define CSR_BT_TD_DB_WORD_SIZE          2

#define CSR_BT_TD_DB_WORD_TO_BYTE_SIZE(_words)      (_words * CSR_BT_TD_DB_WORD_SIZE)
#define CSR_BT_TD_DB_BYTE_TO_WORD_SIZE(_bytes)      (((_bytes) + CSR_BT_TD_DB_WORD_SIZE - 1) / CSR_BT_TD_DB_WORD_SIZE)

#define CSR_BT_TD_DB_KEY_STRUCT_COMPILE_ASSERT(_struct)                         \
    CSR_COMPILE_TIME_ASSERT(sizeof(_struct) <= CSR_BT_TD_DB_BYTES_PER_KEY,      \
                            CSR_SYNERGY_TD_DB_KEY_SIZE_VIOLATION_ ## _struct);  \
    CSR_COMPILE_TIME_ASSERT((sizeof(_struct) % CSR_BT_TD_DB_WORD_SIZE) == 0,    \
                            CSR_SYNERGY_TD_DB_KEY_ALIGNMENT_VIOLATION_ ## _struct)

/* Sources */
typedef CsrUint8 CsrBtTdDbSource;
#define CSR_BT_TD_DB_SOURCE_GAP           ((CsrBtTdDbSource) 0)
/* CSR_BT_TD_DB_SOURCE_SC is reserved for CM/SC module and shall not be used by any other module or application. */
#define CSR_BT_TD_DB_SOURCE_SC            ((CsrBtTdDbSource) 1)
/* CSR_BT_TD_DB_SOURCE_GATT is reserved for GATT module and shall not be used by any other module or application. */
#define CSR_BT_TD_DB_SOURCE_GATT          ((CsrBtTdDbSource) 2)
#define CSR_BT_TD_DB_SOURCE_DIRECT        ((CsrBtTdDbSource) 3)

#define CSR_BT_TD_DB_SOURCE_MAX             4

/* Result codes */
typedef CsrBtResultCode TdDbResultCode;
#define CSR_BT_RESULT_CODE_TD_DB_SUCCESS                ((TdDbResultCode) 0)
#define CSR_BT_RESULT_CODE_TD_DB_NO_DEVICE              ((TdDbResultCode) 1)
#define CSR_BT_RESULT_CODE_TD_DB_INVALID_SUPPLIER       ((TdDbResultCode) 2)
#define CSR_BT_RESULT_CODE_TD_DB_INVALID_KEY            ((TdDbResultCode) 3)
#define CSR_BT_RESULT_CODE_TD_DB_INVALID_PARAMS         ((TdDbResultCode) 4)
#define CSR_BT_RESULT_CODE_TD_DB_SLOTS_EXHAUSTED        ((TdDbResultCode) 5)
#define CSR_BT_RESULT_CODE_TD_DB_WRITE_FAILED           ((TdDbResultCode) 6)
#define CSR_BT_RESULT_CODE_TD_DB_READ_FAILED            ((TdDbResultCode) 7)
#define CSR_BT_RESULT_CODE_TD_DB_DELETE_FAILED          ((TdDbResultCode) 8)
#define CSR_BT_RESULT_CODE_TD_DB_TASK_FAILED            ((TdDbResultCode) 9)
#define CSR_BT_RESULT_CODE_TD_DB_UPDATE_FAILED          ((TdDbResultCode) 10)

/* Keys for direct source */
#define CSR_BT_TD_DB_DIRECT_KEY_ATTR                    (0)

/*This macro defines the size of the hash buffer in csr_bt_gatt_ps.c*/
#define CSR_BT_DB_HASH_SIZE (16)

typedef CsrUint8 CsrBtTdDbUpdateFlag;
#define CSR_BT_TD_DB_UPDATE_FLAG_MRU               ((CsrBtTdDbUpdateFlag) (1 << 0))
#define CSR_BT_TD_DB_UPDATE_FLAG_PRIORITISE        ((CsrBtTdDbUpdateFlag) (1 << 1))
#define CSR_BT_TD_DB_UPDATE_FLAG_DEPRIORITISE      ((CsrBtTdDbUpdateFlag) (1 << 2))

typedef CsrUint8 CsrBtTdDbFilter;
#define CSR_BT_TD_DB_FILTER_EXCLUDE_NONE           ((CsrBtTdDbFilter) 0)
#define CSR_BT_TD_DB_FILTER_EXCLUDE_PRIORITY       ((CsrBtTdDbFilter) (1 << 0))

typedef struct
{
    CsrUint16               er[8];
    CsrUint16               ir[8];
    CsrUint32               signCounter;
    CsrUint16               div;
    CsrUint8                hash[CSR_BT_DB_HASH_SIZE]; /*This buffer is added
                                                         here as it is required
                                                         to be stored, independent
                                                         of a device.*/
} CsrBtTdDbSystemInfo;

#if defined(CSR_TARGET_PRODUCT_VM) || defined(INCLUDE_BT_WEARABLE_TD_DB_PS)
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbInit
 *
 *  DESCRIPTION
 *      This function is called to initialize(or create) the database in the
 *      required state.
 *
 *  PARAMETERS
 *      This function takes 1 argument.
 *      numTdlDevices:  Number of devices in Trusted List
 *----------------------------------------------------------------------------*/
void CsrBtTdDbInit(uint16 numTdlDevices);
#else
/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbInit
 *
 *  DESCRIPTION
 *      This function is called to initialize(or create) the database in the
 *      required state.
 *
 *  PARAMETERS
 *      No parameters are there in this function.
 *----------------------------------------------------------------------------*/
void CsrBtTdDbInit(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbDeInit
 *
 *  DESCRIPTION
 *      This function is called to initialize(or create) the database in the
 *      required state.
 *
 *  PARAMETERS
 *      No parameters are there in this function.
 *----------------------------------------------------------------------------*/
void CsrBtTdDbDeInit(void);
#endif

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbListDevices
 *
 *  DESCRIPTION
 *      This function is called to get the data about all the devices whose rank
 *      is less than count(i.e. First argument of the function).
 *      An 8 bit unsigned integer value is returned denoting the total number of
 *      devices in the database.
 *  PARAMETERS
 *      count: It's an unsigned 8 bit integer. This argument is to define the
 *             upper limit of ranks of the devices which are to be found.
 *       addr: It's of type CsrBtTypedAddr. This argument is where the list of
 *             all the required devices is returned.
 *----------------------------------------------------------------------------*/
CsrUint8 CsrBtTdDbListDevices(CsrUint8 count, CsrBtTypedAddr* addr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbCountDevices
 *
 *  DESCRIPTION
 *      This macro is called to get the total number of devices in the database.
 *      It uses CsrBtTdDbListDevices function with count argument as 0 and addr
 *      argument a NULL.
 *      An 8 bit unsigned integer value is returned denoting the total number of
 *      devices in the database.
 *  PARAMETERS
 *      This macro function does not have any parameters.
 *----------------------------------------------------------------------------*/
#define CsrBtTdDbCountDevices()             CsrBtTdDbListDevices(0, NULL)

     /*----------------------------------------------------------------------------*
      *  NAME
      *      CsrBtTdDbWriteEntry
      *
      *  DESCRIPTION
      *      This function is called with the following parameters to store the data
      *      of the required device into the database.
      *      This function returns a result code of CsrBtResultCode type. The result
      *      code will be CSR_BT_RESULT_CODE_TD_DB_SUCCESS if the call to this
      *      function is successful and if the call fails the result code will differ
      *      depending on the error.
      *
      *  PARAMETERS
      *      This function takes 6 arguments.

      *      addressType:  This argument has the type of address of the device.
      *      deviceAddr:   This argument has the address of the device. This argument
      *                    is a const argument.
      *      source: 	  This is an 8-bit unsigned integer which tells from where
      *                    the function is called,i.e., 0 for GAP, 1 for SC, 2 for
      *                    GATT and 3 for DIRECT.
      *      key: 		  This argument is a 16-bit unsigned integer. This argument
      *                    is the key to the entry of the data in the database.
      *      length: 	  This argument is a 16-bit unsigned integer. This argument
      *                    is the length of the value that has been passed.
      *      value:        This argument is a const pointer of type void. This argument
      *                    contains the data to be added into the database.
      *----------------------------------------------------------------------------*/
    CsrBtResultCode CsrBtTdDbWriteEntry(CsrBtAddressType addressType,
        const CsrBtDeviceAddr* deviceAddr,
        CsrBtTdDbSource source,
        CsrUint16 key,
        CsrUint16 length,
        const void* value);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbReadEntry
 *
 *  DESCRIPTION
 *      This function is called in order to get the information
 *      about a trusted device from the database. In this function the length argument
 *      represents the length of data to be read, if the length of data read is
 *      different then the length argument is updated to new value.
 *
 *      This function returns a result code of CsrBtResultCode type. The result
 *      code will be CSR_BT_RESULT_CODE_TD_DB_SUCCESS if the call to this function
 *      is successful and if the call fails the result code will differ depending
 *      on the error.
 *
 *      Note: if length=0 and value=NULL, this API fetches the minimum buffer size
 *            required for the key, into the length parameter.
 *
 * PARAMETERS
 *      addressType:  Type of the address for the device identified by
 *                    parameter deviceAddr.
 *      deviceAddr:   Bluetooth address of the trusted device.
 *      source:       This is an 8-bit unsigned integer which tells from which
 *                    key group the function is called. Refer to
 *                    CSR_BT_TD_DB_SOURCE_X above for more details.
 *      key:          This argument is a 16-bit unsigned integer. This argument
 *                    is the key to the entry of the data in the database.
 *      length:       This argument is a 16-bit unsigned integer. This argument
 *                    works both as input and output. After the value
 *                    has been read from the database it's length is
 *                    returned through this argument. Any non-zero value passed
 *                    by the caller must be word aligned.
 *      value:        This argument is a pointer of type void. This argument
 *                    contains the buffer supplied from the caller to read the
 *                    data from the database. If not NULL, value pointer must
 *                    be word aligned.
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtTdDbReadEntry(CsrBtAddressType addressType,
                                   const CsrBtDeviceAddr* deviceAddr,
                                   CsrBtTdDbSource source,
                                   CsrUint16 key,
                                   CsrUint16* length,
                                   void* value);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbGetEntry
 *
 *  DESCRIPTION
 *      This function is called with the following parameters to get the information
 *      about a device from the database. In this function the length argument
 *      represents the length of data to be read, if the length of data read is
 *      different then an error is thrown.
 *
 *      This function returns a result code of CsrBtResultCode type. The result
 *      code will be CSR_BT_RESULT_CODE_TD_DB_SUCCESS if the call to this
 *      function is successful and if the call fails the result code will differ
 *      depending on the error.
 *
 * PARAMETERS
 *      addressType:  Type of the address for the device identified by
 *                    parameter deviceAddr.
 *      deviceAddr:   Bluetooth address of the trusted device.
 *      source:       This is an 8-bit unsigned integer which tells from which
 *                    key group the function is called. Refer to
 *                    CSR_BT_TD_DB_SOURCE_X above for more details.
 *      key:          This argument is a 16-bit unsigned integer. This argument
 *                    is the key to the entry of the data in the database.
 *      length:       This argument is a 16-bit unsigned integer. This argument
 *                    is the length of the value that must be read from the database.
 *                    Value of this must be word aligned.
 *      value:        This argument is a pointer of type void. This argument
 *                    contains the buffer supplied from the caller to read the
 *                    data from the database. This pointer must be word aligned.
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtTdDbGetEntry(CsrBtAddressType addressType,
                                  const CsrBtDeviceAddr* deviceAddr,
                                  CsrBtTdDbSource source,
                                  CsrUint16 key,
                                  CsrUint16 length,
                                  void* value);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbReadEntryByIndex
 *
 *  DESCRIPTION
 *      This function is called with the following arguments to get the information
 *      regarding the required device from the database. This function differs
 *      from CsrBtTdDbReadEntry function only in terms that this uses index of
 *      device instead of address of device to search for the trusted device.
 *
 *      This function returns a result code of CsrBtResultCode type. The result
 *      code will be CSR_BT_RESULT_CODE_TD_DB_SUCCESS if the call to this function
 *      is successful and if the call fails the result code will differ depending
 *      on the error.
 *
 * PARAMETERS
 *      deviceIndex:  This argument is an 8-bit unsigned integer. This denotes
 *                    the index (or rank) of device in the database.
 *      source:       This is an 8-bit unsigned integer which tells from which
 *                    key group the function is called. Refer to
 *                    CSR_BT_TD_DB_SOURCE_X above for more details.
 *      key:          This argument is a 16-bit unsigned integer. This argument
 *                    is the key to the entry of the data in the database.
 *      length:       This argument is a 16-bit unsigned integer. This argument
 *                    works both as input and output. After the value
 *                    has been read from the database it's length is
 *                    returned through this argument. Any non-zero value passed
 *                    by the caller must be word aligned.
 *      value:        This argument is a pointer of type void. This argument
 *                    contains the buffer supplied from the caller to read the
 *                    data from the database. If not NULL, value pointer must
 *                    be word aligned.
 *      addr:         This argument is pointer of type CsrBtTypedAddr. This
 *                    argument will hold the type of address and the address
 *                    of the device.
 *      isPriority    This is an output argument which indicates if the fetched
 *                    device is a priority device or not.
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtTdDbReadEntryByIndex(CsrUint8 deviceIndex,
                                          CsrBtTdDbSource source,
                                          CsrUint16 key,
                                          CsrUint16* length,
                                          void* value,
                                          CsrBtTypedAddr* addr,
                                          CsrBool *isPriority);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbGetEntryByIndex
 *
 *  DESCRIPTION
 *      This function is called with the following arguments to get the information
 *      regarding the required device from the database. This function differs
 *      from CsrBtTdDbGetEntry function only in terms that this uses index of
 *      device instead of address of device to search for the device.
 *
 *      This function returns a result code of CsrBtResultCode type. The result
 *      code will be CSR_BT_RESULT_CODE_TD_DB_SUCCESS if the call to this
 *      function is successful and if the call fails the result code will differ
 *      depending on the error.
 *
 * PARAMETERS
 *      deviceIndex:  This argument is an 8-bit unsigned integer. This denotes
 *                    the index (or rank) of device in the database.
 *      source:       This is an 8-bit unsigned integer which tells from which
 *                    key group the function is called. Refer to
 *                    CSR_BT_TD_DB_SOURCE_X above for more details.
 *      key:          This argument is a 16-bit unsigned integer. This argument
 *                    is the key to the entry of the data in the database.
 *      length:       This argument is a 16-bit unsigned integer. This argument
 *                    is the length of the value that must be read from the database.
 *                    Value of this argument must be word aligned.
 *      value:        This argument is a pointer of type void. This argument
 *                    contains the buffer supplied from the caller to read the
 *                    data from the database. This pointer must be word aligned.
 *      addr:         This argument is pointer of type CsrBtTypedAddr. This
 *                    argument will hold the type of address and the address
 *                    of the device.
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtTdDbGetEntryByIndex(CsrUint8 deviceIndex,
                                         CsrBtTdDbSource source,
                                         CsrUint16 key,
                                         CsrUint16 length,
                                         void* value,
                                         CsrBtTypedAddr* addr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbEntryExists
 *
 *  DESCRIPTION
 *      This macro function is called to find whether a particular entry exists
 *      in the database or not. This macro function makes use of csrBtTdDbReadEntry,
 *      with length and value arguments being passed as NULL and remaining arguments
 *      passed respectively from this macro function, by using equality
 *      operator (==)  with with CSR_BT_RESULT_CODE_TD_DB_SUCCESS.
 *      This macro function returns true if the required entry exists in the
 *      database and returns false if the required entry is not found in the database.
 *
 *  PARAMETERS
 *      This macro function takes 4 arguments.
 *      _addressType: This argument is of type CsrBtAddressType. This argument
 *                    has the type of address of the device.
 *      _deviceAddr:  This argument has the address of the device. This argument
 *                    is a const pointer of type CsrBtDeviceAddr.
 *      _source:      This is an 8-bit unsigned integer which tells from where
 *                    the function is called,i.e., 0 for GAP, 1 for SC,
 *                    2 for GATT and 3 for DIRECT.
 *      _key:         This argument is a 16-bit unsigned integer. This argument
 *                    is the key to the entry of the data in the database.
 *----------------------------------------------------------------------------*/
#define CsrBtTdDbEntryExists(_addressType,                                  \
                             _deviceAddr,                                   \
                             _source,                                       \
                             _key)                                          \
    (CsrBtTdDbReadEntry(_addressType,                                       \
                        _deviceAddr,                                        \
                        _source,                                            \
                        _key,                                               \
                        NULL,                                               \
                        NULL) == CSR_BT_RESULT_CODE_TD_DB_SUCCESS)

 /*----------------------------------------------------------------------------*
  *  NAME
  *      CsrBtTdDbDeviceExists
  *
  *  DESCRIPTION
  *      This macro function is called to verify whether a particular device exists
  *      in the database or not. This macro function uses CsrBtTdDbEntryExists macro
  *      function with _source and _key arguments as 0 while _addressType and
  *      _deviceAddr are passed directly from the arguments of this macro function.
  *      This macro function returns true if the device exists and false if the
  *      device does not exist.
  *
  *  PARAMETERS
  *      This macro function takes 2 arguments:-
  *      _addressType: This argument is of type CsrBtAddressType. This argument
                       has the type of address of the device.
  *      _deviceAddr:  This argument has the address of the device. This argument
                       is a const pointer of type CsrBtDeviceAddr.
  *----------------------------------------------------------------------------*/
#define CsrBtTdDbDeviceExists(_addressType, _deviceAddr)                    \
    CsrBtTdDbEntryExists(_addressType, _deviceAddr, 0, 0)

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbDeleteDevice
 *
 *  DESCRIPTION
 *      This API is called to remove a trusted device from the persistent
 *      trusted device list. A flag indicating if the device was successfully
 *      removed or not is returned.
 *
 *      Note: This API is meant only to update PS and not to propagate the
 *            same change to lower layers. In case if the requirement is
 *            to update lower layers and PS both, use API CmScDmRemoveDeviceReq
 *            instead.
 *
 * PARAMETERS
 *      addressType:    Type of bluetooth address of the device.
 *      deviceAddr:     Bluetooth address of the device to be deleted.
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtTdDbDeleteDevice(CsrBtAddressType addressType, const CsrBtDeviceAddr* deviceAddr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbDeleteAll
 *
 *  DESCRIPTION
 *      This API is called to remove all trusted devices from the persistent
 *      trusted device list using a filter provided in order to refrain some
 *      devices from getting deleted.
 *
 *      Note: This API is meant only to update PS and not to propagate the
 *            same change to lower layers. In case if the requirement is
 *            to update lower layers and PS both, use API CmScDmRemoveDeviceReq
 *            instead.
 *
 * PARAMETERS
 *      filter:     Mask to filter certain set of devices from deleting.
 *                  CSR_BT_TD_DB_FILTER_EXCLUDE_PRIORITY -
 *                  make sures that the priority devices are not deleted.
 *                  CSR_BT_TD_DB_FILTER_EXCLUDE_NONE -
 *                  filter none which means all the devices will be deleted.
 *----------------------------------------------------------------------------*/
void CsrBtTdDbDeleteAll(CsrBtTdDbFilter filter);

/****************************************************************************
    NAME
        CsrBtTdDbPrioritiseDevice

    FUNCTION
        This function is called to keep a track of the most recently used device.
        The TDI index is updated provided that the device specified is currently
        stored in the TDL.

    RETURNS
        TRUE if device specified is in the TDL, otherwise FALSE
*/
CsrBtResultCode CsrBtTdDbPrioritiseDevice(CsrBtAddressType addressType,
    const CsrBtDeviceAddr* deviceAddr,
    CsrBtTdDbUpdateFlag options);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbSetSystemInfo
 *
 *  DESCRIPTION
 *      This function is called to store the system's information into the database.
 *      This function returns a result code with value "CSR_BT_RESULT_CODE_TD_DB_SUCCESS"
 *      if the write was successful and a result code with value
 *      "CSR_BT_RESULT_CODE_TD_DB_WRITE_FAILED" if the write was unsuccessful.
 *
 *  PARAMETERS
 *      This function takes an argument of type "CsrBtTdDbSystemInfo".
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtTdDbSetSystemInfo(const CsrBtTdDbSystemInfo* systemInfo);

/*----------------------------------------------------------------------------*
 *  NAME
 *      CsrBtTdDbGetSystemInfo
 *
 *  DESCRIPTION
 *      This function is called to get the system's information stored in the database.
 *      This function returns 2 things:-
 *          1. A result code of value "CSR_BT_RESULT_CODE_TD_DB_SUCCESS" is returned
 *             if the read was successful and a result code of value
 *             "CSR_BT_RESULT_CODE_TD_DB_WRITE_FAILED" is returned if the read
 *             was unsuccessful.
 *          2. This function returns system information in the argument( i.e. systemInfo)
 *             passed.
 *
 *  PARAMETERS
 *      This function takes an argument of type "CsrBtTdDbSystemInfo". The system
 *      information is returned in this argument.
 *----------------------------------------------------------------------------*/
CsrBtResultCode CsrBtTdDbGetSystemInfo(CsrBtTdDbSystemInfo* systemInfo);

#ifdef __cplusplus
}
#endif

#endif /* __CSR_BT_TD_DB_H_ */
