/*!
\copyright  Copyright (c) 2021 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Initialisation of qualcomm specific information for secure connection and qhs.
*/

#ifdef INCLUDE_QCOM_CON_MANAGER

#include <qualcomm_connection_manager.h>


/*!< Qualcomm specific vendor and lmp version information
     to enable secure connnection and qhs between dongle
     and remote device*/

#define LMP_COMPID_QUALCOMM        0x001D
#define LMP_COMPID_QTIL            0x000A
#define MIN_LMP_VERSION            9
#define MIN_LMP_SUB_VERSION_2000   2000      /* Required with LMP_COMPID_QUALCOMM*/
#define MIN_LMP_SUB_VERSION_3000   3000      /* Required with LMP_COMPID_QTIL*/


const QCOM_CON_MANAGER_SC_OVERRIDE_VENDOR_INFO_T vendor_info[] =
{
    /*comp_id,               min_lmp_version,     min_lmp_sub_version */
#ifdef QCOM_BLUETOOTH_HIGH_SPEED
    {LMP_COMPID_QUALCOMM,    MIN_LMP_VERSION,     MIN_LMP_SUB_VERSION_2000},
#endif
    {0,                      0,                   0                       }
};

#endif /* INCLUDE_QCOM_CON_MANAGER */
