/*!
    \copyright  Copyright (c) 2020 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       qualcomm_connection_manager.h
    \defgroup   qualcomm_connection_manager Qualcomm Connection Manager
    @{
    \ingroup    bt_domain
    \brief      Header file for Qualcomm Connection Manager
*/

#ifndef __QCOM_CON_MANAGER_H
#define __QCOM_CON_MANAGER_H


#include <message.h>
#include <bdaddr.h>
#include <hci.h>
#include "domain_message.h"

/*! \brief Type definition to specify enable or disable of dynamic flush timeout. */
#define QCOM_CON_MANAGER_SET_FLUSH_TIMEOUT_ENABLE            0x01
#define QCOM_CON_MANAGER_SET_FLUSH_TIMEOUT_DISABLE           0x00

#define QCOM_CON_MANAGER_FIXED_LENGTH_PDU                    0X00
#define QCOM_MAX_SUPPORTED_CIS                                  2
#define QCOM_MAX_NUM_OF_QHS_RATE                                5

/*! \brief LL Mode values for dynamic flush timeout. */
typedef enum {
    QCOM_CON_SET_FLUSH_LL_MODE_NO_CHANGE,
    QCOM_CON_SET_FLUSH_LL_MODE_HQ,
    QCOM_CON_SET_FLUSH_LL_MODE_0,
    QCOM_CON_SET_FLUSH_LL_MODE_1,
    QCOM_CON_SET_FLUSH_LL_MODE_2
} QCOM_CON_SET_FLUSH_LL_MODE_T;

/*! \brief QHS level bit mask. */
#define QCOM_QHS_2_MASK            (0x01)
#define QCOM_QHS_3_MASK            (0x02)
#define QCOM_QHS_4_MASK            (0x04)
#define QCOM_QHS_5_MASK            (0x10)
#define QCOM_QHS_6_MASK            (0x20)

/*! \brief QHS levels. */
#define QCOM_QHS_LEVEL_2           2
#define QCOM_QHS_LEVEL_3           3
#define QCOM_QHS_LEVEL_4           4
#define QCOM_QHS_LEVEL_5           5
#define QCOM_QHS_LEVEL_6           6
#define QCOM_QHS_LEVEL_INVALID     0

/*! \brief vendor information */

typedef struct{

     /*! Company/vendor specific id. */
    uint16 comp_id;
    /*! Minimum lmp version for sc/qhs support. */
    uint8 min_lmp_version;
    /*! Minimum lmp sub version for sc/qhs support. */
    uint16 min_lmp_sub_version;

} QCOM_CON_MANAGER_SC_OVERRIDE_VENDOR_INFO_T;

/*! \brief Flush timeout information */

typedef struct{

    /*! CIG Identifier to which this FT update is to be applied. */
    uint8 cig_id;
    /*! Enable/Disable dynamic FT changes. */
    uint8 enable;
    /*! Latency Mode. Range 0x00 - 0x04 */
    uint8 ll_mode;
    /*! Min Flush timeout in multiples of ISO_Interval from Central to Peripheral. */
    uint8 min_ft_c_to_p;
    /*! Max Flush timeout in multiples of ISO_Interval from Central to Peripheral. */
    uint8 max_ft_c_to_p;
    /*! Min Flush timeout in multiples of ISO_Interval from Peripheral to Central. */
    uint8 min_ft_p_to_c;
    /*! Max Flush timeout in multiples of ISO_Interval from Peripheral to Central. */
    uint8 max_ft_p_to_c;
    /*! rate at which the system latency is adjusted in units of 0.1 ms per 500 ms. */
    uint8 ttp_adjust_rate;

} QCOM_CON_MANAGER_SET_FLUSH_TIMEOUT_RANGE_PARAM_T;

