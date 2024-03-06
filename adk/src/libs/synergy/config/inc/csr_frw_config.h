#ifndef _CSR_FRW_CONFIG_DEFAULT_H
#define _CSR_FRW_CONFIG_DEFAULT_H
/******************************************************************************
 Copyright (c) 2010-2023 Qualcomm Technologies International, Ltd.
 All Rights Reserved.
 Qualcomm Technologies International, Ltd. Confidential and Proprietary.

 REVISION:      $Revision: #60 $
******************************************************************************/

/*--------------------------------------------------------------------------
 * Version info
 *--------------------------------------------------------------------------*/
#define CSR_FRW_VERSION_MAJOR    3
#define CSR_FRW_VERSION_MINOR    6
#define CSR_FRW_VERSION_FIXLEVEL 0
#define CSR_FRW_VERSION_BUILD    0
#define CSR_FRW_RELEASE_TYPE_ENG
#ifdef CSR_FRW_RELEASE_TYPE_ENG
#define CSR_FRW_VERSION "3.6.0.0"
#else
#define CSR_FRW_VERSION "3.6.0"
#endif
#define CSR_FRW_VERSION_NUMBER CSR_VERSION_NUMBER(3, 6, 0)
#define CSR_FRW_VERSION_CHECK(major,minor,fix) (CSR_FRW_VERSION_NUMBER >= CSR_VERSION_NUMBER(major,minor,fix))

/*--------------------------------------------------------------------------
 * Misc defines for the framework
 *--------------------------------------------------------------------------*/
/* #undef CSR_MASK_ERROR_REASON_VALUES */
/* #undef CSR_IP_SUPPORT_FLOWCONTROL */
#define CSR_IP_SUPPORT_ETHER
#define CSR_IP_SUPPORT_IFCONFIG
/* #undef CSR_IP_SUPPORT_TLS */
/* #undef CSR_TLS_SUPPORT_PSK */
#define CSR_USE_STDC_LIB
/* #undef CSR_MEMALLOC_PROFILING */
/* #undef CSR_PMEM_DEBUG */

/*--------------------------------------------------------------------------
 * Defines for the IP and TLS interfaces
 *--------------------------------------------------------------------------*/
#define CSR_IP_MAX_ETHERS 8
#define CSR_IP_MAX_SOCKETS 16
#define CSR_TLS_MAX_SOCKETS 8

/*--------------------------------------------------------------------------
 * Defines for the application framework
 *--------------------------------------------------------------------------*/
#define CSR_SCHEDULER_INSTANCES 3

/*--------------------------------------------------------------------------
 * Defines for the generic scheduler
 *--------------------------------------------------------------------------*/
/*
 * The maximum number of messages to store in the
 * per-scheduler instance message container free list.
 * Helps reducing allocations in the message put path.
 */
#define CSR_SCHED_MESSAGE_POOL_LIMIT 10

/*
 * The maximum number of timers per scheduler instance
 */
#define CSR_SCHED_TIMER_POOL_LIMIT 100

/*
 * Enable BCCMD commands
 */

#define EXCLUDE_CSR_BCCMD_MODULE

/*-------------------------------------------------------------------------
 * Defines for MOJAVE and HANA
 *--------------------------------------------------------------------------*/
#define NIL     0
#define QCC5100_HOST   1
/* Auto Platforms */
#define MOJAVE  2
#define HANA    3

/* #undef CSR_PLATFORM_MOJAVE */

#if (NIL == QCC5100_HOST)
#define CSR_HOST_PLATFORM   QCC5100_HOST
#elif (NIL == MOJAVE)
#define CSR_HOST_PLATFORM   MOJAVE
#elif (NIL == HANA)
#define CSR_HOST_PLATFORM   HANA
#elif (defined(CSR_PLATFORM_MOJAVE))
#define CSR_HOST_PLATFORM   MOJAVE
#else
#define CSR_HOST_PLATFORM   NIL
#endif /* CSR_HOST_PLATFORM */

#undef NONE

/*-------------------------------------------------------------------------
 * Defines for SYN_BT_HOST_TRANSPORT
 *--------------------------------------------------------------------------*/
 
#define IPC 1

#if (NIL == IPC)
#define SYN_BT_HOST_TRANSPORT IPC
#else
#define SYN_BT_HOST_TRANSPORT   NIL
#endif /* SYN_BT_HOST_TRANSPORT */

/*---------------------------------------------------------------------------
 * Following defines are applicable only for QCA chips
 *--------------------------------------------------------------------------*/
#ifdef CSR_USE_QCA_CHIP

#ifdef EXCLUDE_CSR_BCCMD_MODULE
#define EXCLUDE_CSR_HQ_MODULE
#endif /* EXCLUDE_CSR_BCCMD_MODULE */

