/*!

Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

\file   vsdm_prim.h

\brief  Vendor specific Device Manager

        The functionalities exposed in this file are Qualcomm proprietary.

        Vendor Specific Device Manager provides application interface to 
        perform vendor specific functionalities.
*/
#ifndef _VSDM_PRIM_H_
#define _VSDM_PRIM_H_

#include "hci.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
    \name Response/result error and status codes

    \{
*/
/*! Operation was successful */
#define VSDM_RESULT_SUCCESS                      0x0000
#define VSDM_RESULT_INVALID_PARAM                0x0001
#define VSDM_RESULT_INPROGRESS                   0x0002
#define VSDM_RESULT_FAIL                         0x0003

#define VSDM_MAX_NO_OF_COMPIDS     (4)
#define VSDM_QLM_SUPP_FET_SIZE     (16)
#define VSDM_QLL_SUPP_FET_SIZE     (8)

/*! \name Bluestack primitive segmentation and numbering

    \brief VSDM primitives occupy the number space from
    VSDM_PRIM_BASE to (VSDM_PRIM_BASE | 0x00FF).

    \{ */
#define VSDM_PRIM_DOWN           (VSDM_PRIM_BASE)
#define VSDM_PRIM_UP             (VSDM_PRIM_BASE | 0x0080)
#define VSDM_PRIM_MAX            (VSDM_PRIM_BASE | 0x00FF)

typedef enum vsdm_prim_tag
{
    /* downstream primitives */
    ENUM_VSDM_REGISTER_REQ = VSDM_PRIM_DOWN,
    ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ,
    ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ,
    ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ,
    ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ,
    ENUM_VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ,
    ENUM_VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ,
    ENUM_VSDM_SET_QHS_HOST_MODE_REQ,
    ENUM_VSDM_SET_WBM_FEATURES_REQ,
    ENUM_VSDM_CONVERT_RPA_TO_IA_REQ,
    ENUM_VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ,
    ENUM_VSDM_SET_STREAMING_MODE_REQ,
    ENUM_VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ,
    ENUM_VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_REQ,
    ENUM_VSDM_QLE_SET_HOST_FEATURES_REQ,
    ENUM_VSDM_QLE_SET_CIG_QHS_MAP_REQ,
    ENUM_VSDM_SET_EVENT_MASK_REQ,

    /* upstream primitives */
    ENUM_VSDM_REGISTER_CFM = VSDM_PRIM_UP,
    ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM,
    ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM,
    ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM,
    ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM,
    ENUM_VSDM_QLM_CONNECTION_COMPLETE_IND,
    ENUM_VSDM_QCM_PHY_CHANGE_IND,
    ENUM_VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM,
    ENUM_VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM,
    ENUM_VSDM_SET_QHS_HOST_MODE_CFM,
    ENUM_VSDM_SET_WBM_FEATURES_CFM,
    ENUM_VSDM_CONVERT_RPA_TO_IA_CFM,
    ENUM_VSDM_INCOMING_PAGE_IND,
    ENUM_VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM,
    ENUM_VSDM_SET_STREAMING_MODE_CFM,
    ENUM_VSDM_QLL_CONNECTION_COMPLETE_IND,
    ENUM_VSDM_READ_REMOTE_QLL_SUPP_FEATURES_CFM,
    ENUM_VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_CFM,
    ENUM_VSDM_QLE_SET_HOST_FEATURES_CFM,
    ENUM_VSDM_QLE_SET_CIG_QHS_MAP_CFM,
    ENUM_VSDM_CIG_QHS_RATE_CHANGED_IND,
    ENUM_VSDM_LE_PA_DATA_TRANSMITTED_IND,
    ENUM_VSDM_SET_EVENT_MASK_CFM,
    ENUM_VSDM_BT_QUALITY_MONITORING_REPORT_IND,
    ENUM_VSDM_BT_XSCO_VOICE_CHOPPY_REPORT_IND,
    ENUM_VSDM_BT_LE_AUDIO_CHOPPY_REPORT_IND
} VSDM_PRIM_T;

/* downstream primitives */
#define VSDM_REGISTER_REQ                           ((vsdm_prim_t)ENUM_VSDM_REGISTER_REQ)
#define VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ       ((vsdm_prim_t)ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ)
#define VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ      ((vsdm_prim_t)ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ)
#define VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ        ((vsdm_prim_t)ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ)
#define VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ         ((vsdm_prim_t)ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ)
#define VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ    ((vsdm_prim_t)ENUM_VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ)
#define VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ     ((vsdm_prim_t)ENUM_VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ)
#define VSDM_SET_QHS_HOST_MODE_REQ                  ((vsdm_prim_t)ENUM_VSDM_SET_QHS_HOST_MODE_REQ)
#define VSDM_SET_WBM_FEATURES_REQ                   ((vsdm_prim_t)ENUM_VSDM_SET_WBM_FEATURES_REQ)
#define VSDM_CONVERT_RPA_TO_IA_REQ                  ((vsdm_prim_t)ENUM_VSDM_CONVERT_RPA_TO_IA_REQ)
#define VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ   ((vsdm_prim_t)ENUM_VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ)
#define VSDM_SET_STREAMING_MODE_REQ                 ((vsdm_prim_t)ENUM_VSDM_SET_STREAMING_MODE_REQ)
#define VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ      ((vsdm_prim_t)ENUM_VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ)
#define VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_REQ        ((vsdm_prim_t)ENUM_VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_REQ)
#define VSDM_QLE_SET_HOST_FEATURES_REQ              ((vsdm_prim_t)ENUM_VSDM_QLE_SET_HOST_FEATURES_REQ)
#define VSDM_QLE_SET_CIG_QHS_MAP_REQ                ((vsdm_prim_t)ENUM_VSDM_QLE_SET_CIG_QHS_MAP_REQ)
#define VSDM_SET_EVENT_MASK_REQ                     ((vsdm_prim_t)ENUM_VSDM_SET_EVENT_MASK_REQ)

/* upstream primitives */
#define VSDM_REGISTER_CFM                           ((vsdm_prim_t)ENUM_VSDM_REGISTER_CFM)
#define VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM       ((vsdm_prim_t)ENUM_VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM)
#define VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM      ((vsdm_prim_t)ENUM_VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM)
#define VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM        ((vsdm_prim_t)ENUM_VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM)
#define VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM         ((vsdm_prim_t)ENUM_VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM)
#define VSDM_QLM_CONNECTION_COMPLETE_IND            ((vsdm_prim_t)ENUM_VSDM_QLM_CONNECTION_COMPLETE_IND)
#define VSDM_QCM_PHY_CHANGE_IND                     ((vsdm_prim_t)ENUM_VSDM_QCM_PHY_CHANGE_IND)
#define VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM    ((vsdm_prim_t)ENUM_VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM)
#define VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM     ((vsdm_prim_t)ENUM_VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM)
#define VSDM_SET_QHS_HOST_MODE_CFM                  ((vsdm_prim_t)ENUM_VSDM_SET_QHS_HOST_MODE_CFM)
#define VSDM_SET_WBM_FEATURES_CFM                   ((vsdm_prim_t)ENUM_VSDM_SET_WBM_FEATURES_CFM)
#define VSDM_CONVERT_RPA_TO_IA_CFM                  ((vsdm_prim_t)ENUM_VSDM_CONVERT_RPA_TO_IA_CFM)
#define VSDM_INCOMING_PAGE_IND                      ((vsdm_prim_t)ENUM_VSDM_INCOMING_PAGE_IND)
#define VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM   ((vsdm_prim_t)ENUM_VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM)
#define VSDM_SET_STREAMING_MODE_CFM                 ((vsdm_prim_t)ENUM_VSDM_SET_STREAMING_MODE_CFM)
#define VSDM_QLL_CONNECTION_COMPLETE_IND            ((vsdm_prim_t)ENUM_VSDM_QLL_CONNECTION_COMPLETE_IND)
#define VSDM_READ_REMOTE_QLL_SUPP_FEATURES_CFM      ((vsdm_prim_t)ENUM_VSDM_READ_REMOTE_QLL_SUPP_FEATURES_CFM)
#define VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_CFM        ((vsdm_prim_t)ENUM_VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_CFM)
#define VSDM_QLE_SET_HOST_FEATURES_CFM              ((vsdm_prim_t)ENUM_VSDM_QLE_SET_HOST_FEATURES_CFM)
#define VSDM_QLE_SET_CIG_QHS_MAP_CFM                ((vsdm_prim_t)ENUM_VSDM_QLE_SET_CIG_QHS_MAP_CFM)
#define VSDM_CIG_QHS_RATE_CHANGED_IND               ((vsdm_prim_t)ENUM_VSDM_CIG_QHS_RATE_CHANGED_IND)
#define VSDM_LE_PA_DATA_TRANSMITTED_IND             ((vsdm_prim_t)ENUM_VSDM_LE_PA_DATA_TRANSMITTED_IND)
#define VSDM_SET_EVENT_MASK_CFM                     ((vsdm_prim_t)ENUM_VSDM_SET_EVENT_MASK_CFM)
#define VSDM_BT_QUALITY_MONITORING_REPORT_IND       ((vsdm_prim_t)ENUM_VSDM_BT_QUALITY_MONITORING_REPORT_IND)
#define VSDM_BT_XSCO_VOICE_CHOPPY_REPORT_IND        ((vsdm_prim_t)ENUM_VSDM_BT_XSCO_VOICE_CHOPPY_REPORT_IND)
#define VSDM_BT_LE_AUDIO_CHOPPY_REPORT_IND          ((vsdm_prim_t)ENUM_VSDM_BT_LE_AUDIO_CHOPPY_REPORT_IND)
/*! \} */

/*! \brief Types for VSDM */
typedef uint16_t                vsdm_prim_t;
typedef uint16_t                vsdm_result_t;

/**
 * Type definition used to specify phy type.
 */
typedef uint8_t vsdm_phy_type_t;

#define PHY_TYPE_BREDR       ((vsdm_phy_type_t)0x00)
#define PHY_TYPE_QHS         ((vsdm_phy_type_t)0x01)

/**
 * Type definition used to specify source type.
 */
typedef uint8_t vsdm_source_type_t;

#define SOURCE_TYPE_LOCAL   ((vsdm_source_type_t)0x00)
#define SOURCE_TYPE_REMOTE  ((vsdm_source_type_t)0x01)

/**
 * Type definition used to specify secure connections(SC) host support CoD 
 * bit value.type.
 */
typedef uint8_t vsdm_sc_cod_type_t;

#define SC_HOST_SUPP_DISABLE_COD      ((vsdm_sc_cod_type_t)0x00)
#define SC_HOST_SUPP_ENABLE_COD       ((vsdm_sc_cod_type_t)0x01)

/*! \brief Register the VSDM subsystem request

    Before any VSDM operations can be performed the VSDM subsystem shall
    be registered and a destination phandle for upstream application
    primitives shall also be registered.
*/
typedef struct
{
    vsdm_prim_t         type;           /*!< Always VSDM_REGISTER_REQ */
    phandle_t           phandle;        /*!< Destination phandle */
} VSDM_REGISTER_REQ_T;

typedef struct
{
    vsdm_prim_t         type;           /*!< Always VSDM_REGISTER_CFM */
    phandle_t           phandle;        /*!< Destination phandle */
    vsdm_result_t       result;         /*!< Result code - uses VSDM_RESULT range */
} VSDM_REGISTER_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Read local supported QLM features command
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;               /* Always VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ */
    phandle_t           phandle;            /* destination phandle */
} VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Read local supported QLM features complete
 *     
 *      QLMP feature bit mask is 16 octets and is represented as follows :
 *      qlmp_supp_features[0], Bit 0 -> Split ACL (LSB)
 *      qlmp_supp_features[0], Bit 1 -> TWM eSCO
 *      qlmp_supp_features[0], Bit 2 -> eSCO DTX
 *      qlmp_supp_features[0], Bit 3 -> Reserved
 *      qlmp_supp_features[0], Bit 4 -> QHS Classic Mode including QHS-P2 packet support
 *      qlmp_supp_features[0], Bit 5 -> QHS-P3 packet support
 *      qlmp_supp_features[0], Bit 6 -> QHS-P4 packet support
 *      qlmp_supp_features[0], Bit 7 -> QHS-P5 packet support
 *      qlmp_supp_features[1], Bit 0 -> QHS-P6 packet support
 *      qlmp_supp_features[1], Bit 1 -> Real Time Soft Combining
 *      qlmp_supp_features[1], Bit 2 -> QHS Classic Mode eSCO packets without MIC
 *      qlmp_supp_features[1], Bit 3 -> QHS Classic Mode Separate ACL and eSCO Nonces
 *      qlmp_supp_features[1], Bit 4 -> ACL mirroring
 *      qlmp_supp_features[1], Bit 5 -> eSCO mirroring
 *      qlmp_supp_features[1], Bit 6 -> CSB Burst Mode
 *      qlmp_supp_features[1], Bit 7 -> Non-DM1 Encapsulated Payloads
 *      qlmp_supp_features[2], Bit 0 -> ACL Handover
 *      qlmp_supp_features[2], Bit 1 -> Reserved
 *      qlmp_supp_features[2], Bit 2 -> eSCO Handover
 *      qlmp_supp_features[2], Bit 3 -> TWM Mirroring Fast Handover
 *      qlmp_supp_features[2], Bit 4 -> 1.5 Slot QHS Packets
 *      qlmp_supp_features[2], Bit 5 -> Broadcast Relay
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;                                       /*!< Always VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM */
    phandle_t           phandle;                                    /*!< destination phandle */
    hci_return_t        status;                                     /*!< status */
    uint8_t             qlmp_supp_features[VSDM_QLM_SUPP_FET_SIZE]; /*!< QLMP supported features */
} VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Read remote supported QLM features command
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;           /* Always VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ */
    phandle_t               phandle;        /* destination phandle */
    hci_connection_handle_t handle;         /* connection handle */
    BD_ADDR_T               bd_addr;        /* Bluetooth device address */
} VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      To set or unset event mask for QLM or QLL. Setting a bit to 1 enables
 *      Controller to generate the corresponding event by the HCI for the Host.
 *
 * Note:
 *      This function can be called multiple times and will overwrite the 
 *      previously set event mask.
 *
 * evt_mask_type
 *      VSDM_EVT_TYPE_QLM - If evt_mask[] are QLM event mask.
 *      VSDM_EVT_TYPE_QLL - If evt_mask[] are QLL event mask.
 * 
 * evt_mask
 *      evt_mask[] is 8 octets bit mask.
 *      Those bit position which are mentioned as "Reserved" below cannot be 
 *      set or unset via this API.
 *
 *      evt_mask[0], Bit 0 -> Reserved
 *      evt_mask[0], Bit 1 -> Reserved
 *      evt_mask[0], Bit 2 -> Reserved
 *      evt_mask[0], Bit 3 -> Reserved
 *      evt_mask[0], Bit 4 -> Reserved
 *      evt_mask[0], Bit 5 -> Reserved
 *      evt_mask[0], Bit 6 -> Reserved
 *      evt_mask[0], Bit 7 -> Reserved
 *      evt_mask[1], Bit 0 -> Reserved
 *      evt_mask[1], Bit 1 -> Reserved
 *      evt_mask[1], Bit 2 -> Reserved
 *      evt_mask[1], Bit 3 -> Enables VSDM_LE_PA_DATA_TRANSMITTED_IND event.
                              Event when new PA data is transmitted for the first time.
 *                            evt_mask_type shall be set to VSDM_EVT_TYPE_QLL.
 *      evt_mask[1], Bit 4 -> Reserved
 *      evt_mask[1], Bit 5 -> Reserved
 ...
 ...
 *      evt_mask[7], Bit 7 -> Reserved
 *----------------------------------------------------------------------------*/
