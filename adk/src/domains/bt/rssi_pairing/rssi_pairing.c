/*!
    \copyright  Copyright (c) 2021-2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \ingroup    rssi_pairing
    \brief      Component managing pairing based on RSSI strength
*/

#include "rssi_pairing.h"

#include <logging.h>
#include <inquiry_manager.h>
#include <unexpected_message.h>
#include <pairing.h>
#include <connection_manager.h>
#include <panic.h>
#include <ui.h>

#define NUMBER_OF_INQUIRY_RESULTS 2

/*! Maximum size of LE capable BREDR list */
#define RSSI_PAIRING_LE_CAPABLE_BREDR_DEVICES_LIST_SIZE_MAX   20

/*! RSSI Pairing state */
typedef enum
{
    /*! Idle state */
    RSSI_PAIRING_STATE_IDLE,

    /*! Device search is in progress (either BREDR or LE-or both) */
    RSSI_PAIRING_STATE_DEVICE_SCAN,

    /*! Establishing connection with device to pair */
    RSSI_PAIRING_STATE_ACL_CONNECTING,

    /*! Pairing with the device */
    RSSI_PAIRING_STATE_PAIRING,
} rssi_pairing_state_t;

/*! Structure for a RSSI Pairing device candidate */
typedef struct
{
    /*! RSSI Value of the candidate */
    int16               rssi;

    /*! Bluetooth address of the candidate */
    tp_bdaddr           tp_addr;
} rssi_paring_device_candidate_t;

/*! RSSI Pairing data */
typedef struct
{
    /*! Init's local task */
    TaskData task;

    /*! The selected minimum gap between the first and second candidate.
        i.e. There must be this much of a gap in the RSSI values in order for Pairing to happen*/
    uint16 scan_rssi_gap;

    /*! The selected minimum threshold
        that a device must be over for it to be chosen as a candidate */
    int16 scan_rssi_threshold;

    /*! The number of iterations left in the inquiry scan */
    uint16 inquiry_count;

    /*! The filter to use for the scan */
    uint16 inquiry_filter_index;

    /*! The List of candidates */
    rssi_paring_device_candidate_t inquiry_results[NUMBER_OF_INQUIRY_RESULTS];

    /*! RSSI Pairing state */
    rssi_pairing_state_t state;

    /*! The task to receive RSSI Pairing messages */
    Task client_task;

    /*! RSSI pairing mode. ie, LE, BREDR or dual mode */
    rssi_pairing_mode_t mode;

#ifdef ENABLE_LE_RSSI_PAIRING
    /*! Class of device filter used to identify that a given BREDR device is also capable of LE */
    uint32 cod_filter;

    /*! List of BREDR devices seen during inquiry scanning and that is capable of LE */
    rssi_paring_device_candidate_t *le_capable_bredr_devices_list;

     /*! Number of BREDR devices found which supports LE */
    uint8 le_capable_bredr_devices_list_count;

    /*! Hook function to further filter LE Advert reports */
    rssi_le_adv_hook_callback_t  adv_hook_cb;
#endif /* ENABLE_LE_RSSI_PAIRING */

} rssi_pairing_data_t;

rssi_pairing_data_t rssi_pairing_data;

/*! Get pointer to RSSI Pairing task*/
#define RssiPairing_GetTask() (&rssi_pairing_data.task)

/*! Get pointer to RSSI Pairing data structure */
#define RssiPairing_GetTaskData() (&rssi_pairing_data)

#ifdef ENABLE_LE_RSSI_PAIRING

/*! Do a BLE Scan for five seconds */
#define RSSI_PAIRING_LE_SCAN_DURATION_MS            (5000)

/*! Check if scanning is required for 'only' LE */
#define rssiPairing_IsScanForLeOnly(mode)           (mode == RSSI_PAIRING_MODE_LE)

/*! Check if scanning is required for both LE and BREDR (ie dual) */
#define rssiPairing_IsScanForDualMode(mode)         (mode == RSSI_PAIRING_MODE_DUAL_PREF_LE || \
                                                     mode == RSSI_PAIRING_MODE_DUAL_PREF_BREDR)

/*! Check if scanning is required for both LE and BREDR (ie dual) and preferred connection is BREDR */
#define rssiPairing_IsScanForDualPrefBredr(mode)    (mode == RSSI_PAIRING_MODE_DUAL_PREF_BREDR)

/*! \brief Messages the RSSI Pairing can send to itself. */
typedef enum
{
    /*! Internal message to stop the BLE Scan. */
    RSSI_PAIRING_INTERNAL_STOP_SCAN_IF_DEVICE_FOUND,
} rssiPairingInternalMessage;

/*! Save the BREDR device in a list so that it can be used to identify dual mode device */
static void rssiPairing_SaveDeviceInLeCapabaleBredrList(const bdaddr *bd_addr, int16 rssi);