/*! \brief QHS parameters */
typedef struct
{
    /*! Burst number from the Central to Peripheral for a given QHS rate */
    uint8_t          bn_c_to_p;
    /*! Burst number from the Peripheral to Central for a given QHS rate.*/
    uint8_t          bn_p_to_c;
    /*! maximum PDU size from Central to Peripheral */
    uint16_t         max_pdu_c_to_p;
    /*! maximum PDU size from Peripheral to Central */
    uint16_t         max_pdu_p_to_c;
} QCOM_CON_MANAGER_CIS_QHS_PARAMS_T;

/*! \brief Set CIG QHS Mapping */
typedef struct{
    uint8_t          cig_id;      /* CIG identifier */
    uint8_t          flags;       /* Max-pdu fields length flag*/
    uint8_t          cis_count;   /* CIS count */
    uint8_t          cis_ids[QCOM_MAX_SUPPORTED_CIS];
    /* CIS parameters for all QHS-rate starting from qhs-rate-6(0th indx).
       Ex: cis_params_qhs[0] = cis_params_qhs6
           cis_params_qhs[1] = cis_params_qhs5
           cis_params_qhs[2] = cis_params_qhs4
           cis_params_qhs[3] = cis_params_qhs3
           cis_params_qhs[4] = cis_params_qhs2*/
    const QCOM_CON_MANAGER_CIS_QHS_PARAMS_T   *cis_qhs_map;
}QCOM_CON_MANAGER_SET_CIG_QHS_MAP_REQ_T;

/*! \brief Vendor information array buffer to be provided by application.
     This array will be terminated by setting company id(comp_id) to 0.
     Maximum number of company ids supported are VSDM_MAX_NO_OF_COMPIDS.
*/
extern const QCOM_CON_MANAGER_SC_OVERRIDE_VENDOR_INFO_T vendor_info[];

/*! \brief Events sent by qualcomm connection manager module to other modules. */
typedef enum
{
    /*! Module initialisation complete */
    QCOM_CON_MANAGER_INIT_CFM = QCOM_CON_MANAGER_MESSAGE_BASE,

    /*! QHS Established Indication to mirroring profile */
    QCOM_CON_MANAGER_QHS_CONNECTED,

    /*! Connection Barge-In Indication */
    QCOM_CON_MANAGER_BARGE_IN_IND,

    /*! Truncated scan enable confirmation */
    QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM,

    /*! Remote supports QHS on ISO indication */
    QCOM_CON_MANAGER_REMOTE_ISO_QHS_CAPABLE_IND,

#ifdef ENABLE_ACK_FOR_PA_TRANSMITTED
    /*! PA Ack received for the first PA which is has been sent with updated metadata */
    QCOM_CON_MANAGER_PA_TRANSMITTED_ACK_IND,
#endif

    /*! QHS rate Change Indication */
    QCOM_CON_MANAGER_QHS_RATE_CHANGED_IND,

    /*! This must be the final message */
    QCOM_CON_MANAGER_MESSAGE_END
}qcm_msgs_t;

typedef struct
{
    hci_return_t status;

} QCOM_CON_MANAGER_TRUNCATED_SCAN_ENABLE_CFM_T;

typedef struct
{
    /*! BT address of the device to barge-in. */
    bdaddr bd_addr;

} QCOM_CON_MANAGER_BARGE_IN_IND_T;

typedef struct
{
    /*! BT address of qhs connected device. */
    bdaddr bd_addr;

} QCOM_CON_MANAGER_QHS_CONNECTED_T;

typedef struct
{
    /*! BT address of the remote device. */
    tp_bdaddr tp_addr;

} QCOM_CON_MANAGER_REMOTE_ISO_QHS_CAPABLE_IND_T;

typedef struct
{
    /*! CIG Identifier */
    uint8_t cig_id;

    /*! New QHS Rate
    *      QHS rate bit mask is 1 octet and the bits are represented as follows:
    *      Bit-0 --> QHS2
    *      Bit-1 --> QHS3
    *      Bit-2 --> QHS4
    *      Bit-3 --> QHS5
    *      Bit-4 --> QHS6
    *      Bit-5  to Bit-7 are reserved*/
    uint8_t qhs_rate;

} QCOM_CON_MANAGER_QHS_RATE_CHANGED_IND_T;