/* Values of evt_mask_type supported */
#define VSDM_EVT_TYPE_QLM           1 /* QLM event mask. Reserved not supported */
#define VSDM_EVT_TYPE_QLL           2 /* QLL event mask */

/* Size of the evt_mask[] */
#define VSDM_SIZE_OF_EVT_MASK       8

typedef struct
{
    vsdm_prim_t     type;           /* Always VSDM_SET_EVENT_MASK_REQ */
    phandle_t       phandle;        /* Destination phandle */
    uint8_t         evt_mask_type;  /* Refer VSDM_EVT_TYPE_XXX above */
    uint8_t         evt_mask[VSDM_SIZE_OF_EVT_MASK]; /* Event bit mask. Refer evt_mask above for details */
} VSDM_SET_EVENT_MASK_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_SET_EVENT_MASK_REQ 
 * command given by the application.
 *
 * Status value other than zero would imply that the operation has failed.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;           /* Always VSDM_SET_EVENT_MASK_CFM */
    phandle_t       phandle;        /* Destination phandle */
    vsdm_result_t   result;         /* Result code - uses VSDM_RESULT range */
} VSDM_SET_EVENT_MASK_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Notification of remote supported QLM features
 *     
 *      QLMP feature bit mask is 16 octets and is represented as follows :
 *      qlmp_supp_features[0], Bit 0 -> Split ACL (LSB)
 *      qlmp_supp_features[0], Bit 1 -> TWM eSCO
 *      qlmp_supp_features[0], Bit 2 -> eSCO DTX
 *      qlmp_supp_features[0], Bit 3 -> Reserved
 *      qlmp_supp_features[0], Bit 4 -> QHS Classic Mode including QHS-P2 packet support
 *      qlmp_supp_features[0], Bit 5 -> QHS-P3 packet support
 *      qlmp_supp_features[0], Bit 6 -> QHS-P4 packet support
 *      qlmp_supp_features[0], Bit 7 -> QHS-P5 packet support
 *      qlmp_supp_features[1], Bit 0 -> QHS-P6 packet support
 *      qlmp_supp_features[1], Bit 1 -> Real Time Soft Combining
 *      qlmp_supp_features[1], Bit 2 -> QHS Classic Mode eSCO packets without MIC
 *      qlmp_supp_features[1], Bit 3 -> QHS Classic Mode Separate ACL and eSCO Nonces
 *      qlmp_supp_features[1], Bit 4 -> ACL mirroring
 *      qlmp_supp_features[1], Bit 5 -> eSCO mirroring
 *      qlmp_supp_features[1], Bit 6 -> CSB Burst Mode
 *      qlmp_supp_features[1], Bit 7 -> Non-DM1 Encapsulated Payloads
 *      qlmp_supp_features[2], Bit 0 -> ACL Handover
 *      qlmp_supp_features[2], Bit 1 -> Reserved
 *      qlmp_supp_features[2], Bit 2 -> eSCO Handover
 *      qlmp_supp_features[2], Bit 3 -> TWM Mirroring Fast Handover
 *      qlmp_supp_features[2], Bit 4 -> 1.5 Slot QHS Packets
 *      qlmp_supp_features[2], Bit 5 -> Broadcast Relay
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;                                        /*!< Always VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM */
    phandle_t           phandle;                                     /*!< destination phandle */
    hci_return_t        status;                                      /*!< Success or failure */
    BD_ADDR_T           bd_addr;                                     /*!< Bluetooth device address */
    uint8_t             qlmp_supp_features[VSDM_QLM_SUPP_FET_SIZE];  /*!< QLMP supported features */
} VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Read remote supported QLL features command
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;           /* Always VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ */
    phandle_t               phandle;        /* destination phandle */
    hci_connection_handle_t handle;         /* connection handle */
    TP_BD_ADDR_T            tp_addrt;       /* Transport Bluetooth device address */
} VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Notification of remote supported QLL features
 *     
 *      QLL feature bit mask is 8 octets and is represented as follows :
 *      qll_supp_features[0], Bit 0 -> QHS-P2 TX
 *      qll_supp_features[0], Bit 1 -> QHS-P3 TX
 *      qll_supp_features[0], Bit 2 -> QHS-P4 TX
 *      qll_supp_features[0], Bit 3 -> QHS-P5 TX
 *      qll_supp_features[0], Bit 4 -> QHS-P6 TX
 *      qll_supp_features[0], Bit 5 -> QHS-P2 RX
 *      qll_supp_features[0], Bit 6 -> QHS-P3 RX
 *      qll_supp_features[0], Bit 7 -> QHS-P4 RX
 *      qll_supp_features[1], Bit 0 -> QHS-P5 RX
 *      qll_supp_features[1], Bit 1 -> QHS-P6 RX
 *      qll_supp_features[1], Bit 2 -> QHS-F2 TX
 *      qll_supp_features[1], Bit 3 -> QHS-F3 TX
 *      qll_supp_features[1], Bit 4 -> QHS-F4 TX
 *      qll_supp_features[1], Bit 5 -> QHS-F5 TX
 *      qll_supp_features[1], Bit 6 -> QHS-F6 TX
 *      qll_supp_features[1], Bit 7 -> QHS-F2 RX
 *      qll_supp_features[2], Bit 0 -> QHS-F3 RX
 *      qll_supp_features[2], Bit 1 -> QHS-F4 RX
 *      qll_supp_features[2], Bit 2 -> QHS-F5 RX
 *      qll_supp_features[2], Bit 3 -> QHS-F6 RX
 *      qll_supp_features[2], Bit 4 -> Real Time Soft Combining
 *      qll_supp_features[2], Bit 5 -> Reserved
 *      qll_supp_features[2], Bit 6 -> Extended ISO Channels
 *      qll_supp_features[2], Bit 7 -> Reserved
 *      qll_supp_features[3], Bit 0 -> LE Empty Data Packet HardAlign
 *      qll_supp_features[3], Bit 1 -> QLL Channel Classification
 *      qll_supp_features[3], Bit 2 -> FT Change
 *      qll_supp_features[3], Bit 3 -> BN Variation by QHS rate
 *      qll_supp_features[3], Bit 4 -> Fast Exit subrate
 *      qll_supp_features[3], Bit 5 -> QSS Host Support Bit 0
 *      qll_supp_features[3], Bit 6 -> QSS Host Support Bit 1
 *      Rest are reserved
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;                                        /*!< Always VSDM_READ_REMOTE_QLL_SUPP_FEATURES_CFM */
    phandle_t           phandle;                                     /*!< destination phandle */
    hci_return_t        status;                                      /*!< Success or failure */
    TP_BD_ADDR_T        tp_addrt;                                    /*!< Transport Bluetooth device address */
    uint8_t             qll_supp_features[VSDM_QLL_SUPP_FET_SIZE];   /*!< QLL supported features */
} VSDM_READ_REMOTE_QLL_SUPP_FEATURES_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of QLMP Connection Establishment.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;      /* Always VSDM_QLM_CONNECTION_COMPLETE_IND */
    phandle_t               phandle;   /* destination phandle */
    hci_connection_handle_t handle;    /* QLM Connection handle */
    BD_ADDR_T               bd_addr;   /* Bluetooth device address */
    hci_return_t            status;    /* HCI_SUCCESS if QLM connection completed successfully, otherwise error */
} VSDM_QLM_CONNECTION_COMPLETE_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of QLL Connection Establishment.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;      /* Always VSDM_QLL_CONNECTION_COMPLETE_IND */
    phandle_t               phandle;   /* destination phandle */
    hci_connection_handle_t handle;    /* QLL Connection handle */
    TP_BD_ADDR_T            tp_addrt;  /* Transport Bluetooth device address */
    hci_return_t            status;    /* HCI_SUCCESS if QLL connection completed successfully, otherwise error */
} VSDM_QLL_CONNECTION_COMPLETE_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of QCM PHY change to indicate the controller has changed the
 *      the PHY used on a normal ACL connection or a mirrored ACL connection.
 *
 *----------------------------------------------------------------------------*/
 typedef struct
{
    vsdm_prim_t             type;      /*!< Always VSDM_QCM_PHY_CHANGE_IND */
    phandle_t               phandle;   /*!< destination phandle */
    hci_connection_handle_t handle;    /*!< QLM Connection handle */
    BD_ADDR_T               bd_addr;   /*!< Bluetooth device address */
    vsdm_phy_type_t         phy;       /*!< Type of phy, either BR/EDR or QHS */
    vsdm_source_type_t      source;    /*!< Type of source, either local or remote */
    hci_return_t            status;    /*!< 0 if PHY changed successfully, otherwise error */
} VSDM_QCM_PHY_CHANGE_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of page indication from third remote device when
 *      already 2 devices are connected.
 *
 *----------------------------------------------------------------------------*/
 typedef struct
{
    vsdm_prim_t             type;      /*!< Always VSDM_INCOMING_PAGE_IND */
    phandle_t               phandle;   /*!< destination phandle */
    BD_ADDR_T               bd_addr;   /*!< Bluetooth device address */
    uint24_t                class_of_device;       /*!< Class of device */
} VSDM_INCOMING_PAGE_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of CIG QHS rate changed indicates that the QHS rate on the 
 *      CIG identified by the cig_id has changed.
 *
 *      QHS rate bit mask is 1 octet and the bits are represented as follows:
 *      Bit-0 --> QHS2
 *      Bit-1 --> QHS3
 *      Bit-2 --> QHS4
 *      Bit-3 --> QHS5
 *      Bit-4 --> QHS6
 *      Bit-5  to Bit-7 are reserved
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;      /*!< Always VSDM_CIG_QHS_RATE_CHANGED_IND */
    phandle_t               phandle;   /*!< destination phandle */
    uint8_t                 cig_id;    /*!< CIG identifier */
    uint8_t                 qhs_rate;  /*!< QHS-rate */
} VSDM_CIG_QHS_RATE_CHANGED_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *      Indication of Periodic advertisement data transmitted indicates that 
 *      the Controller has transmitted the periodic advertising data first time
 *      post data set by DM_HCI_ULP_PERIODIC_ADV_SET_DATA_REQ for the adv_handle.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t             type;      /*!< Always VSDM_LE_PA_DATA_TRANSMITTED_IND */
    phandle_t               phandle;   /*!< destination phandle */
    uint8_t                 adv_handle;/*!< advertisement handle */
} VSDM_LE_PA_DATA_TRANSMITTED_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 * This command writes an array of compID, min_lmpVersion, and min_lmpSubVersion
 * parameters to be used by the Controller. After the LMP version sequence the
 * controller determines whether the compID, lmpVersion, and lmpSubVersion are
 * valid compared to the array written by this command. If the compID matches
 * and the lmpVersion and lmpSubVersion of the remote device is greater than
 * the values stored then the controller forces the SC_host_support LMP feature
 * bit to 'Enabled' in the LMP feature sequence. By default the controller can 
 * be configured to indicate that host does not support SC. Based on the parameters 
 * provided here, if the remote device qualifies then the controller would overide 
 * the SC bit to indicated host SC support to the remote device. This can as well 
 * be overridden for an individual device using DM_WRITE_SC_HOST_SUPPORT_OVERRIDE_REQ 
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t   type;                                      /* Always VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ */
    phandle_t     phandle;                                   /* destination phandle */
    uint8_t       num_compIDs;                               /* Number of compIDs */
    uint16_t      compID[VSDM_MAX_NO_OF_COMPIDS];            /* compIDs to apply host mode override values */
    uint8_t       min_lmpVersion[VSDM_MAX_NO_OF_COMPIDS];    /*!< min_lmpVersion associated with compIDs */
    uint16_t      min_lmpSubVersion[VSDM_MAX_NO_OF_COMPIDS]; /*!< min_lmpSubVersion associated with compIDs */
} VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;       /* Always VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM */
    phandle_t       phandle;    /* destination phandle */
    hci_return_t    status;     /* status of write secure connections override */
} VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command reads the array of compID, lmpVersion, and lmpSubVersion parameters
 * used by the controller to determine whether to override the SC_host_support LMP
 * feature bit.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;       /* VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ */
    phandle_t       phandle;    /* destination phandle */
} VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t   type;                                      /* Always VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM */
    phandle_t     phandle;                                   /* destination phandle */
    hci_return_t  status;                                    /* status of read secure connections QC override */
    uint8_t       num_compIDs;                               /* Number of compIDs */
    uint16_t      compID[VSDM_MAX_NO_OF_COMPIDS];            /* compIDs to apply host mode override values */
    uint8_t       min_lmpVersion[VSDM_MAX_NO_OF_COMPIDS];    /*!< min_lmpVersion associated with compIDs */
    uint16_t      min_lmpSubVersion[VSDM_MAX_NO_OF_COMPIDS]; /*!< min_lmpSubVersion associated with compIDs */
} VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 * 
 * This command writes the bit number of the Class of Device (CoD) and its 
 * corresponding value to be used by the Controller to determine whether to 
 * override the Secure_Connections_Host_Support LMP Feature bit in the 
 * LMP_feature_req or LMP_feature_res PDU.
 *
 * When this command is received by the Controller, the previous parameter 
 * values are overwritten by the parameter values of this command.
 * After the Controller becomes aware of the CoD of the remote device, it shall 
 * then based on the bit being enabled or disabled the Controller shall force the
 * Secure_Connections_Host_Support LMP feature bit to Enabled in the subsequent 
 * LMP feature sequence unless the HCI_Write_Secure_Connections_Host_Support
 * _Override command has forced the bit to Disabled for that BD_ADDR. 
 *
 * This command is only relevant on the Peripheral since the Host of the 
 * Peripheral only becomes aware of the CoD value of the Central in the 
 * HCI_Connection_Request event which is too late to override the 
 * Secure_Connections_Host_Support LMP feature bit using the 
 * HCI_Write_Secure_Connections_Host_Support_Override command (since the LMP 
 * feature exchange would have taken place already by that time). 
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;          /* Always VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ */
    phandle_t           phandle;       /* destination phandle */
    uint8_t             bit_number;    /* Bit position in the Class of Device(0 to 23) */
    vsdm_sc_cod_type_t  enable;        /* Enable or disable SC based on Class of Device */
}VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;          /* Always VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM */
    phandle_t           phandle;       /* destination phandle */
    hci_return_t        status;        /* status of write secure connections CoD override */
} VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 * 
 * This command reads the Bit_Number and Bit_Value parameters in the CoD used
 * by the Controller to determine whether to override the Secure_Connections_Host
 * _Support LMP Feature bit in the LMP_feature_req or LMP_feature_res PDU.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;          /* Always VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ */
    phandle_t           phandle;       /* destination phandle */
}VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t         type;          /* Always VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM */
    phandle_t           phandle;       /* destination phandle */
    hci_return_t        status;        /* status of read secure connections CoD override */
    uint8_t             bit_number;    /* Bit position in the Class of Device */
    vsdm_sc_cod_type_t  value;         /* Value of the bit_number */
} VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM_T;


