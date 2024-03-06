/*!
\copyright  Copyright (c) 2014-2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Utilities for working with byte arrays.

@{
*/

#ifndef BYTE_UTILS_H_
#define BYTE_UTILS_H_

#include <csrtypes.h>

#define MAKEWORD_HI_LO(msb, lsb) ((uint16)(((uint16)((uint8)((msb) & 0xff))) << 8 | ((uint8)((lsb) & 0xff))))
#define MAKEWORD(lsb, msb)       ((uint16)(((uint8)((lsb) & 0xff)) | ((uint16)((uint8)((msb) & 0xff))) << 8))
#define MAKELONG(lsw, msw)       ((uint32)(((uint16)((lsw) & 0xffff)) | ((uint32)((uint16)((msw) & 0xffff))) << 16))
#define LOWORD(l)                ((uint16)((l) & 0xffff))
#define HIWORD(l)                ((uint16)((l) >> 16))
#define LOBYTE(w)                ((uint8)((w) & 0xff))
#define HIBYTE(w)                ((uint8)((w) >> 8))

/* Return true is bit set in mask are set in word */
#define ByteUtilsAreBitsSet(word, mask) (((word) & (mask)) == (mask))
#define ByteUtilsAreBitsClear(word, mask) (((word) & (mask)) == 0)

/*! Writes data from source address to destination address. Will convert each byte to 16-bit, then write these to the dst in 16-bit little endian for non-HYDRACORE, 
    else copies data verbatim.

    Non-HYDRACORE example:
    uint8 src[4] = { 0x12 0x34 0x56 0x78 };
    uint8 dst[8] = {0};
    ByteUtilsMemCpyToStream(dst, src, 8);   
    dst = { 0x0 0x12 0x0 0x34 0x0 0x56 0x0 0x78 }
    
    HYDRACORE example:
    uint8 src[4] = { 0x12 0x34 0x56 0x78 };
    uint8 dst[8] = {0};
    ByteUtilsMemCpyToStream(dst, src, 4);
    dst = { 0x12 0x34 0x56 0x78 }

    \param dst A pointer to the address to populate
    \param src A pointer to the address to get data from
    \param size The number of bytes in the 16-bit little endian output (amount of bytes to copy * 2) for non-HYDRACORE, else amount of bytes to copy.
    \return The size parameter
*/
uint16 ByteUtilsMemCpyToStream(uint8 *dst, const uint8 *src, uint16 size);

/*! Writes data from source address to destination address in 16-bit little endian for non-HYDRACORE, 
    else copies data verbatim.

    Non-HYDRACORE example:
    uint8 src[4] = { 0x12 0x34 0x56 0x78 };
    uint8 dst[4] = {0};
    ByteUtilsMemCpyFromStream(dst, src, 8);
    dst = { 0x34 0x12 0x78 0x56 }
    
    HYDRACORE example:
    uint8 src[4] = { 0x12 0x34 0x56 0x78 };
    uint8 dst[4] = {0};
    ByteUtilsMemCpyFromStream(dst, src, 4);  
    dst = { 0x12 0x34 0x56 0x78 }

    \param dst A pointer to the address to populate
    \param src A pointer to the address to get data from
    \param size The number of bytes to copy.
    \return The size parameter
*/
uint16 ByteUtilsMemCpyFromStream(uint8 *dst, const uint8 *src, uint16 size);

/*! Writes data from the source array at the specified index to the destination array at the specified index

    When working with odd byte indices the functionality varies slightly, see below examples.

    Non-HYDRACORE example:
    uint8 src[6] =  { 0x00, 0x12, 0x34, 0x56, 0x78, 0x00 };
    uint8 dst[4] = {0};
    ByteUtilsMemCpy(dst, 0, src, 1, 4);
    dst = { 0x56 0x0 0x0 0x34 }
    
    HYDRACORE example:
    uint8 src[6] =  { 0x00, 0x12, 0x34, 0x56, 0x78, 0x00 };
    uint8 dst[4] = {0};
    ByteUtilsMemCpy(dst, 0, src, 1, 4);
    dst = { 0x12 0x34 0x56 0x78  }

    \param dst A pointer to the address to populate
    \param dstIndex Offset into the destination address to populate
    \param src A pointer to the address to get data from
    \param srcIndex Offset into the source address to get data from
    \param size The number of bytes to copy.
    \return The size parameter
*/
uint16 ByteUtilsMemCpy(uint8 *dst, uint16 dstIndex, const uint8 *src, uint16 srcIndex, uint16 size);

