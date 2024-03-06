/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.
                All Rights Reserved.
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \defgroup   rssi_pairing RSSI Pairing
    @{
    \ingroup    bt_domain
    \brief      Implementation of RSSI Pairing component.
                This component can be used in order to pair to a device using its RSSI value.
                The component will use the Inquiry Manager/LE Scan Manager to start an inquiry/LE scan and get
                returned results.
                If there are more than one result then the result with the highest RSSI value will be chosen
                so long as its RSSI is sufficiently higher then the next result and above the configured RSSI
                Threshold
                If there is only one returned device then that will be chosen if it's RSSI is above the threshhold

                For the chosen device RSSI Pairing will first create an ACL connection and then use the
                Pairing component in order to perform pairing. Once this is completed successfully
                a message will be sent to the client task.

                To pair with a BREDR device, a set of Inquiry parameters must be defined in the application and the 
                chosen index shall be passed to the RSSI pairing module in the call to RssiPairing_Start()

                To pair with a BLE device, a the inquiry parameters, LE scan type and advertising filter must be defined
                in the application and the chosen index shall be passed to the RSSI pairing module in the call to RssiPairing_LeStart()
                
*/

#ifndef RSSI_PAIRING_H_
#define RSSI_PAIRING_H_

#include <inquiry_manager.h>
#include <domain_message.h>

#ifdef ENABLE_LE_RSSI_PAIRING
#include <le_scan_manager.h>
#endif

/*! \brief RSSI Pairing external messages. */
enum rssi_pairing_messages
{
    /*! Confirm pairing is complete */
    RSSI_PAIRING_PAIR_CFM = RSSI_PAIRING_MESSAGE_BASE,

    /*! This must be the final message */
    RSSI_PAIRING_MESSAGE_END
};

/*! \brief Different modes of RSSI pairing */
typedef enum
{
    /*! To pair BREDR device */
    RSSI_PAIRING_MODE_BREDR,

#ifdef ENABLE_LE_RSSI_PAIRING
    /*! To pair LE device */
    RSSI_PAIRING_MODE_LE,

    /*! To pair LE, BREDR or dual mode device. If device to pair is a dual mode device,
        LE connection will be established. */
    RSSI_PAIRING_MODE_DUAL_PREF_LE,

    /*! To pair LE, BREDR or dual mode device. If device to pair is a dual mode device,
        BREDR connection will be established. */
    RSSI_PAIRING_MODE_DUAL_PREF_BREDR
#endif /* ENABLE_LE_RSSI_PAIRING */
} rssi_pairing_mode_t;

/*! \brief Hook function registered by client to further filter advertisement reports. The advertisement is only
    processed if this function returns TRUE. */
typedef bool (*rssi_le_adv_hook_callback_t)(const uint8 *adv_report, uint16 adv_report_len);

/*! \brief LE RSSI parameters */
typedef struct
{
#ifdef ENABLE_LE_RSSI_PAIRING
    /*! LE Extended Advert filtering parameter */
    le_extended_advertising_filter_t *filter_param;

    /*! Callback function to accept the adv or not after applying above filter */
    rssi_le_adv_hook_callback_t cb;

    /*! Class of device filter used to identify an LE capable device from a BREDR
        inquiry result. Eg. To identify a dual mode device
        Pass 0 if this filter is not needeed. */
    uint32 cod_filter;
#else /* ENABLE_LE_RSSI_PAIRING */
    uint32 dummy;
#endif /* ENABLE_LE_RSSI_PAIRING */
} rssi_pairing_le_scan_parameters_t;

/*! \brief Type of the device which got paired */
typedef enum
{
    /*! Device type is unknown */
    RSSI_PAIRING_DEVICE_TYPE_UNKNOWN,

    /*! Device which supports BREDR only */
    RSSI_PAIRING_DEVICE_TYPE_BREDR,

#ifdef ENABLE_LE_RSSI_PAIRING
    /*! Device which supports LE only */
    RSSI_PAIRING_DEVICE_TYPE_LE,

    /*! Device which supports both BREDR and LE */
    RSSI_PAIRING_DEVICE_TYPE_DUAL
#endif
} rssi_pairing_device_type_t;

/*! \brief Definition of the #RSSI_PAIRING_PAIR_CFM_T message content */
typedef struct
{
    /*! Status if the pairing was a success. */
    bool status;

    /*! Type of the device which got paired. ie, LE/BREDR/dual */
    rssi_pairing_device_type_t device_type;

    /*! The device address that was paired */
    bdaddr bd_addr;
} RSSI_PAIRING_PAIR_CFM_T;

/*! \brief Rssi Pairing parameters*/
typedef struct
{
    /*! The minimum gap between the first and second candidate.
        i.e. There must be this much of a gap in the RSSI values in order for Pairing to happen*/
    uint16 rssi_gap;

    /*! The minimum threshold that a device must be over for it to be chosen as a candidate */
    int16 rssi_threshold;

    /*! The index of the inquiry set defined in the application that should be used */
    uint16 inquiry_filter;

    /*! Total number of inquiries. If 0, no inquiries will be performed */
    uint16 inquiry_count;
} rssi_pairing_parameters_t;

/*! \brief Initialise the RSSI Pairing Component.

    \param init_task Not used
    \return TRUE
*/
bool RssiPairing_Init(Task init_task);

/*! \brief Starts RSSI pairing for LE/BREDR device (or both)

    \param client_task The task that shall receive the pairing confirmation.
    \param mode RSSI pairing mode ie, either LE, BREDR or both (dual mode)
    \param scan_parameters Pointer to the scan parameters to start the LE RSSI Pairing process with.
    \param le_parameters Pointer to LE scan parameters.

    \return TRUE if RSSI Pairing was successfully started.

     Note: Message RSSI_PAIRING_PAIR_CFM will be send to the task indicating the status of RSSI pairing.
           It will also have device type field which will indicate the type of device which got paired
           (if RSSI pairing is successfull).
           If RSSI pairing is started in dual mode, device with better RSSI will be choosen to pair
           and this can be any device LE/BREDR/dual.
*/
bool RssiPairing_Start(Task client_task,
                       rssi_pairing_mode_t mode,
                       const rssi_pairing_parameters_t *scan_parameters,
                       const rssi_pairing_le_scan_parameters_t *le_scan_parameters);

/*! \brief Stop RSSI Pairing Immediately.
           This will return a RSSI_PAIRING_PAIR_CFM message to the client task with status FALSE
*/
void RssiPairing_Stop(void);

/*! \brief Check if the RSSI Pairing module is active.
           i.e. if it is inquiry scanning or attempting to pair

    \return TRUE if pairing module is active.
*/
bool RssiPairing_IsActive(void);

/*! \brief Start RSSI pairing for BREDR devices.

    \param client_task The task that shall receive the pairing confirmation.
    \param scan_parameters Pointer to the scan parameters to start the RSSI Pairing process with.

    \return TRUE if RSSI Pairing was successfully started.

    Note: The task will receive RSSI_PAIRING_PAIR_CFM which will indicate pairing is succeeded or not
*/
#define RssiPairing_BredrStart(client_task, scan_parameters) \
    RssiPairing_Start(client_task, RSSI_PAIRING_MODE_BREDR, scan_parameters, NULL)

/*! \brief Stop BREDR RSSI Pairing Immediately.
           This will return a RSSI_PAIRING_PAIR_CFM message to the client task with status FALSE
*/
#define RssiPairing_BredrStop() \
    RssiPairing_Stop()

#ifdef ENABLE_LE_RSSI_PAIRING

/*! \brief Starts the RSSI Pairing for LE devices

    \param client_task The task that shall receive the pairing confirmation.
    \param scan_parameters Pointer to the scan parameters to start the LE RSSI Pairing process with.
    \param le_parameters Pointer to LE scan parameters.

    \return TRUE if LE RSSI Pairing was successfully started.

     Note: RSSI_PAIRING_PAIR_CFM will be received with a device type which indicates the type of the
           device which got paired.
*/
#define RssiPairing_LeStart(client_task, scan_parameters, le_scan_parameters) \
    RssiPairing_Start(client_task, RSSI_PAIRING_MODE_LE, scan_parameters, le_scan_parameters)

/*! \brief Stop LE RSSI Pairing Immediately.
           This will return a RSSI_PAIRING_PAIR_CFM message to the client task with status FALSE
*/
#define RssiPairing_LeStop()            RssiPairing_Stop()

#endif /* ENABLE_LE_RSSI_PAIRING */

#endif /* RSSI_PAIRING_H_ */
/*! @} */ 