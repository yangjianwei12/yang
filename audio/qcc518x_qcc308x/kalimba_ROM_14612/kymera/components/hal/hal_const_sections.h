/****************************************************************************
 * Copyright (c) 2014 - 2021 Qualcomm Technologies International, Ltd.
 ***************************************************************************/
#ifndef HAL_CONST_SECTIONS_H
#define HAL_CONST_SECTIONS_H

#ifdef __KCC__
#define DMCONST _Pragma("datasection DMCONST")
#define DMCONST16 _Pragma("datasection DMCONST16")
#define DMCONST_WINDOWED16 _Pragma("datasection DMCONST_WINDOWED16")
#define CONST   _Pragma("datasection CONST")
#define CONST16 _Pragma("datasection CONST16")
#define CONST_SLT_REF _Pragma("datasection CONST_SLT_REF")
#else /* __KCC__ */
#define DMCONST
#define DMCONST16
#define DMCONST_WINDOWED16
#define CONST
#define CONST16
#define CONST_SLT_REF
#endif /* __KCC__ */

#endif /* HAL_CONST_SECTIONS_H */