/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command is used by the Host to tell the Controller which QHS mode
 * to use on the indicated Transport.
 *
 *----------------------------------------------------------------------------*/
/**
 * Type definitions to specify the QHS host mode host is interested on
 * indicated transport.
 */
typedef uint8_t qhs_transport_t;

#define TRANSPORT_TYPE_BREDR        ((qhs_transport_t) 0x00)
#define TRANSPORT_TYPE_LE           ((qhs_transport_t) 0x01)
#define TRANSPORT_TYPE_LE_ISOC      ((qhs_transport_t) 0x02)

typedef uint8_t qhs_host_mode_t;

/* For All transports */
#define QHS_HOST_MODE_DISABLED      ((qhs_host_mode_t) 0x00)

/* Only for TRANSPORT_TYPE_BREDR */
#define QHS_HOST_MODE_BREDR_ENABLE  ((qhs_host_mode_t) 0x01)

/* Only for TRANSPORT_TYPE_LE */
#define QHS_HOST_MODE_LE2M          ((qhs_host_mode_t) 0x01)
#define QHS_HOST_MODE_LE1M_OR_LE2M  ((qhs_host_mode_t) 0x02)

/* For All transports*/
#define QHS_HOST_MODE_HOST_AWARE    ((qhs_host_mode_t) 0x03)