/*! Writes data from the uint16 source array at the specified index to the uint8 destination array at the specified index

    When working with odd byte indices the functionality varies slightly, see below examples.

    Non-HYDRACORE example:
    uint16 src[3] =  { 0x0012, 0x3456, 0x7800 };
    uint8 dst[4] = {0};
    ByteUtilsMemCpy(dst, 0, src, 1, 4);
    dst = { 0x34 0x12 0x78 0x56 }
    
    HYDRACORE example:
    uint16 src[3] =  { 0x0012, 0x3456, 0x7800 };
    uint8 dst[4] = {0};
    ByteUtilsMemCpy(dst, 0, src, 1, 4);
    dst = { 0x00 0x56 0x34 0x00   }

    \param dst A pointer to the address to populate
    \param dstIndex Offset into the destination address to populate
    \param src A pointer to the address to get data from
    \param srcIndex Offset into the source address to get data from
    \param size The number of bytes to copy.
    \return The size parameter
*/
uint16 ByteUtilsMemCpy16(uint8 *dst, uint16 dstIndex, const uint16 *src, uint16 srcIndex, uint16 size);

/*! Packs data from a uint8 array into a uint16 array

    Example:
    uint8 src[4] = { 0x12 0x34 0x56 0x78 };
    uint16 dst[2] = {0};
    ByteUtilsMemCpyPackString(dst, src, 4);
    
    dst = { 0x1234 0x5678 }

    \param dst A pointer to the uint16 array to populate
    \param src A pointer to the uint8 array to get data from
    \param size The number of bytes to copy.
    \return The size parameter
*/
uint16 ByteUtilsMemCpyPackString(uint16 *dst, const uint8 *src, uint16 size);

/*! Unpacks data from a uint16 array into a uint8 array

    Example:
    uint16 src[2] = { 0x1234 0x5678 };
    uint8 dst[4] = {0};
    ByteUtilsMemCpyUnpackString(dst, src, 4);
    
    dst = { 0x12 0x34 0x56 0x78 }

    \param dst A pointer to the uint8 array to populate
    \param src A pointer to the uint16 array to get data from
    \param size The number of bytes to copy.
    \return The size parameter
*/
uint16 ByteUtilsMemCpyUnpackString(uint8 *dst, const uint16 *src, uint16 size);

/*! Get the length of uint16 array in bytes

    Example:
    uint16 buf[2] = { 0x1234 0x5678 };
    ByteUtilsGetPackedStringLen(buf, 2);
    
    output = 4

    \param src A pointer to the uint16 array
    \param max_len The length of the uint16 array
    \return The size in bytes
*/
uint16 ByteUtilsGetPackedStringLen(const uint16 *src, const uint16 max_len);

/*! Writes a single byte to the destination array at the index specified

    \param dst A pointer to the array to populate
    \param byteIndex The array index at which to write
    \param val The value to write
    \return The number of bytes written
*/
uint16 ByteUtilsSet1Byte(uint8 *dst, uint16 byteIndex, uint8 val);

/*! Writes two bytes to the destination array at the index specified in big endian for HYDRACORE, else little endian

    \param dst A pointer to the array to populate
    \param byteIndex The array index at which to write
    \param val The value to write
    \return The number of bytes written
*/
uint16 ByteUtilsSet2Bytes(uint8 *dst, uint16 byteIndex, uint16 val);

/*! Writes four bytes to the destination array at the index specified in big endian for HYDRACORE, else little endian

    \param dst A pointer to the array to populate
    \param byteIndex The array index at which to write
    \param val The value to write
    \return The number of bytes written
*/
uint16 ByteUtilsSet4Bytes(uint8 *dst, uint16 byteIndex, uint32 val);

/*! Reads a single byte from the address specified

    \param src The address to read data from
    \return The value read
*/
uint8 ByteUtilsGet1ByteFromStream(const uint8 *src);

/*! Reads two bytes from the address specified. Expects big endian for HYDRACORE, else little endian

    \param src The address to read data from
    \return The value read
*/
uint16 ByteUtilsGet2BytesFromStream(const uint8 *src);

/*! Reads four bytes from the address specified. Expects big endian for HYDRACORE, else little endian

    \param src The address to read data from
    \return The value read
*/
uint32 ByteUtilsGet4BytesFromStream(const uint8 *src);
 
