/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   hidd_profile HID Device profile
    @{
    \ingroup    profiles
    \brief      HID Device profile
*/
#ifndef HIDDEVICE_PROFILE_H
#define HIDDEVICE_PROFILE_H
#include "domain_message.h"
#include "task_list.h"
#include "device.h"

/*! \brief Report Types */
typedef enum
{
    hidd_profile_report_type_other,
    hidd_profile_report_type_input,
    hidd_profile_report_type_output,
    hidd_profile_report_type_feature,
}hidd_report_type_t;

/*! \brief Handshake status */
typedef enum
{
    hidd_profile_handshake_success,
    hidd_profile_handshake_not_ready,
    hidd_profile_handshake_invalid_report_id,
    hidd_profile_handshake_unsupported,
    hidd_profile_handshake_invalid_parameter,
    hidd_profile_handshake_unknown,
    hidd_profile_handshake_fatal
}hidd_handshake_result_t;

/*! \brief HIDD Profile status */
typedef enum
{
    hidd_profile_status_connected,
    hidd_profile_status_connect_failed,
    hidd_profile_status_disconnected,
    hidd_profile_status_conn_loss,
    hidd_profile_status_reconnecting,
    hidd_profile_status_unacceptable_parameters,
    hidd_profile_status_sdp_register_failed,
    hidd_profile_status_timeout,
    hidd_profile_status_conn_term_by_remote,
    hidd_profile_status_conn_term_by_local
} hidd_profile_status_t;

typedef struct {
    hidd_profile_status_t status;
} HIDD_PROFILE_ACTIVATE_CFM_T;

typedef struct {
    hidd_profile_status_t status;
    bdaddr addr;
    uint32 connid;
} HIDD_PROFILE_CONNECT_IND_T;

typedef struct {
    bdaddr addr;
    bool successful;
} HIDD_PROFILE_CONNECT_CFM_T;

typedef struct {
    hidd_profile_status_t status;
    bdaddr addr;
} HIDD_PROFILE_DISCONNECT_IND_T;

typedef struct {
    bdaddr addr;
    bool successful;
} HIDD_PROFILE_DISCONNECT_CFM_T;

typedef struct {
    hidd_report_type_t report_type;
    uint16 size;
    uint8 reportid;
} HIDD_PROFILE_GET_REPORT_IND_T;

typedef struct {
    hidd_report_type_t type;
    uint8 reportid;
    uint16 reportLen;
    uint8* data;
} HIDD_PROFILE_SET_REPORT_IND_T;

typedef HIDD_PROFILE_SET_REPORT_IND_T HIDD_PROFILE_DATA_IND_T;

typedef struct {
    hidd_profile_status_t status;
} HIDD_PROFILE_DATA_CFM_T;

typedef struct {
    hidd_profile_status_t status;
} HIDD_PROFILE_DEACTIVATE_CFM_T;

/*! \brief Confirmation Messages to be sent to clients */
typedef enum hidd_profile_messages
{
    /*! HIDD SDP registered */
    HIDD_PROFILE_ACTIVATE_CFM = HIDD_PROFILE_MESSAGE_BASE,
    /*! Connect indication */
    HIDD_PROFILE_CONNECT_IND,
    /*! Connect Confirmation for locally initiated connection request */
    HIDD_PROFILE_CONNECT_CFM,
    /*! Disconnect indication */
    HIDD_PROFILE_DISCONNECT_IND,
    /*! Disconnect Confirmation for locally initiated disconnection request */
    HIDD_PROFILE_DISCONNECT_CFM,
    /*! Data Indication (usually output reports on interrupt channel) */
    HIDD_PROFILE_DATA_IND,
    /*! Set Report Indication */
    HIDD_PROFILE_SET_REPORT_IND,
    /*! Get Report Indication */
    HIDD_PROFILE_GET_REPORT_IND,
    /*! Confirmation in response to HiddProfile_DataReq */
    HIDD_PROFILE_DATA_CFM,
    /*! Deactivation (and disconnection if required) complete. */
    HIDD_PROFILE_DEACTIVATE_CFM,
    HIDD_PROFILE_MESSAGE_END
}hidd_profile_messages_t;

#ifdef USE_SYNERGY
#ifdef INCLUDE_HIDD_PROFILE
/*! \brief Initiate HID Device Profile
    \return TRUE if Init was successfull, FALSE otherwise
*/
bool HiddProfile_Init(Task task);

/*! \brief Register Client application
    \param client_task The task to send notifications
    \param sdplen Length of sdp data
    \param sdp SDP data buffer
    \param Interrupt Channel flush timeout (0xFFFF to disable)
*/
void HiddProfile_RegisterDevice(Task client_task, uint16 sdplen, const uint8* sdp, uint16 int_flush_timeout);

/*! \brief Connect to HID Host
    \param remote_device bdaddr of HID Host
    \return TRUE if connect requested, FALSE otherwise
*/
bool HiddProfile_Connect(bdaddr *bd_addr);

/*! \brief Disconnect from HID Host
    \param remote_device bdaddr of HID Host
    \return TRUE if connect requested, FALSE otherwise
*/
bool HiddProfile_Disconnect(bdaddr *remote_device);

/*! \brief Send data over interrupt channel.
    \param reportid Report ID
    \param datalen Length of report data
    \param data Report data buffer.
    \return TRUE if disconnect requested, FALSE otherwise
*/
bool HiddProfile_DataReq(uint8 reportid, uint16 datalen, const uint8* data);

/*! \brief Send handshake to HID host. This is usually required as a response to
 *         Get_Report and Set_Report messages from the host.
    \param status handshake status
    \return TRUE if Handshake was sent, FALSE otherwise
*/
bool HiddProfile_Handshake(hidd_handshake_result_t status);

/*! \brief Send Data on control channel. Typically in response to Get_report/Get_protocol from HID host
    \param report_type Type of report - input/output/feature/other
    \param reportid Report ID
    \param datalen Length of data
    \param data Report data
    \return TRUE if Report was sent, FALSE otherwise
*/
bool HiddProfile_DataRes(hidd_report_type_t report_type, uint8 reportid, uint16 datalen, uint8* data);

/*! \brief Deinitialize and disconnect (if connected) hidd profile
    \param client_task The task to notfiy when the operation is complete
*/
void HiddProfile_Deinit(void);

/*! \brief Get Connection ID for current connection.
    \param none
    \return connection_id (non zero for a valid connection)
*/
uint32 HiddProfile_GetConnectionId(void);

/*! \brief Check if HID Profile is connected with any device
    \param none
    \return TRUE if connected
*/
bool HiddProfile_IsConnected(void);

#else /*!INCLUDE_HIDD_PROFILE*/
#define HiddProfile_Init(t) (TRUE)
#define HiddProfile_RegisterDevice(t, l, s, f) UNUSED(l); UNUSED(s); UNUSED(f)
#define HiddProfile_Connect(a) (TRUE)
#define HiddProfile_Disconnect (TRUE)
#define HiddProfile_DataReq(t, d, l) UNUSED(d); UNUSED(l); TRUE
#define HiddProfile_Handshake(s) (TRUE)
#define HiddProfile_DataRes(t, r, l, d) UNUSED(r); UNUSED(l); UNUSED(d); TRUE
#define HiddProfile_Deinit(t)
#define HiddProfile_GetConnectionId() (0)
#define HiddProfile_IsConnected() (FALSE)
#endif /*INCLUDE_HIDD_PROFILE*/
#endif /*USE_SYNERGY*/
#endif /*HIDDEVICE_PROFILE_H*/

/*! @} */