typedef struct
{
    vsdm_prim_t         type;           /* Always VSDM_SET_QHS_HOST_MODE_REQ */
    phandle_t           phandle;        /* Destination phandle */
    qhs_transport_t     transport;       /* Transport type */
    qhs_host_mode_t     qhs_host_mode;  /* QHS mode type host want to set */
} VSDM_SET_QHS_HOST_MODE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_SET_QHS_HOST_MODE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;       /* Always VSDM_SET_QHS_HOST_MODE_CFM */
    phandle_t       phandle;    /* Destination phandle */
    hci_return_t    status;     /* Status of set QHS host mode request */
} VSDM_SET_QHS_HOST_MODE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command is used to enable or disable the support of Weak Bitmask (WBM)
 * propagation feature generated by RTSC/QBM on a connection handle basis.
 * The Connection_Handle may be a BR/EDR connection handle, an eSCO
 * connection handle, or an LE Iso Channels connection handle.
 *
 *----------------------------------------------------------------------------*/

/*
 * Type definitions to specify enable or disable of Weak Bitmask (WBM).
 */

typedef uint16_t enable_bit_mask_t;

#define WBM_FEATURES_BIT_DISABLE      ((enable_bit_mask_t) 0x0000)
#define WBM_FEATURES_BIT_ENABLE       ((enable_bit_mask_t) 0x0001)