/*! Check if the given address is present in the LE capable BREDR list */
static bool rssiPairing_IsGivenDeviceSupportsBredrAndLe(const bdaddr *bd_addr);

/*! Stop the LE scan if required */
static bool rssiPairing_StopLeScanIfReqd(bool stop_cond);

#else /* !ENABLE_LE_RSSI_PAIRING */

/*! Check if scanning is required for 'only' LE */
#define rssiPairing_IsScanForLeOnly(mode)           (FALSE)

/*! Check if scanning is required for both LE and BREDR (ie dual) */
#define rssiPairing_IsScanForDualMode(mode)         (FALSE)

/*! Stop the LE scan if required */
#define rssiPairing_StopLeScanIfReqd(stop_cond)     (FALSE)

/*! Get the type of device which got paired  */
#define rssiPairing_GetPairedDeviceType(addr)       (RSSI_PAIRING_DEVICE_TYPE_BREDR)

#endif /* ENABLE_LE_RSSI_PAIRING */

/*! Check if scanning is required for 'only' BREDR */
#define rssiPairing_IsScanForBredrOnly(mode)        (mode == RSSI_PAIRING_MODE_BREDR)

/*! Check if scanning is required for LE (ie either LE only scanning or dual) */
#define rssiPairing_IsScanForLe(mode)               (rssiPairing_IsScanForLeOnly(mode) || \
                                                     rssiPairing_IsScanForDualMode(mode))

/*! Check if scanning is required for BREDR (ie either BREDR only scanning or dual) */
#define rssiPairing_IsScanForBredr(mode)            (rssiPairing_IsScanForBredrOnly(mode) || \
                                                     rssiPairing_IsScanForDualMode(mode))

/*! Send pairing failure indication (if needed) and reset the RSSI module */
static void rssiPairing_SendPairingFailureAndReset(bool update_clients);

/*! \brief Reset the candidate list*/
static void rssiPairing_ResetDevices(void)
{
    rssi_pairing_data.inquiry_results[0].rssi = 0;
    rssi_pairing_data.inquiry_results[1].rssi = 0;
    BdaddrTpSetEmpty(&rssi_pairing_data.inquiry_results[0].tp_addr);
    BdaddrTpSetEmpty(&rssi_pairing_data.inquiry_results[1].tp_addr);
}

/*! \brief Rest the RSSI Pairing manager*/
static void rssiPairing_ResetManager(void)
{
    rssi_pairing_data.client_task = NULL;
    rssi_pairing_data.scan_rssi_gap = 0;
    rssi_pairing_data.scan_rssi_threshold = 0;
    rssi_pairing_data.inquiry_count = 0;
    rssi_pairing_data.inquiry_filter_index = 0;

#ifdef ENABLE_LE_RSSI_PAIRING
    pfree(rssi_pairing_data.le_capable_bredr_devices_list);
    rssi_pairing_data.le_capable_bredr_devices_list = NULL;
    rssi_pairing_data.cod_filter = 0;
    rssi_pairing_data.le_capable_bredr_devices_list_count = 0;
#endif

    rssi_pairing_data.mode = 0;
    rssi_pairing_data.state = RSSI_PAIRING_STATE_IDLE;
    rssiPairing_ResetDevices();

    ConManagerUnregisterTpConnectionsObserver(cm_transport_all, RssiPairing_GetTask());
}

/*! \brief Handler for connection library INQUIRY_MANAGER_RESULT message.
           If the result RSSI is not above the threshold then it is discarded.
           If the candidate list is empty then it is added.
           If the RSSI value is not greater than the top 2 results it is discarded
           An incoming result will remove a previous candidate if its RSSI is greater.

    \param result the Inquiry Manager result*/
