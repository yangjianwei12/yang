#ifndef _CSR_SYNERGY_H
#define _CSR_SYNERGY_H
/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

#include "csr_components.h"

#define CSR_PLATFORM_HYDRA

#define CSR_FUNCATTR_NORETURN(x) x

/* Versioning */
#define CSR_VERSION_NUMBER(major,minor,fix) ((major * 10000) + (minor * 100) + fix)

/* Overall configuration */
#ifndef CSR_ENABLE_SHUTDOWN
/* #undef CSR_ENABLE_SHUTDOWN */
#endif

#ifndef EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
#define EXCLUDE_CSR_EXCEPTION_HANDLER_MODULE
#endif

#ifndef CSR_EXCEPTION_PANIC
#define CSR_EXCEPTION_PANIC 1
#endif

#ifndef CSR_CHIP_MANAGER_TEST_ENABLE
/* #undef CSR_CHIP_MANAGER_TEST_ENABLE */
#endif

#ifndef CSR_CHIP_MANAGER_QUERY_ENABLE
/* #undef CSR_CHIP_MANAGER_QUERY_ENABLE */
#endif

#ifndef CSR_CHIP_MANAGER_ENABLE
/* #undef CSR_CHIP_MANAGER_ENABLE */
#endif

#ifndef CSR_BUILD_DEBUG
/* #undef CSR_BUILD_DEBUG */
#endif

#ifndef CSR_INSTRUMENTED_PROFILING_SERVICE
/* #undef CSR_INSTRUMENTED_PROFILING_SERVICE */
#endif

#ifndef CSR_LOG_ENABLE
/* #undef CSR_LOG_ENABLE */
#endif

#ifndef CSR_USE_QCA_CHIP
/* #undef CSR_USE_QCA_CHIP */
#endif

#ifndef CSR_QCA_QVSC
#define CSR_QCA_QVSC
#endif

#ifndef CSR_QCA_CHIP_CTL_LOG_ENABLE
/* #undef CSR_QCA_CHIP_CTL_LOG_ENABLE */
#endif

#ifndef CSR_QCA_CHIP_32BIT_SOC_SUPPORT
/* #undef CSR_QCA_CHIP_32BIT_SOC_SUPPORT */
#endif

#ifndef CSR_ASYNC_LOG_TRANSPORT
/* #undef CSR_ASYNC_LOG_TRANSPORT */
#endif

#ifndef CSR_AMP_ENABLE
/* #undef CSR_AMP_ENABLE */
#endif

#ifndef CSR_BT_LE_ENABLE
#define CSR_BT_LE_ENABLE
#endif

#ifndef CSR_STREAMS_ENABLE
#define CSR_STREAMS_ENABLE
#endif

#define NONE        0
#define VM          1
#define IOT         2
#define AUTO        3
#define WEARABLE    4

#if (VM == VM)
#define CSR_TARGET_PRODUCT_VM
#elif (VM == IOT)
#define CSR_TARGET_PRODUCT_IOT
#elif (VM == AUTO)
#define CSR_TARGET_PRODUCT_AUTO
#elif (VM == WEARABLE)
#define CSR_TARGET_PRODUCT_WEARABLE
#else
#define CSR_TARGET_PRODUCT_NONE
#endif /* CSR_TARGET_PRODUCT */

#define CSR_FRW_BUILDSYSTEM_AVAILABLE
#ifdef CSR_FRW_BUILDSYSTEM_AVAILABLE
#include "csr_frw_config.h"
#endif

#ifdef CSR_COMPONENT_BT
#include "csr_bt_config.h"
#endif

/* #undef CSR_WIFI_BUILDSYSTEM_AVAILABLE */
#ifdef CSR_WIFI_BUILDSYSTEM_AVAILABLE
#include "csr_wifi_config.h"
#endif

/* #undef CSR_MERCURY_BUILDSYSTEM_AVAILABLE */
#ifdef CSR_MERCURY_BUILDSYSTEM_AVAILABLE
#include "csr_mercury_config.h"
#endif

/* Do not edit this area - Start */
#ifdef CSR_LOG_ENABLE
#ifdef CSR_TARGET_PRODUCT_VM
#define SYNERGY_LOG_FUNC_ENABLED
#endif
#ifndef CSR_LOG_INCLUDE_FILE_NAME_AND_LINE_NUMBER
#define CSR_LOG_INCLUDE_FILE_NAME_AND_LINE_NUMBER
#endif
#endif

#ifdef CSR_BUILD_DEBUG
#ifndef MBLK_DEBUG
#define MBLK_DEBUG
#endif
#endif

#ifdef CSR_ENABLE_SHUTDOWN
#ifndef ENABLE_SHUTDOWN
#define ENABLE_SHUTDOWN
#endif
#endif

#ifdef CSR_EXCEPTION_PANIC
#ifndef EXCEPTION_PANIC
#define EXCEPTION_PANIC
#endif
#endif

#ifndef FTS_VER
#define FTS_VER "9.9.19.0"
#endif

#ifndef CSR_QCA_BAUD_NO_RSP_SKIP_PANIC
/* #undef CSR_QCA_BAUD_NO_RSP_SKIP_PANIC */
#endif

#ifndef CSR_SERIAL_COM_TX_USE_TCFLUSH
/* #undef CSR_SERIAL_COM_TX_USE_TCFLUSH */
#endif

#ifndef CSR_FRW_INSTALL_TRANS_RECOVERY
/* #undef CSR_FRW_INSTALL_TRANS_RECOVERY */
#endif

#ifndef CSR_FRW_INSTALL_BT_CRASH_DUMP
/* #undef CSR_FRW_INSTALL_BT_CRASH_DUMP */
#endif

#ifndef CSR_FRW_BT_CRASH_DUMP_PATH
#define CSR_FRW_BT_CRASH_DUMP_PATH "."
#endif

/* Do not edit this area - End */

#endif /* _CSR_SYNERGY_H */