typedef struct
{
    vsdm_prim_t             type;        /* Always VSDM_SET_WBM_FEATURES_REQ */
    phandle_t               phandle;     /* Destination phandle */
    hci_connection_handle_t conn_handle; /* Connection handle */
    enable_bit_mask_t       enable_mask; /* Enable or disable WBM features */
} VSDM_SET_WBM_FEATURES_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_SET_WBM_FEATURES_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;       /* Always VSDM_SET_WBM_FEATURES_CFM */
    phandle_t       phandle;    /* Destination phandle */
    hci_return_t    status;     /* Status of set WBM features */
} VSDM_SET_WBM_FEATURES_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command is used by the Host to request the current Resolvable Private
 * Address our Controller associates with the Identity Address provided.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t      type;     /* Always VSDM_CONVERT_RPA_TO_IA_REQ */
    phandle_t        phandle;  /* Destination phandle */
    BD_ADDR_T        rpa;      /* Resolvable Private Address */
} VSDM_CONVERT_RPA_TO_IA_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_CONVERT_RPA_TO_IA_REQ command.
 * A status value other than zero would imply that the operation has failed.
 * Note that the return parameters also include the address type and privacy
 * mode currently configured for the address provided.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;             /* Always VSDM_CONVERT_RPA_TO_IA_CFM */
    phandle_t       phandle;          /* Destination phandle */
    hci_return_t    status;           /* Status of address resolution */
    TYPED_BD_ADDR_T identity_address; /* Resolved identity address if successful */
    uint8_t         priv_mode;        /* Privacy mode of address entry if present */
} VSDM_CONVERT_RPA_TO_IA_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command is used by the Host to enable or disable truncated page scan 
 * in the Controller.
 *
 *----------------------------------------------------------------------------*/