#define EXCLUDE_CSR_AM_MODULE
#define EXCLUDE_CSR_DHCP_SERVER_MODULE
#define EXCLUDE_CSR_DSPM_MODULE
#define EXCLUDE_CSR_FP_MODULE
#define EXCLUDE_CSR_IP_ETHER_MODULE
#define EXCLUDE_CSR_IP_IFCONFIG_MODULE
#define EXCLUDE_CSR_IP_SOCKET_MODULE
#define EXCLUDE_CSR_TFTP_MODULE
#define EXCLUDE_CSR_TLS_MODULE
#define EXCLUDE_CSR_VM_MODULE

#else /* CSR_USE_QCA_CHIP */
#define EXCLUDE_CSR_QVSC_MODULE

/*---------------------------------------------------------------------------
 * Following defines are not applicable for QCA chips
 *--------------------------------------------------------------------------*/

/* #undef CSR_DSPM_FORCE_PATCH_ENABLE */
/* #undef CSR_HYDRA_SSD */
/* #undef CSR_USE_BCSP_HTRANS */
#define CSR_SDIO_USE_SDIO
#define CSR_SDIO_USE_CSPI
#define CSR_SDIO_ASYNC_ENABLE
/* #undef CSR_HCI_SOCKET_TRANSPORT */

/*--------------------------------------------------------------------------
 * Defines for the BlueCore bootstrap procedure
 *--------------------------------------------------------------------------*/

/*
 * The fixed time (in us) to wait after a reset command, before the transport
 * is restarted.
 */
#define CSR_BLUECORE_RESET_TIMER 500000

/*
 * Enable this option to enable an application to control the activation and
 * deactivation of the BlueCore.
 */
/* #undef CSR_BLUECORE_ONOFF */

/*
 * The maximum time (in us) to wait for the BlueCore to come alive after
 * sending a reset command. Only applicable when CSR_BLUECORE_ONOFF is defined.
 */
#ifdef CSR_BLUECORE_ONOFF
#define CSR_BLUECORE_RESET_TIMEOUT 5000000
#endif

/*
 * Enable this to periodically send a command to the BlueCore to monitor the
 * state of the communication link. If the BlueCore communication is lost, a
 * CSR_TM_BLUECORE_TRANSPORT_DEACTIVATE_IND will be sent to the application
 * that activated the BlueCore transport, requesting it to deactivate the
 * transport. Leave undefined to disable this functionality. Only applicable
 * when CSR_BLUECORE_ONOFF is defined.
 */
#ifdef CSR_BLUECORE_ONOFF
#define CSR_BLUECORE_PING_INTERVAL 5000000
#endif

/*
 * The maximum time (in us) to wait for the response to a BlueCore command.
 * If no response is received within this time limit, the communication link
 * will be considered lost, and a CSR_TM_BLUECORE_TRANSPORT_DEACTIVATE_IND
 * will be sent to the application that activated the BlueCore transport,
 * requesting it to deactivate the transport. Only applicable when
 * CSR_BLUECORE_ONOFF is defined.
 */
#define CSR_BCCMD_CMD_TIMEOUT 2000000

/*--------------------------------------------------------------------------
 * Defines for Type-A
 *--------------------------------------------------------------------------*/
/*
 * Type-A deep sleep enable timeout in ms.
 * If 0, deep sleep is disabled.
 */
#define CSR_TRANSPORT_TYPE_A_ENABLE
#define CSR_TYPE_A_SLEEP_TIMEOUT 0

/*
 * The maximum bus speed to set during
 * normal operation.
 */
#define CSR_TYPE_A_BUSSPEED_AWAKE 12500000

/*
 * Type-A deep sleep wakeup delay in ms.
 */
#define CSR_TYPE_A_WAKEUP_TIMEOUT 125

/* Type-A initialisation delay in ms */
#define CSR_TYPE_A_ENABLE_DELAY 1000

/* Time to wait for chip wakeup in ms */
#define CSR_TYPE_A_WAKEUP_DELAY 1000

#define CSR_TYPE_A_RXBUF_POOLSIZE 32

#define CSR_TYPE_A_RXBUF_ELMSIZE (8 * 1024)

/* Maximum Type-A buffer size. */
#define CSR_TYPE_A_TXBUF_SIZE (64 * 1024 + 4 + 4)

/*--------------------------------------------------------------------------
 * Defines for BCSP
 *--------------------------------------------------------------------------*/
/* If CSR_ABCSP_TXCRC is #defined then the optional CRC field is appended to each
BCSP message transmitted, else the CRC is not appended. Do NOT define in this
file, must be a global definition, i.e. give it as argument to the compiler */