/*! Reads a single byte from the source array at the index specified. Expects little endian for non-HYDRACORE, else big endian.

    \param src A pointer to the array to read
    \param byteIndex The array index at which to read
    \param val A pointer to populate with the read value
    \return The number of bytes read
*/
uint16 ByteUtilsGet1Byte(const uint8 *src, uint16 byteIndex, uint8 *val);

/*! Reads two bytes from the source array at the index specified. Expects little endian for HYDRACORE, else big endian

    When working with odd byte indices the functionality varies slightly, see below examples.
    
    HYDRACORE example:
    uint8 input[4] = { 0x00, 0x34, 0x12, 0x00 };
    ByteUtilsGet2Bytes(input, 1, &ouput);
    output = 0x1234
    
    Non-HYDRACORE example:
    uint8 input[4] = { 0x12, 0x00, 0x00, 0x34 };
    ByteUtilsGet2Bytes(input, 1, &ouput);
    output = 0x1234

    \param src A pointer to the array to read
    \param byteIndex The array index at which to read
    \param val A pointer to populate with the read value
    \return The number of bytes read
*/
uint16 ByteUtilsGet2Bytes(const uint8 *src, uint16 byteIndex, uint16 *val);

/*! Reads four bytes from the source array at the index specified. Expects little endian for HYDRACORE, else big endian

    When working with odd byte indices the functionality varies slightly, see below examples.
    
    HYDRACORE example:
    uint8 input[6] = { 0x00, 0x34, 0x12, 0x78, 0x56, 0x00 };
    ByteUtilsGet4Bytes(input, 1, &output);
    output = 0x12345678
    
    Non-HYDRACORE example: 
    uint8 input[6] = { 0x12, 0x00, 0x56, 0x34, 0x00, 0x78 };
    ByteUtilsGet4Bytes(input, 1, &output);
    output = 0x12345678

    \param src A pointer to the array to read
    \param byteIndex The array index at which to read
    \param val A pointer to populate with the read value
    \return The number of bytes read
*/
uint16 ByteUtilsGet4Bytes(const uint8 *src, uint16 byteIndex, uint32 *val);

/*! Unpacks and copies up to dstsize - 1 characters from the string src to dst,
    NUL-terminating the result if dstsize is not 0.

    If the return value is >= dstsize, the output string has been truncated.
    It is the caller's responsibility to handle this.

    \param dst Pointer to destination address
    \param src Pointer to source address
    \uint16 dstsize Size of destination buffer

    \return The string length
 */
uint16 ByteUtilsStrLCpyUnpack(uint8 *dst, const uint16 *src, uint16 dstsize);

/*! Implementation of strnlen_s from C11, that is not yet available.

    The function returns the length of the string. If the string 
    exceeds the supplied max_length then max_length is returned.
    If src is NULL then zero is returned

    \param[in] string Pointer to string to find length of
    \param max_length Maximum allowed length of the supplied string

    \return the string length
 */
uint16 ByteUtilsStrnlen_S(const uint8 *string, uint16 max_length);

/*!
 *  \brief Packs an unsigned 32-bit variable into a uint8 array in little endian
 *         Example: 0x11223344 -> [0x44 0x33 0x22 0x11]
 *  \param data The unsigned 32-bit variable to be packed
 *  \param destination A pointer to the starting address of the destination
 */
void ByteUtils32toLE(uint32_t data, uint8_t *destination);

/*!
 *  \brief Packs an unsigned 16-bit variable into a uint8 array in little endian
 *         Example: 0x1122 -> [0x22 0x11]
 *  \param data The unsigned 16-bit variable to be packed
 *  \param destination A pointer to the starting address of the destination
 */
void ByteUtils16toLE(uint16_t data, uint8_t *destination);

/*!
 *  \brief Unpacks an unsigned 32-bit variable from a uint8 array in little endian
 *         Example: [0x44 0x33 0x22 0x11] -> 0x11223344
 *  \param data A pointer to the starting address of the memory from which to unpack the variable
 *  \return The unpacked unsigned 32-bit variable
 */
uint32 ByteUtilsLEto32(uint8_t * data);

/*!
 *  \brief Unpacks an unsigned 16-bit variable from a uint8 array in little endian
 *         Example: [0x22 0x11] -> 0x1122
 *  \param data A pointer to the starting address of the memory from which to unpack the variable
 *  \return The unpacked unsigned 16-bit variable
 */
uint16 ByteUtilsLEto16(uint8_t * data);

#endif /* BYTE_UTILS_H_ */

/* @} */