/**
 * Type definitions to specify enable or disable of truncated page scan.
 */
#define WRITE_TRUNCATED_PAGE_SCAN_DISABLE    ((uint8_t)0x00)
#define WRITE_TRUNCATED_PAGE_SCAN_ENABLE     ((uint8_t)0x01)

typedef struct
{
    vsdm_prim_t      type;     /* Always VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ */
    phandle_t        phandle;  /* Destination phandle */
    uint8_t          enable;   /* Enable or Disable truncated page scan */
} VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ_T;


/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t      type;      /* Always VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM */
    phandle_t        phandle;   /* Destination phandle */
    hci_return_t     status;    /* Status for truncated page scan request */
} VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command is used by the Host to set the streaming mode in the controller.
 *
 *----------------------------------------------------------------------------*/
/**
 * Type definitions to specify streaming mode.
 */
#define STREAMING_MODE_NORMAL    ((uint8_t)0x00)
#define STREAMING_MODE_APTX      ((uint8_t)0x01)
#define STREAMING_MODE_GAMING    ((uint8_t)0x02)

typedef struct
{
    vsdm_prim_t      type;     /* Always VSDM_SET_STREAMING_MODE_REQ_T */
    phandle_t        phandle;  /* Destination phandle */
    TP_BD_ADDR_T     tp_addrt; /* Address of the remote device */
    uint8_t          streaming_mode; /* Streaming mode to be set */
} VSDM_SET_STREAMING_MODE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_SET_STREAMING_MODE_REQ_T command 
 * given by the application.
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t      type;      /* Always VSDM_SET_STREAMING_MODE_CFM_T */
    phandle_t        phandle;   /* Destination phandle */
    hci_return_t     status;    /* Status for truncated page scan request */
    TP_BD_ADDR_T     tp_addrt;  /* Address of the remote device */
} VSDM_SET_STREAMING_MODE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command is used by the Host to enable dynamic flush timeout and set the 
 * flush timeout parameters in the controller.
 *
 *----------------------------------------------------------------------------*/

/**
 * Type definition to specify enable or disable of dynamic flush timeout.
 */
#define DYANMIC_FLUSH_TIMEOUT_DISABLE           ((uint8_t)0x00)
#define DYANMIC_FLUSH_TIMEOUT_ENABLE            ((uint8_t)0x01)

/**
 * Type definition to specify LL Mode values for dynamic flush timeout.
 */
#define DYANMIC_FLUSH_TIMEOUT_LL_MODE_NO_CHANGE ((uint8_t)0x00)
#define DYANMIC_FLUSH_TIMEOUT_LL_MODE_HQ        ((uint8_t)0x01)
#define DYANMIC_FLUSH_TIMEOUT_LL_MODE_0         ((uint8_t)0x02)
#define DYANMIC_FLUSH_TIMEOUT_LL_MODE_1         ((uint8_t)0x03)
#define DYANMIC_FLUSH_TIMEOUT_LL_MODE_2         ((uint8_t)0x04)

