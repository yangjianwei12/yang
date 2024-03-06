/*!
    \copyright  Copyright (c) 2019 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file
    \addtogroup gatt_connect
    \brief      Header file for GATT connect observer
    @{
*/

#ifndef GATT_CONNECT_OBSERVER_H_
#define GATT_CONNECT_OBSERVER_H_

/*! @brief Called when a GATT connection occurs.

    \param cid          The GATT connection ID

*/
void GattConnect_ObserverNotifyOnConnection(gatt_cid_t cid);

/*! @brief Called when a GATT disconnection occurs.

    \param cid          The GATT connection ID

*/
void GattConnect_ObserverNotifyOnDisconnection(gatt_cid_t cid);

/*! @brief Called when a GATT disconnect has been requested.

    \param cid          The GATT connection ID
    \param response     The observer will call the response function once internal processing
                        is complete due to a disconnect occuring. By responding, the observer will allow the
                        GATT disconnection to proceed.

*/
void GattConnect_ObserverNotifyOnDisconnectRequested(gatt_cid_t cid, gatt_connect_disconnect_req_response response);

/*! @brief Called when the encryption status changes on a GATT connection.

    \param cid          The GATT connection ID
    \param encrypted    The current state of the encryption on the GATT connection.

*/
void GattConnect_ObserverNotifyOnEncryptionChanged(gatt_cid_t cid, bool encrypted);

/*! @brief Get the number of observers that have registered the 'disconnect requested' callback.

    \return The number of observers that have registered the 'disconnect requested' callback.

*/
unsigned GattConnect_ObserverGetNumberDisconnectReqCallbacksRegistered(void);

/*! @brief Initialise gatt_connect_observer

*/
void GattConnect_ObserverInit(void);

#endif 
/*! @} */