static void rssiPairing_HandleInquireManagerResult(const INQUIRY_MANAGER_RESULT_T *result)
{
    DEBUG_LOG_VERBOSE("rssiPairing_HandleInquireManagerResult");
    DEBUG_LOG_VERBOSE("RssiPairing: Inquiry Result:");
    DEBUG_LOG_VERBOSE("     bdaddr 0x%04x 0x%02x 0x%06lx", result->bd_addr.nap,
                                                           result->bd_addr.uap,
                                                           result->bd_addr.lap);
    DEBUG_LOG_VERBOSE("     rssi %d, dev_class: 0x%x", result->rssi, result->dev_class);

    /* if the rssi result is less than the set threshold discard the result. */
    if (result->rssi < rssi_pairing_data.scan_rssi_threshold)
    {
        return;
    }

#ifdef ENABLE_LE_RSSI_PAIRING
    if (rssiPairing_IsScanForDualMode(rssi_pairing_data.mode) &&
        (result->dev_class & rssi_pairing_data.cod_filter))
    {
        DEBUG_LOG_VERBOSE("     dual mode device added to list");

        /* The BREDR device is capable of LE. Store the address in a separate list so that
           we can use this to identify whether the paired LE device supports BREDR or not */
        rssiPairing_SaveDeviceInLeCapabaleBredrList(&result->bd_addr, result->rssi);
        return;
    }
#endif

    /* Update the candidate list if received RSSI is better than the current candidates */
    if (BdaddrIsZero(&rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr) ||
        result->rssi > rssi_pairing_data.inquiry_results[0].rssi)
    {
        DEBUG_LOG_VERBOSE("     Has better RSSI");

        /* Check if address is different from previous peak */
        if (!BdaddrIsSame(&result->bd_addr, &rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
        {
            /* Store previous peak RSSI */
            rssi_pairing_data.inquiry_results[1] = rssi_pairing_data.inquiry_results[0];

            /* Store new address */
            rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr = result->bd_addr;
            rssi_pairing_data.inquiry_results[0].tp_addr.taddr.type = TYPED_BDADDR_PUBLIC;
            rssi_pairing_data.inquiry_results[0].tp_addr.transport = TRANSPORT_BREDR_ACL;

            rssi_pairing_data.inquiry_results[0].rssi = result->rssi;
        }
    }
    else if (BdaddrIsZero(&rssi_pairing_data.inquiry_results[1].tp_addr.taddr.addr)||
             result->rssi > rssi_pairing_data.inquiry_results[1].rssi)
    {
        /* Check if address is different from peak */
        if (!BdaddrIsSame(&result->bd_addr, &rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
        {
            /* Store next highest RSSI */
            rssi_pairing_data.inquiry_results[1].tp_addr.taddr.addr = result->bd_addr;
            rssi_pairing_data.inquiry_results[1].tp_addr.taddr.type = TYPED_BDADDR_PUBLIC;
            rssi_pairing_data.inquiry_results[1].tp_addr.transport = TRANSPORT_BREDR_ACL;
            rssi_pairing_data.inquiry_results[1].rssi = result->rssi;
        }
    }
}

/*! \brief Handler for the INQUIRY_MANAGER_SCAN_COMPLETE message
           If there is atleast one candidate in the list then RSSI pairing will first attempt to create
           an ACL with that device

           If there is more than one result then it will try to connect to the highest RSSI assuming that it is
           sufficiently higher than the next result (peak detection)

           If there are not candidates or the Scan was stopped using RssiPairing_Stop() then
           a RSSI_PAIRING_PAIR_CFM message will be sent with status FALSE */
static void rssiPairing_HandleInquireManagerScanComplete(void)
{
    DEBUG_LOG_FN_ENTRY("rssiPairing_HandleInquireManagerScanComplete");

    DEBUG_LOG_VERBOSE("RSSI Pairing: Inquiry Complete: bdaddr %x,%x,%lx rssi %d, next_rssi %d",
              rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr.nap,
              rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr.uap,
              rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr.lap,
              rssi_pairing_data.inquiry_results[0].rssi,
              rssi_pairing_data.inquiry_results[1].rssi);

    /* RSSI Pairing will be set to idle if RSSI Pairing was Stopped using RssiPairing_Stop() */
    if (rssi_pairing_data.state != RSSI_PAIRING_STATE_IDLE)
    {
        /* Attempt to connect to device with highest RSSI */
        if (!BdaddrIsZero(&rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
        {
            /* Check if RSSI peak is sufficently higher than next */
            if (BdaddrIsZero(&rssi_pairing_data.inquiry_results[1].tp_addr.taddr.addr) ||
                (rssi_pairing_data.inquiry_results[0].rssi - rssi_pairing_data.inquiry_results[1].rssi) >= rssi_pairing_data.scan_rssi_gap)
            {
                DEBUG_LOG_VERBOSE("RSSI Pairing: Pairing with Highest RSSI: bdaddr 0x%04x 0x%02x 0x%06lx",
                               rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr.nap,
                               rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr.uap,
                               rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr.lap);

                if (rssiPairing_StopLeScanIfReqd(rssiPairing_IsScanForDualMode(rssi_pairing_data.mode)))
                {
                    return;
                }

                /* Create an ACL with the device before pairing */
                ConManagerCreateAcl(&rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr);

                rssi_pairing_data.state = RSSI_PAIRING_STATE_ACL_CONNECTING;
                return;
            }
        }

        /* No viable RSSI candidate has been found. Start Inquiry manager again
         * and decrement repeat count if not zero */
        if (rssi_pairing_data.inquiry_count != 0)
        {
            rssi_pairing_data.inquiry_count--;
            DEBUG_LOG_DEBUG("rssiPairing_HandleInquireManagerScanComplete: No Candidate Found. Scanning again inquiry_count:%d", rssi_pairing_data.inquiry_count);

            rssiPairing_ResetDevices();
            InquiryManager_Start(rssi_pairing_data.inquiry_filter_index);
            return;
        }
    }

    /* Reaching here indicates that inquiry time elapsed and unable to find device to connect */
    if (rssiPairing_StopLeScanIfReqd(rssiPairing_IsScanForDualMode(rssi_pairing_data.mode) && rssi_pairing_data.state != RSSI_PAIRING_STATE_IDLE))
    {
        return;
    }

    /* No RSSI candidate found. Send Failure */
    RSSI_PAIRING_PAIR_CFM_T* confirm_message = PanicUnlessNew(RSSI_PAIRING_PAIR_CFM_T);
    BdaddrSetZero(&confirm_message->bd_addr);
    confirm_message->status = FALSE;
    confirm_message->device_type = RSSI_PAIRING_DEVICE_TYPE_UNKNOWN;
    MessageSend(rssi_pairing_data.client_task, RSSI_PAIRING_PAIR_CFM, confirm_message);

    rssiPairing_ResetManager();
}

static void rssiPairing_HandleConManagerTpConnectInd(const CON_MANAGER_TP_CONNECT_IND_T *message)
{
    TRANSPORT_T transport = message->tpaddr.transport;
    const typed_bdaddr *taddr = &message->tpaddr.taddr;

    if (rssi_pairing_data.state == RSSI_PAIRING_STATE_ACL_CONNECTING &&
        BdaddrIsSame(&taddr->addr, &rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
    {
        DEBUG_LOG_DEBUG("rssiPairing_HandleConManagerTpConnectInd");

        /* If the ACL was successfully created to the candidate device then the pairing module will be used
           to pair with the device */
        if (transport == TRANSPORT_BREDR_ACL)
        {
            Pairing_PairAddress(RssiPairing_GetTask(), &rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr);
        }
        else
        {
            Pairing_PairLeAddressAsMaster(RssiPairing_GetTask(), &rssi_pairing_data.inquiry_results[0].tp_addr.taddr);
        }
        rssi_pairing_data.state = RSSI_PAIRING_STATE_PAIRING;
    }
}

static void rssiPairing_HandleConManagerTpDisconnectInd(const CON_MANAGER_TP_CONNECT_IND_T *message)
{
    TRANSPORT_T transport = message->tpaddr.transport;
    const typed_bdaddr *taddr = &message->tpaddr.taddr;

    if (rssi_pairing_data.state == RSSI_PAIRING_STATE_ACL_CONNECTING &&
        BdaddrIsSame(&taddr->addr, &rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr) &&
        transport == TRANSPORT_BLE_ACL)
    {
        DEBUG_LOG_DEBUG("rssiPairing_HandleConManagerTpDisconnectInd");

        rssiPairing_SendPairingFailureAndReset(TRUE);
    }
}

static void rssiPairing_SendPairingCfm(bdaddr device_bd_addr, rssi_pairing_device_type_t device_type, bool status)
{
    RSSI_PAIRING_PAIR_CFM_T* confirm_message = PanicUnlessNew(RSSI_PAIRING_PAIR_CFM_T);
    confirm_message->bd_addr = device_bd_addr;
    confirm_message->device_type = device_type;
    confirm_message->status = status;
    MessageSend(rssi_pairing_data.client_task, RSSI_PAIRING_PAIR_CFM, confirm_message);
}

static void rssiPairing_SendPairingFailureAndReset(bool update_clients)
{
    bdaddr empty_addr;

    if (update_clients)
    {
        BdaddrSetZero(&empty_addr);
        rssiPairing_SendPairingCfm(empty_addr, RSSI_PAIRING_DEVICE_TYPE_UNKNOWN, FALSE);
    }

    rssiPairing_ResetManager();
}

/*! \brief Handler for PAIRING_STOP_CFM message */
static void rssiPairing_HandlePairingStopped(const PAIRING_STOP_CFM_T *message)
{
    DEBUG_LOG_FN_ENTRY("rssiPairing_HandlePairingStopped: status: enum:pairingStatus:%d", message->status);
}

#ifdef ENABLE_LE_RSSI_PAIRING

static bool rssiPairing_StopLeScanIfReqd(bool stop_cond)
{
    if (stop_cond)
    {
        /* If in dual mode, stop the LE scan */
        LeScanManager_Stop(RssiPairing_GetTask());
        return TRUE;
    }

    return FALSE;
}

/*! \brief Store the given BREDR devices in a separate BREDR list.
           In this list, we will capture upto RSSI_PAIRING_LE_CAPABLE_BREDR_DEVICES_LIST_SIZE_MAX devices
           found during inquiry scan and have LE support. If a new device arrives and the list
           is already full, the element which have least RSSI will be getting replaced.

    \param bdaddr The bluetooth address of device to check
    \param rssi RSSI of the device to store
 */
static void rssiPairing_SaveDeviceInLeCapabaleBredrList(const bdaddr *bd_addr, int16 rssi)
{
    int16 lowest_rssi = 1; /* Initialised with a max. value possible for RSSI */
    rssi_paring_device_candidate_t *device, *end_device, *device_to_update = NULL;

    for (device = rssi_pairing_data.le_capable_bredr_devices_list,
            end_device = &device[rssi_pairing_data.le_capable_bredr_devices_list_count];
         device < end_device;
         device++)
    {
        /* Check if same bd address is already there in the list */
        if (BdaddrIsSame(&device->tp_addr.taddr.addr, bd_addr))
        {
            /* Just update the RSSI if it is better value */
            if (rssi > device->rssi)
            {
                device->rssi = rssi;
            }

            return;
        }

        /* Find the lowest RSSI in the list and store its index so that we can replace this element later if needed */
        if (device->rssi < lowest_rssi)
        {
            lowest_rssi = device->rssi;
            device_to_update = device;
        }
    }

    /* Reaching here indicates that the bd address is not in the list already */
    if (rssi_pairing_data.le_capable_bredr_devices_list_count < RSSI_PAIRING_LE_CAPABLE_BREDR_DEVICES_LIST_SIZE_MAX)
    {
        device_to_update = device;
        rssi_pairing_data.le_capable_bredr_devices_list_count++;
    }
    else if (rssi <= lowest_rssi)
    {
        /* If new device have doesn't have better rssi than the lowest, ignore the device. */
        return;
    }

    /* Update the entry */
    device_to_update->rssi = rssi;
    device_to_update->tp_addr.taddr.type = TYPED_BDADDR_PUBLIC;
    device_to_update->tp_addr.transport = TRANSPORT_BREDR_ACL;
    device_to_update->tp_addr.taddr.addr = *bd_addr;
}

/*! \brief Iterates over the LE capable BREDR list and see if the given address exists

    \param bdaddr The bluetooth address of device to check

    \return TRUE if address supports BREDR, FALSE otherwise
 */
static bool rssiPairing_IsGivenDeviceSupportsBredrAndLe(const bdaddr *bd_addr)
{
    rssi_paring_device_candidate_t *device, *end_device;
    bool status = FALSE;

    for (device = rssi_pairing_data.le_capable_bredr_devices_list,
            end_device = &device[rssi_pairing_data.le_capable_bredr_devices_list_count];
         device < end_device;
         device++)
    {
         /* Check if same bd address is there in the list */
        if (BdaddrIsSame(&device->tp_addr.taddr.addr, bd_addr))
        {
            status = TRUE;
            break;
        }
    }

    DEBUG_LOG_VERBOSE("rssiPairing_IsGivenDeviceSupportsBredrAndLe status %d", status);

    return status;
}

static void rssiPairing_HandleLeScanManagerStartCfm(const LE_SCAN_MANAGER_START_CFM_T *cfm)
{
    if (cfm->status == LE_SCAN_MANAGER_RESULT_FAILURE)
    {
        DEBUG_LOG_VERBOSE("rssiPairing_LeScanManagerStartCfm LE Scan Start failed");
        Panic();
    }
}

static void rssiPairing_StopScanningIfDeviceFound(void)
{
    if (rssi_pairing_data.state == RSSI_PAIRING_STATE_DEVICE_SCAN)
    {
        /* Is there a device to connect with highest RSSI? If yes and the device is not already paired, attempt to connect */
        if (!BdaddrIsZero(&rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
        {
            /* If so, is the RSSI peak is sufficently higher than next? */
            if (BdaddrIsZero(&rssi_pairing_data.inquiry_results[1].tp_addr.taddr.addr) ||
               (rssi_pairing_data.inquiry_results[0].rssi - rssi_pairing_data.inquiry_results[1].rssi) >= rssi_pairing_data.scan_rssi_gap)
            {
                /* We have found a device to connect. Stop LE Scanning first */
                LeScanManager_Stop(RssiPairing_GetTask());
                return;
            }
        }

        /* No viable RSSI candidate has been found yet */
        if (rssi_pairing_data.inquiry_count != 0)
        {
            DEBUG_LOG_VERBOSE("rssiPairing_StopScanningIfDeviceFound: No device found, Continuing Scan...");
            rssi_pairing_data.inquiry_count--;

            rssiPairing_ResetDevices();
            MessageSendLater(RssiPairing_GetTask(),
                             RSSI_PAIRING_INTERNAL_STOP_SCAN_IF_DEVICE_FOUND,
                             NULL,
                             RSSI_PAIRING_LE_SCAN_DURATION_MS);
            return;
        }
        else
        {
            DEBUG_LOG_VERBOSE("rssiPairing_StopScanningIfDeviceFound: Unable to find devices to connect");
            LeScanManager_Stop(RssiPairing_GetTask());
        }
    }
}

static void rssiPairing_HandleLeScanManagerStopCfm(const LE_SCAN_MANAGER_STOP_CFM_T *message)
{
    bool update_clients = FALSE;

    DEBUG_LOG_VERBOSE("rssiPairing_HandleLeScanManagerStopCfm Result: %d", message->status);

    if (rssi_pairing_data.state == RSSI_PAIRING_STATE_DEVICE_SCAN)
    {
        update_clients = TRUE;

        /* Attempt to connect to device with highest RSSI */
        if (!BdaddrIsZero(&rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
        {
            /* Check if RSSI peak is sufficently higher than next */
            if (BdaddrIsZero(&rssi_pairing_data.inquiry_results[1].tp_addr.taddr.addr) ||
               (rssi_pairing_data.inquiry_results[0].rssi - rssi_pairing_data.inquiry_results[1].rssi) >= rssi_pairing_data.scan_rssi_gap)
            {
                DEBUG_LOG_VERBOSE("rssiPairing_HandleLeScanManagerStopCfm: Pairing with LAP: 0x%06lx", rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr.lap);

                /* Connection should be over BREDR transport if mode is RSSI_PAIRING_MODE_DUAL_PREF_BREDR and
                   device supports both the transport */
                if (rssiPairing_IsScanForDualPrefBredr(rssi_pairing_data.mode) &&
                    rssiPairing_IsGivenDeviceSupportsBredrAndLe(&rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
                {
                    /* Update the transport to BREDR as preferred connection is BREDR */
                    rssi_pairing_data.inquiry_results[0].tp_addr.transport = TRANSPORT_BREDR_ACL;
                }

                /* Create an ACL with the device */
                ConManagerCreateTpAcl(&rssi_pairing_data.inquiry_results[0].tp_addr);
                rssi_pairing_data.state = RSSI_PAIRING_STATE_ACL_CONNECTING;
                return;
            }
        }
    }

    DEBUG_LOG_VERBOSE("rssiPairing_HandleLeScanManagerStopCfm: Unable to find devices to connect");

    /* Reaching here indicates that RSSI stop scan is called or unable to find the device */
    if (rssiPairing_IsScanForDualMode(rssi_pairing_data.mode) &&
        InquiryManager_IsInquiryActive())
    {
        /* In dual mode, stop the inquiry scan also before sending pairing failure indication */
        InquiryManager_Stop();
        return;
    }

    /* No RSSI candidate found */
    rssiPairing_SendPairingFailureAndReset(update_clients);
}

static void rssiPairing_HandleExtendedAdvertReportInd(const LE_SCAN_MANAGER_EXTENDED_ADV_REPORT_IND_T *ind)
{
    tp_bdaddr tp_addr;

    if (rssi_pairing_data.state == RSSI_PAIRING_STATE_DEVICE_SCAN)
    {
        DEBUG_LOG_DEBUG("bdaddr 0x%06lx, RSSI %d, Threshold %d", ind->current_addr.addr.lap, ind->rssi, rssi_pairing_data.scan_rssi_threshold);

        /* if the rssi result is less than the set threshold discard the result. */
        if (ind->rssi < rssi_pairing_data.scan_rssi_threshold)
        {
            return;
        }

        /* Ignore the report if the device is already connected */
        tp_addr.transport = TRANSPORT_BLE_ACL;
        tp_addr.taddr = ind->permanent_addr;
        if (ConManagerIsTpConnected(&tp_addr))
        {
            return;
        }

        if (rssi_pairing_data.adv_hook_cb != NULL &&
            !rssi_pairing_data.adv_hook_cb(ind->adv_data, ind->adv_data_len))
        {
            /* This advert report did not got qualified for some other reasons
             * imposed by the client.so do not process this advert report.
             */
            return;
        }

        /* Update the candidate list if received RSSI is better than the current candidate list */
        if (BdaddrIsZero(&rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr) ||
            ind->rssi > rssi_pairing_data.inquiry_results[0].rssi)
        {
            /* Check if address is different from previous peak */
            if (!BdaddrIsSame(&ind->current_addr.addr, &rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
            {
                /* Store previous peak RSSI */
                rssi_pairing_data.inquiry_results[1] = rssi_pairing_data.inquiry_results[0];

                /* Store new address */
                rssi_pairing_data.inquiry_results[0].tp_addr.taddr = ind->current_addr;
                rssi_pairing_data.inquiry_results[0].tp_addr.transport = TRANSPORT_BLE_ACL;
                rssi_pairing_data.inquiry_results[0].rssi = ind->rssi;
            }
            else
            {
                /* A better RSSI value received from the same device. Just update the RSSI. */
                rssi_pairing_data.inquiry_results[0].rssi = ind->rssi;
            }
        }
        else if (BdaddrIsZero(&rssi_pairing_data.inquiry_results[1].tp_addr.taddr.addr)||
                 ind->rssi > rssi_pairing_data.inquiry_results[1].rssi)
        {
            /* Check if address is different from peak */
            if (!BdaddrIsSame(&ind->current_addr.addr, &rssi_pairing_data.inquiry_results[0].tp_addr.taddr.addr))
            {
                /* Store next highest RSSI */
                rssi_pairing_data.inquiry_results[1].tp_addr.taddr = ind->current_addr;
                rssi_pairing_data.inquiry_results[1].tp_addr.transport = TRANSPORT_BLE_ACL;
                rssi_pairing_data.inquiry_results[1].rssi = ind->rssi;
            }
        }
    }
}

/*! \brief Get the type of device paired based on the transport and whether it suppors both LE and BREDR
    \return Type of device that got paired. ie, LE/BREDR/dual mode
*/
static rssi_pairing_device_type_t rssiPairing_GetPairedDeviceType(const bdaddr *bd_addr)
{
    rssi_pairing_device_type_t paired_device_type;

    bool is_connected_transport_bredr = (rssi_pairing_data.inquiry_results[0].tp_addr.transport == TRANSPORT_BREDR_ACL);

    if (rssiPairing_IsGivenDeviceSupportsBredrAndLe(bd_addr))
    {
        paired_device_type = RSSI_PAIRING_DEVICE_TYPE_DUAL;
    }
    else
    {
        paired_device_type = is_connected_transport_bredr ? RSSI_PAIRING_DEVICE_TYPE_BREDR : RSSI_PAIRING_DEVICE_TYPE_LE;
    }

    DEBUG_LOG_VERBOSE("rssiPairing_GetPairedDeviceType enum:rssi_pairing_device_type_t:%d", paired_device_type);

    return paired_device_type;
}

#endif /* ENABLE_LE_RSSI_PAIRING */

/*! \brief Handler for the PAIRING_PAIR_CFM message
           If the pairing was successful, a RSSI_PAIRING_PAIR_CFM is sent to the client task.

    \param message the Pairing Manager Pair confirmation
*/
static void rssiPairing_HandlePairingConfirm(const PAIRING_PAIR_CFM_T * message)
{
    DEBUG_LOG_FN_ENTRY("rssiPairing_HandlePairingConfirm status: enum:pairingStatus:%d", message->status);

    switch (message->status){
        case pairingSuccess:
        {
            DEBUG_LOG_VERBOSE("RSSI Pairing: Pairing Successful, bdaddr 0x%04x 0x%02x 0x%06lx",
                           message->device_bd_addr.nap,
                           message->device_bd_addr.uap,
                           message->device_bd_addr.lap);
           rssiPairing_SendPairingCfm(message->device_bd_addr, rssiPairing_GetPairedDeviceType(&message->device_bd_addr), TRUE);
        }
        break;
        default:
        {
            rssiPairing_SendPairingCfm(message->device_bd_addr, rssiPairing_GetPairedDeviceType(&message->device_bd_addr), FALSE);
        }
        break;
    }

    /* Release the connection ACL Ownership */
    ConManagerReleaseTpAcl(&rssi_pairing_data.inquiry_results[0].tp_addr);

    rssiPairing_ResetManager();
}

/*! \brief Handler for component messages */
static void rssiPairing_HandleMessage(Task task, MessageId id, Message message)
{
    UNUSED(task);

    switch (id)
    {
        case CON_MANAGER_TP_CONNECT_IND:
            rssiPairing_HandleConManagerTpConnectInd((const CON_MANAGER_TP_CONNECT_IND_T *) message);
        break;

        case CON_MANAGER_TP_DISCONNECT_IND:
            rssiPairing_HandleConManagerTpDisconnectInd((const CON_MANAGER_TP_CONNECT_IND_T *) message);
        break;

        case INQUIRY_MANAGER_RESULT:
            rssiPairing_HandleInquireManagerResult((INQUIRY_MANAGER_RESULT_T *)message);
        break;

        case INQUIRY_MANAGER_SCAN_COMPLETE:
            rssiPairing_HandleInquireManagerScanComplete();
        break;

        case PAIRING_PAIR_CFM:
            rssiPairing_HandlePairingConfirm((PAIRING_PAIR_CFM_T *)message);
        break;

        case PAIRING_STOP_CFM:
            rssiPairing_HandlePairingStopped((PAIRING_STOP_CFM_T *)message);
        break;

#ifdef ENABLE_LE_RSSI_PAIRING
        case LE_SCAN_MANAGER_START_CFM:
            rssiPairing_HandleLeScanManagerStartCfm((LE_SCAN_MANAGER_START_CFM_T *) message);
        return;

        case LE_SCAN_MANAGER_STOP_CFM:
            rssiPairing_HandleLeScanManagerStopCfm((LE_SCAN_MANAGER_STOP_CFM_T *) message);
        return;

        case RSSI_PAIRING_INTERNAL_STOP_SCAN_IF_DEVICE_FOUND:
            rssiPairing_StopScanningIfDeviceFound();
        break;

        case LE_SCAN_MANAGER_EXT_SCAN_FILTERED_ADV_REPORT_IND:
            rssiPairing_HandleExtendedAdvertReportInd((const LE_SCAN_MANAGER_EXTENDED_ADV_REPORT_IND_T *) message);
        break;
#endif /* ENABLE_LE_RSSI_PAIRING */

        default:
            DEBUG_LOG_FN_ENTRY("rssiPairing_HandleMessage Unhandled message %d", id);
            UnexpectedMessage_HandleMessage(id);
        break;
    }
}

bool RssiPairing_Init(Task init_task)
{
    UNUSED(init_task);
    DEBUG_LOG_FN_ENTRY("RssiPairing_Init");

    rssi_pairing_data.task.handler = rssiPairing_HandleMessage;

    rssiPairing_ResetManager();
    InquiryManager_ClientRegister(RssiPairing_GetTask());

    return TRUE;
}

bool RssiPairing_Start(Task client_task,
                       rssi_pairing_mode_t mode,
                       const rssi_pairing_parameters_t *scan_parameters,
                       const rssi_pairing_le_scan_parameters_t *le_parameters)
{
    if (rssi_pairing_data.state != RSSI_PAIRING_STATE_IDLE || scan_parameters == NULL ||
        !scan_parameters->inquiry_count)
    {
        DEBUG_LOG_ERROR("RssiPairing_Start Cannot Start, state %d inquiry_count %d",
                         rssi_pairing_data.state, scan_parameters->inquiry_count);
        return FALSE;
    }

#ifdef ENABLE_LE_RSSI_PAIRING
    if (rssiPairing_IsScanForLe(mode) && (le_parameters == NULL || le_parameters->filter_param == NULL))
    {
        DEBUG_LOG_ERROR("RssiPairing_Start: Not enough LE Params");
        return FALSE;
    }
#endif

    rssiPairing_ResetManager();
    ConManagerRegisterTpConnectionsObserver(cm_transport_all, RssiPairing_GetTask());

    /* Store all received parameters */
    rssi_pairing_data.client_task = client_task;
    rssi_pairing_data.scan_rssi_gap = scan_parameters->rssi_gap;
    rssi_pairing_data.scan_rssi_threshold = scan_parameters->rssi_threshold;
    rssi_pairing_data.inquiry_count = scan_parameters->inquiry_count;
    rssi_pairing_data.inquiry_filter_index = scan_parameters->inquiry_filter;
    rssi_pairing_data.mode = mode;

    rssi_pairing_data.inquiry_count--;

    /* Start BREDR Scanning if it is requested */
    if (rssiPairing_IsScanForBredr(mode))
    {
        /* Start the first inquiry scan */
        if (!InquiryManager_Start(scan_parameters->inquiry_filter))
        {
            DEBUG_LOG_DEBUG("RssiPairing_Start failed on inquiry start");
            return FALSE;
        }
    }

#ifdef ENABLE_LE_RSSI_PAIRING

    if (rssiPairing_IsScanForDualMode(mode))
    {
        rssi_pairing_data.le_capable_bredr_devices_list = PanicUnlessMalloc(sizeof(*rssi_pairing_data.le_capable_bredr_devices_list) *
            RSSI_PAIRING_LE_CAPABLE_BREDR_DEVICES_LIST_SIZE_MAX);
    }

    /* Start LE Scanning if it is requested */
    if (rssiPairing_IsScanForLe(mode))
    {
        /* Store LE scan related parameters */
        rssi_pairing_data.adv_hook_cb = le_parameters->cb;
        rssi_pairing_data.cod_filter = le_parameters->cod_filter;

        LeScanManager_StartExtendedScan(RssiPairing_GetTask(), le_parameters->filter_param);
    }

    if (rssiPairing_IsScanForLeOnly(mode))
    {
        /* LE only scan is continuous, queue a message to check for candidates */
        MessageSendLater(RssiPairing_GetTask(),
                         RSSI_PAIRING_INTERNAL_STOP_SCAN_IF_DEVICE_FOUND,
                         NULL,
                         RSSI_PAIRING_LE_SCAN_DURATION_MS);
    }

#else /* ENABLE_LE_RSSI_PAIRING */

    UNUSED(le_parameters);

#endif /* ENABLE_LE_RSSI_PAIRING */

    rssi_pairing_data.state = RSSI_PAIRING_STATE_DEVICE_SCAN;

    DEBUG_LOG_DEBUG("RssiPairing_Start started");

    return TRUE;
}

void RssiPairing_Stop(void)
{
    if (rssi_pairing_data.state == RSSI_PAIRING_STATE_DEVICE_SCAN)
    {
        /* Don't reset data yet - still need to send a final RSSI_PAIRING_PAIR_CFM
           to the client_task. Just reset state, then request scan stop */

        (void) rssiPairing_StopLeScanIfReqd(rssiPairing_IsScanForLe(rssi_pairing_data.mode));

        if (rssiPairing_IsScanForBredr(rssi_pairing_data.mode))
        {
            InquiryManager_Stop();
        }

        rssi_pairing_data.state = RSSI_PAIRING_STATE_IDLE;
    }
}

bool RssiPairing_IsActive(void)
{
    return (rssi_pairing_data.state > RSSI_PAIRING_STATE_IDLE);
}