#ifdef INCLUDE_QCOM_CON_MANAGER

/*! \brief Initialise the qualcomm connection manager module.
 */
bool QcomConManagerInit(Task init_task);

/*! \brief Register a client task to receive notifications of qualcomm connection manager.

    \param[in] client_task Task which will receive notifications from qualcomm conenction manager.
 */
void QcomConManagerRegisterClient(Task client_task);

/*! \brief Unregister a client task to stop receiving notifications from qualcomm conenction manager.

    \param[in] client_task Task to unregister.
 */
void QcomConManagerUnRegisterClient(Task client_task);

/*! \brief Query if fast exit sniff subrate is supported by the local and remote
           addressed device.
    \param[in] addr Address of the remote device.
    \return TRUE if both local and remote devices support the feature.
*/
bool QcomConManagerIsFastExitSniffSubrateSupported(const bdaddr *addr);

/*! \brief To enable/disable the support of the Weak Bitmask(WBM) propagation 
           feature on an ISO handle 
    \param[in] conn_handle CIS/BIS connection handle
    \param[in] enable TRUE to Enable WBM FALSE to Disable WBM
*/
void QcomConManagerSetWBMFeature(hci_connection_handle_t conn_handle,bool enable);

/*! \brief To enable/disable truncated page scan

    \param[in] client_task Task to send request to enable truncatd page scan.
    \param[in] enable TRUE to Enable FALSE to Disable truncated page scan
*/
void QcomConManagerEnableTruncatedScan(Task client_task,bool enable);

void QcomConManagerRegisterBargeInClient(Task task);

/*! \brief To set the TWM streaming mode

    \param[in] tp_addrt address of connected device
    \param[in] streaming mode. 0 = disabled, 1 = aptX adaptive Low Latency & High Bandwidth mode, 2 = Gaming Mode
 */
void QcomConManagerSetStreamingMode(tp_bdaddr *tp_addr, uint8_t streaming_mode);

/*! \brief To Enable/Disable the Flush Timeout feature, used by the Host to set the minimum and maximum Flush Timeout (FT)
           values and to enable or disable the mode of operation where the FT value is adjusted
           dynamically by the Controller.

    \param[in] flush_timeout_req Flush timeout range
 */
void QcomConManagerSetFlushTimeoutRange(const QCOM_CON_MANAGER_SET_FLUSH_TIMEOUT_RANGE_PARAM_T *flush_timeout_req);

/*! \brief Send QHS Map request

    \param[in] qhs_map_req QHS Map table for each of the QHS type
 */
void QcomConManagerSetQHSMapReq(const QCOM_CON_MANAGER_SET_CIG_QHS_MAP_REQ_T *qhs_map_req);
#else
#define QcomConManagerInit(task) UNUSED(task)

#define QcomConManagerRegisterClient(task) UNUSED(task)

#define QcomConManagerUnRegisterClient(task) UNUSED(task)

#define QcomConManagerIsFastExitSniffSubrateSupported(addr) (UNUSED(addr), FALSE)

#define QcomConManagerSetWBMFeature(conn_handle, enable) ((void) conn_handle, (void) enable)

#define QcomConManagerEnableTruncatedScan(task, enable) (UNUSED(task), UNUSED(enable))

#define QcomConManagerRegisterBargeInClient(task) UNUSED(task)

#define QcomConManagerSetFlushTimeoutRange(flush_timeout_req) (UNUSED(flush_timeout_req))

#define QcomConManagerSetQHSMapReq(qhs_map_req) (UNUSED(qhs_map_req))
#endif /* INCLUDE_QCOM_CON_MANAGER */

#endif /*__QCOM_CON_MANAGER_H*/
/*! @} */ 