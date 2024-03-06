#ifndef CSR_HYDRA_TYPES_H__
#define CSR_HYDRA_TYPES_H__

/******************************************************************************
 Copyright (c) 2019-2022 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #2 $
******************************************************************************/

#if defined(HYDRA) || defined(CAA)

#include "hydra_types.h"

#else

#include <stddef.h> /* For ptrdiff_t and size_t */
#include <limits.h> /* For _MAX,_MIN macros */

/*!
 * \class uint8
 * \brief Portable type 8 bit unsigned integer
 * \note On earlier platforms a uint8 was the same size as uint16.
 */
typedef unsigned char   uint8;
/*!
 * \class uint16
 * \brief Portable type 16 bit unsigned integer
 */
typedef unsigned short  uint16;
/*!
 * \class uint32
 * \brief Portable type 32 bit unsigned integer
 */
typedef unsigned long   uint32;

typedef uint32          uint24;


/*!
 * \class int8
 * \brief Portable type 8 bit signed integer
 * \note On earlier platforms an int8 was the same size as int16.
 */
typedef signed char     int8;
/*!
 * \class int16
 * \brief Portable type 16 bit signed integer
 */
typedef signed short    int16;
/*!
 * \class int32
 * \brief Portable type 32 bit signed integer
 */
typedef signed long     int32;


#ifndef __cplusplus
typedef unsigned        bool;
#endif

typedef unsigned int uintptr; /* Used in some legacy bluecore code*/

#endif /* !HYDRA */

#endif /* CSR_HYDRA_TYPES_H__ */
