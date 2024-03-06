/****************************************************************************
 * Copyright (c) 2014 - 2019 Qualcomm Technologies International, Ltd.
 ***************************************************************************/
#ifndef HAL_DM_SECTIONS_H
#define HAL_DM_SECTIONS_H

/****************************************************************************/
/* DM Bank sections.  */
typedef enum
{
    DM1 = 0,
    DM2 = 1,
}DM;

/* DM data sections */
#ifdef __KCC__

#define DM_SHARED       _Pragma("datasection DM_SHARED")
#define DM_SHARED_ZI    _Pragma("datasection DM_SHARED_ZI")
#define DM_P0_RW        _Pragma("datasection DM1_P0_RW")
#define DM_P0_RW_ZI     _Pragma("datasection DM1_P0_RW_ZI")
#define DM_P1_RW        _Pragma("datasection DM2_P1_RW")
#define DM_P1_RW_ZI     _Pragma("datasection DM2_P1_RW_ZI")
#define DM1_DEBUG       DM_P1_RW_ZI
#define DM2_DEBUG       DM_P0_RW_ZI

#else /* __KCC__ */

#define DM_SHARED
#define DM_SHARED_ZI
#define DM_P0_RW
#define DM_P0_RW_ZI
#define DM_P1_RW
#define DM_P1_RW_ZI
#define DM1_DEBUG
#define DM2_DEBUG

#endif /* __KCC__ */

#endif /* HAL_DM_SECTIONS_H */