typedef struct
{
    vsdm_prim_t      type;        /* Always VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_REQ */
    phandle_t        phandle;     /* Destination phandle */
    uint8_t          cig_id;      /* CIG identifier */
    uint8_t          enable;      /* Disable or Enable dynamic flush timeout */
    uint8_t          ll_mode;     /* Mode */
    uint8_t          min_ft_c_to_p; /* Minimum flush timeout central to peripheral */
    uint8_t          max_ft_c_to_p; /* Maximum flush timeout central to peripheral */
    uint8_t          min_ft_p_to_c; /* Minimum flush timeout peripheral to central */
    uint8_t          max_ft_p_to_c; /* Maximum flush timeout peripheral to central */
    uint8_t          ttp_adjust_rate; /* Rate at which system latency is adjusted */
} VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event will notify the status of VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_REQ_T
 * command given by the application.
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t      type;        /* Always VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_CFM */
    phandle_t        phandle;     /* Destination phandle */
    hci_return_t     status;      /* Status of setting the flush timeout range */
    uint8_t          cig_id;      /* CIG identifier */
} VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This command enables/disables proprietary Host feature support.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t      type;        /* Always VSDM_QLE_SET_HOST_FEATURES_REQ */
    phandle_t        phandle;     /* Destination phandle */
    uint8_t          bit_number;  /* Feature mask index */
    uint8_t          bit_value;   /* Enable/disable feature */
} VSDM_QLE_SET_HOST_FEATURES_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event indicates the status of a VSDM_QLE_SET_HOST_FEATURES_REQ command
 * Status value other than zero would imply that the operation has failed.
 *
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t      type;        /* Always VSDM_QLE_SET_HOST_FEATURES_CFM */
    phandle_t        phandle;     /* Destination phandle */
    hci_return_t     status;      /* Status of setting the QLE Host feature */
} VSDM_QLE_SET_HOST_FEATURES_CFM_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * The VSDM_QLE_SET_CIG_QHS_MAP_REQ command is used to indicate the (Max_PDU_C_To_P, 
 * Max_PDU_P_To_C, BN_C_To_P, and BN_P_To_C) values for each QHS rate to be used 
 * for one or more CISes in a CIG. The Host shall create a CIG using the 
 * HCI_LE_Set_CIG_Parameters or the HCI_LE_Set_CIG_Parameters_Test command before 
 * issuing this command.
 *----------------------------------------------------------------------------*/
#define VSDM_MAX_SUPPORTED_CIS 2
#define VSDM_MAX_NUM_OF_QHS_RATE  5

typedef struct
{
    uint8_t          bn_c_to_p;   /* Burst number from the Central to Peripheral
                                     for a given QHS rate. If this parameter is 
                                     set to zero then the corresponding QHS rate 
                                     shall not be used on the CIS from Central to 
                                     Peripheral */
    uint8_t          bn_p_to_c;   /* Burst number from the Peripheral to Central
                                     for a given QHS rate. If this parameter is 
                                     set to zero then the corresponding QHS rate 
                                     shall not be used on the CIS from Central to 
                                     Peripheral */
    uint16_t         max_pdu_c_to_p;  /* maximum PDU size from Central to Peripheral */
    uint16_t         max_pdu_p_to_c;  /* maximum PDU size from Peripheral to Central */
} CIS_QHS_PARAMS;

typedef struct
{
    uint8_t          cis_id;      /* CIS identifer */
    /* CIS parameters for all QHS-rate starting from qhs-rate-6(0th indx).
       Ex: cis_params_qhs[0] = cis_params_qhs6
           cis_params_qhs[1] = cis_params_qhs5
           cis_params_qhs[2] = cis_params_qhs4
           cis_params_qhs[3] = cis_params_qhs3
           cis_params_qhs[4] = cis_params_qhs2
    */
    CIS_QHS_PARAMS   cis_params_qhs[VSDM_MAX_NUM_OF_QHS_RATE];
} VSDM_CIS_QHS_MAP;

typedef struct
{
    vsdm_prim_t      type;        /* Always VSDM_QLE_SET_CIG_QHS_MAP_REQ */
    phandle_t        phandle;     /* Destination phandle */
    uint8_t          cig_id;      /* CIG identifier */
    uint8_t          flags;       /* Max-pdu fields length flag*/
    uint8_t          cis_count;   /* CIS count */
    VSDM_CIS_QHS_MAP  cis_qhs_map[VSDM_MAX_SUPPORTED_CIS]; /* CIS parameters and QHS map */
} VSDM_QLE_SET_CIG_QHS_MAP_REQ_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 *
 * This event indicates the status of a VSDM_QLE_SET_CIG_QHS_MAP_REQ command
 * Status value other than zero would imply that the operation has failed.
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t      type;        /* Always VSDM_QLE_SET_CIG_QHS_MAP_CFM */
    phandle_t        phandle;     /* Destination phandle */
    hci_return_t     status;      /* Status of setting the CIG QHS map feature. */
    uint8_t          cig_id;      /* CIG identifier */
} VSDM_QLE_SET_CIG_QHS_MAP_CFM_T;


/*----------------------------------------------------------------------------*
 * PURPOSE
 * 
 * This event indicates that the controller reported link quality related 
 * BQR(Bluetooth Quality Report) sub-event on BR/EDR ACL link to the host.
 *
 * vs_data[0][0] = 0x58 (Bluetooth_Quality_Report)
 * vs_data[0][1] = 0x01 (Bluetooth_Quality_Monitoring_Report_Subevent)
 * vs_data[0][2] = Android_Parameters(variable length).
 *                 Refer QBCE specification for details
 * vs_data[n][n] = Last 24 octets points to Vendor_Specific_Parameter
 *
 * Refer to Bluetooth_Quality_Monitoring_Report_Subevent vendor specific event 
 * in the QBCE specification for the details of the parameters in vs_data[].
 *----------------------------------------------------------------------------*/
typedef struct
{
    vsdm_prim_t     type;        /* Always VSDM_BT_QUALITY_MONITORING_REPORT_IND */
    phandle_t       phandle;     /* Destination phandle */
    uint8_t         data_length; /* Total length of data packed in vs_data[] */
    /* Event parameters in the vendor specific event.
       Each array element can hold maximum 32 octets(HCI_VAR_ARG_POOL_SIZE)
    */
    uint8_t         *vs_data[HCI_VS_DATA_BYTE_PACKET_PTRS];
} VSDM_BT_QUALITY_MONITORING_REPORT_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 * 
 * This event indicates that the controller reported link quality event due to
 * choppy voice on eSCO link to the host.
 *
 * vs_data[0][0] = 0x58 (Bluetooth_Quality_Report)
 * vs_data[0][1] = 0x04 (Bluetooth_xSCO_Choppy_Report_Subevent)
 * vs_data[0][2] = Android_Parameters(variable length).
 *                 Refer QBCE specification for details
 * vs_data[n][n] = Last 56 octets points to Vendor_Specific_Parameter
 *
 * Refer to Bluetooth_xSCO_Choppy_Report_Subevent vendor specific event in the 
 * QBCE specification for the details of the parameters in vs_data[].
 *----------------------------------------------------------------------------*/