#define CSR_ABCSP_TXCRC

/* The CRC field is optional on BCSP messages.  If CSR_ABCSP_RXCRC is #defined
then the CRC fields found on all received BCSP messages are checked.  If
CSR_ABCSP_RXCRC is not #defined then no received CRC field is checked; this means
that packets with invalid CRCs may be accepted as good messages. Please also note
that if this is undefined it should also be undefined on the BC which will send
two unnecessary CRC bytes on every reliable BCSP packet to the host */

#define CSR_ABCSP_RXCRC

/*
 * BCSP retransmission timer and BCSP timer jitter.
 *
 * The BCSP retransmission timer is used to control for how
 * long the host waits for a response from the chip before
 * retransmitting a message.
 * If CSR_BCSP_RETRANSMISSION_TIMER is defined, its value is
 * used as the BCSP retransmission timer period value.
 * If not, the value is derived from the currently configured
 * (i.e. at runtime) UART speed.
 * By default, a 250ms timer is used.
 *
 * The retransmission timer is jittered to avoid scenarios
 * where the chip may keep missing the retransmission due to
 * unfortunate timing.  The jitter value controls the limits for
 * the random offset applied to the base retransmission timer value.
 * A value of 10ms is used by default which together with a 250ms
 * baseline of retransmission timer base gives a retransmission
 * timer in the interval [240ms; 260ms].
 *
 * CSR_BCSP_RETRANSMISSION_MINIMUM specifies the minimum period
 * used when jittering is enabled.
 */

/*
 */
/* #undef CSR_BCSP_AUTO_TIMER */

#ifndef CSR_BCSP_AUTO_TIMER
#define CSR_BCSP_RETRANSMISSION_TIMER 250000
#endif

#define CSR_BCSP_RETRANSMISSION_JITTER 10000

#define CSR_BCSP_RETRANSMISSION_MINIMUM 10000

#define CSR_BCSP_TSHY_TIMER 250000

#define CSR_BCSP_TCONF_TIMER 250000

/* The size of the BCSP transmit window.  This must be between 1 and 7.  This
is normally set to 4.  This is called "winsiz" in the BCSP protocol
specification.

This determines the number of BCSP messages that can be handled by the abcsp
library's transmit path at a time, so it affects the storage requirements for
ABCSP_TXMSG messages. */

#define CSR_ABCSP_TXWINSIZE 4

/*--------------------------------------------------------------------------
 * Defines for FastPipe
 *--------------------------------------------------------------------------*/

#define CSR_FP_CONTROLER_CREDIT_MAX 3132
#define CSR_FP_CONTROLLER_PACKET_SIZE_MAX 1019
#define CSR_FP_HOST_PACKET_SIZE_MAX 1
#define CSR_FP_PACEKTS_MAX 7

/*--------------------------------------------------------------------------
 * Defines for Chip Manager
 *--------------------------------------------------------------------------*/
/* Default number of microseconds between sending PING request */
#define CSR_BLUECORE_DEFAULT_PING_INTERVAL 5000000

#endif /* !CSR_USE_QCA_CHIP */


/*--------------------------------------------------------------------------
 * Defines for Csr Log
 *--------------------------------------------------------------------------*/
/* Defines the maximum string length that will be written to the log transport
 * in one call to the CSR_LOG_TEXT_XXX() functions found in csr_log_text.h.
 * NB: This limit does not apply to the CSR_LOG_TEXT() macro. */
#define CSR_LOG_TEXT_MAX_STRING_LEN 255

/* Defines an upper limit in bytes on the amount of primitive data to write in
 * a put/get/pop/save message entry. NB: This limit only applies if the log
 * level define CSR_LOG_LEVEL_TASK_PRIM_APPLY_LIMIT is set for a task.
 *
 * WARNING: Using this will seriusly affect the readability of the wireshark
 * logs, but it might help as a measure to reduce the amount of log info
 * generated on a platform */
#define CSR_LOG_PRIM_SIZE_UPPER_LIMIT 64

/* Specify the output template format for the cleartext logger
 * (see csr_log_cleartext.h) for a description of all possible templates */
#define CSR_LOG_CLEARTEXT_FORMAT CSR_LOG_CLEARTEXT_TEMPLATE_YEAR "/" CSR_LOG_CLEARTEXT_TEMPLATE_MONTH "/" CSR_LOG_CLEARTEXT_TEMPLATE_DAY CSR_LOG_CLEARTEXT_TEMPLATE_HOUR ":" CSR_LOG_CLEARTEXT_TEMPLATE_MIN ":" CSR_LOG_CLEARTEXT_TEMPLATE_TIME_SEC ":" CSR_LOG_CLEARTEXT_TEMPLATE_TIME_MSEC " " CSR_LOG_CLEARTEXT_TEMPLATE_TASK_NAME " " CSR_LOG_CLEARTEXT_TEMPLATE_SUBORIGIN_NAME " " CSR_LOG_CLEARTEXT_TEMPLATE_LOG_LEVEL_NAME ": " CSR_LOG_CLEARTEXT_TEMPLATE_STRING CSR_LOG_CLEARTEXT_TEMPLATE_BUFFER