typedef VSDM_BT_QUALITY_MONITORING_REPORT_IND_T VSDM_BT_XSCO_VOICE_CHOPPY_REPORT_IND_T;

/*----------------------------------------------------------------------------*
 * PURPOSE
 * 
 * This event indicates that the controller reported link quality event due to 
 * choppy LE audio on ISO link to the host.
 *
 * vs_data[0][0] = 0x58 (Bluetooth_Quality_Report)
 * vs_data[0][1] = 0x07 (Bluetooth_LE_Choppy_Audio_Report_Subevent)
 * vs_data[0][2] = Android_Parameters(variable length).
 *                 Refer QBCE specification for details
 * vs_data[n][n] = Last 56 octets points to Vendor_Specific_Parameter
 *
 * Refer to Bluetooth_LE_Choppy_Audio_Report_Subevent vendor specific event in
 * the QBCE specification for the details of the parameters in vs_data[].
 *----------------------------------------------------------------------------*/
typedef VSDM_BT_QUALITY_MONITORING_REPORT_IND_T VSDM_BT_LE_AUDIO_CHOPPY_REPORT_IND_T;

/*! \brief Union of the primitives */
typedef union
{
    /* Shared */
    vsdm_prim_t                        type;

    /* Downstream */
    VSDM_REGISTER_REQ_T                          vsdm_register_req;
    VSDM_READ_LOCAL_QLM_SUPP_FEATURES_REQ_T      vsdm_read_local_qlm_supp_features_req;
    VSDM_READ_REMOTE_QLM_SUPP_FEATURES_REQ_T     vsdm_read_remote_qlm_supp_features_req;
    VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_REQ_T       vsdm_write_sc_host_supp_override_req;
    VSDM_READ_SC_HOST_SUPP_OVERRIDE_REQ_T        vsdm_read_sc_host_supp_override_req;
    VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_REQ_T   vsdm_write_sc_host_supp_cod_override_req;
    VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_REQ_T    vsdm_read_sc_host_supp_cod_override_req;
    VSDM_SET_QHS_HOST_MODE_REQ_T                 vsdm_set_qhs_host_mode_req;
    VSDM_SET_WBM_FEATURES_REQ_T                  vsdm_set_wbm_features_req;
    VSDM_CONVERT_RPA_TO_IA_REQ_T                 vsdm_convert_rpa_to_ia_req;
    VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_REQ_T  vsdm_write_truncated_page_scan_enable_req;
    VSDM_SET_STREAMING_MODE_REQ_T                vsdm_set_streaming_mode_req;
    VSDM_READ_REMOTE_QLL_SUPP_FEATURES_REQ_T     vsdm_read_remote_qll_supp_features_req;
    VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_REQ_T       vsdm_qle_set_flush_timeout_range_req;
    VSDM_QLE_SET_HOST_FEATURES_REQ_T             vsdm_qle_set_host_features_req;
    VSDM_QLE_SET_CIG_QHS_MAP_REQ_T               vsdm_qle_set_cig_qhs_map_req;
    VSDM_SET_EVENT_MASK_REQ_T                    vsdm_set_event_mask_req;

    /* Upstream */
    VSDM_REGISTER_CFM_T                          vsdm_register_cfm;
    VSDM_READ_LOCAL_QLM_SUPP_FEATURES_CFM_T      vsdm_read_local_qlm_supp_features_cfm;
    VSDM_READ_REMOTE_QLM_SUPP_FEATURES_CFM_T     vsdm_read_remote_qlm_supp_features_cfm;
    VSDM_WRITE_SC_HOST_SUPP_OVERRIDE_CFM_T       vsdm_write_sc_host_supp_override_cfm;
    VSDM_READ_SC_HOST_SUPP_OVERRIDE_CFM_T        vsdm_read_sc_host_supp_override_cfm;
    VSDM_QLM_CONNECTION_COMPLETE_IND_T           vsdm_qlm_connection_complete_ind;
    VSDM_QCM_PHY_CHANGE_IND_T                    vsdm_qcm_phy_change_ind;
    VSDM_WRITE_SC_HOST_SUPP_COD_OVERRIDE_CFM_T   vsdm_write_sc_host_supp_cod_override_cfm;
    VSDM_READ_SC_HOST_SUPP_COD_OVERRIDE_CFM_T    vsdm_read_sc_host_supp_cod_override_cfm;
    VSDM_SET_QHS_HOST_MODE_CFM_T                 vsdm_set_qhs_host_mode_cfm;
    VSDM_SET_WBM_FEATURES_CFM_T                  vsdm_set_wbm_features_cfm;
    VSDM_CONVERT_RPA_TO_IA_CFM_T                 vsdm_convert_rpa_to_ia_cfm;
    VSDM_INCOMING_PAGE_IND_T                     vsdm_incoming_page_ind;
    VSDM_WRITE_TRUNCATED_PAGE_SCAN_ENABLE_CFM_T  vsdm_write_truncated_page_scan_enable_cfm;
    VSDM_SET_STREAMING_MODE_CFM_T                vsdm_set_streaming_mode_cfm;
    VSDM_QLL_CONNECTION_COMPLETE_IND_T           vsdm_qll_connection_complete_ind;
    VSDM_READ_REMOTE_QLL_SUPP_FEATURES_CFM_T     vsdm_read_remote_qll_supp_features_cfm;
    VSDM_QLE_SET_FLUSH_TIMEOUT_RANGE_CFM_T       vsdm_qle_set_flush_timeout_range_cfm;
    VSDM_QLE_SET_HOST_FEATURES_CFM_T             vsdm_qle_set_host_features_cfm;
    VSDM_QLE_SET_CIG_QHS_MAP_CFM_T               vsdm_qle_set_cig_qhs_map_cfm;
    VSDM_SET_EVENT_MASK_CFM_T                    vsdm_set_event_mask_cfm;
    VSDM_BT_QUALITY_MONITORING_REPORT_IND_T      vsdm_bt_quality_monitoring_report_ind;
    VSDM_BT_XSCO_VOICE_CHOPPY_REPORT_IND_T       vsdm_bt_xsco_voice_choppy_report_ind;
    VSDM_BT_LE_AUDIO_CHOPPY_REPORT_IND_T         vsdm_bt_le_audio_choppy_report_ind;
} VSDM_UPRIM_T;

#ifdef __cplusplus
}
#endif

#endif /* _VSDM_PRIM_H_ */