/*--------------------------------------------------------------------------
 * Defines for the IP stack
 *--------------------------------------------------------------------------*/
/* By default the IP stack assumes that the host architecture is little endian
 * if this is not the case the define below should be defined to force the IP
 * stack to use big endian */
/* #undef CSR_IP_USE_BIG_ENDIAN */

/*--------------------------------------------------------------------------
 * Defines for the CSR_DATA_STORE
 *--------------------------------------------------------------------------*/
/* This define specifies the directory where the CSR_DATA_STORE task places
 * its database files current default is in the directory "data_store" relative
 * to where the executable is started.
 * NB: It is important that this path ends with a trailing '/' */
#define CSR_DATA_STORE_ROOT_DIR "./data_store/"

#define CSR_DATA_STORE_DEFAULT_ENTRY_NAME "ds_info.cdi"

/*--------------------------------------------------------------------------
 * Defines for DSPM
 *--------------------------------------------------------------------------*/
/* If support for downloading capabilities in DSPM is not required, this
   definition can be removed to reduce the code size. */
#define CSR_DSPM_SUPPORT_CAPABILITY_DOWNLOAD

/* #undef CSR_DSPM_SUPPORT_ACCMD */

#ifdef CSR_DSPM_SUPPORT_CAPABILITY_DOWNLOAD
#ifdef CSR_USE_QCA_CHIP
#define CSR_DSPM_KCS_DOWNLOAD
#else
#define CSR_DSPM_CAP_DOWNLOAD
#endif /* CSR_USE_QCA_CHIP */
#endif /* CSR_DSPM_SUPPORT_CAPABILITY_DOWNLOAD */

/*--------------------------------------------------------------------------
 * Defines for pclin DHCP client configuration
 *--------------------------------------------------------------------------*/
/* Full path to dhcpcd (used on Android) and dhclient. */
#define DHCPCD_PATH "/system/bin/dhcpcd"

#define DHCLIENT_PATH "/sbin/dhclient"

/*--------------------------------------------------------------------------
 * Defines for DHCP_SERVER
 *--------------------------------------------------------------------------*/
/* #undef CSR_DHCP_SERVER_USE_IFCONFIG_ARP */

/*-------------------------------------------------------------------------
 * Defines for BTM
 *--------------------------------------------------------------------------*/
/* #undef CSR_BTM_TASK */

/*-------------------------------------------------------------------------
 * Defines for H4, H4DS and H4IBS
 *--------------------------------------------------------------------------*/
#define H4    1
#define H4IBS 2
#define H4DS  3

#if (H4DS == H4)
#define CSR_H4_TRANSPORT_ENABLE
#elif (H4DS == H4IBS)
#define CSR_H4_TRANSPORT_ENABLE
#ifdef CSR_USE_QCA_CHIP
#define CSR_H4IBS_TRANSPORT_ENABLE
#endif
#elif (H4DS == H4DS)
#define CSR_H4DS_TRANSPORT_ENABLE
#endif /* CSR_H4_TRANSPORT */

#ifdef CSR_H4_TRANSPORT_ENABLE
#define CSR_H4_PKT_READ_TIMEOUT 4
/* #undef CSR_H4_ALLOW_STRAY_BYTES */
#endif /* CSR_H4_TRANSPORT_ENABLE */

#ifndef CSR_FRW_INSTALL_TRANS_RECOVERY
#define CSR_FRW_TRANS_PROBE_TIME 60000
#endif

#ifndef CSR_FRW_INSTALL_TRANS_RECOVERY
#define CSR_FRW_TRANS_PANIC_ON_NO_RSP_TIME 2000
#endif

#ifdef CSR_H4IBS_TRANSPORT_ENABLE
#define CSR_H4_EXTENSION
#define CSR_H4IBS_TX_IDLE_TIMEOUT 40
#define CSR_H4IBS_TX_WAKE_RETRY_TIMEOUT 10
/* #undef CSR_H4_IBS_WAKE_RETRY_COUNT_MAX */
#endif /* CSR_H4IBS_TRANSPORT_ENABLE */

#endif /* _CSR_FRW_CONFIG_DEFAULT_H */
#include "csr_frw_config_private.h" 
