/*!
    \copyright  Copyright (c)  2019-2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \defgroup   gatt_server_gap GATT GAP Server
    @{
    \ingroup    gatt_servers_domain
    \brief      Header file for the GAP Server module.

    Component that deals with initialising the gatt_gap_server library, 
    and deals with messages sent from this library.
    Also responsible for LE advertising of the local name.

*/

#ifndef GATT_SERVER_GAP_H_
#define GATT_SERVER_GAP_H_


/*! \brief Device appearance is of type Earbud */
#define GATT_SERVER_GAP_BT_APPEARANCE_EARBUD          (0x0941)

/*! \brief Device appearance is of type Headset */
#define GATT_SERVER_GAP_BT_APPEARANCE_HEADSET         (0x0942)

/*! \brief Device appearance is of type Unknown */
#define GATT_SERVER_GAP_BT_APPEARANCE_UNKNOWN         (0x00)

/*! \brief Initialise the GAP Server.

    \param init_task    Task to send init completion message to

    \returns TRUE
*/
bool GattServerGap_Init(Task init_task);

/*! \brief Tells the GAP server to use the complete local name, or if it can be shortened.

    The GAP server handles the LE advertising of local name which can be shortened depending on the
    space in the advertising data.
    This function can ensure the complete local name is always supplied in the advertising data.
    
    \param complete    Set to TRUE so the complete local name is always used. Otherwise can be shortened.
*/
void GattServerGap_UseCompleteLocalName(bool complete);

/*! \brief Check whether server is configured to advertise complete name.

    \returns TRUE if complete local name is to be advertised, FALSE if it is to be shortened
*/
bool GattServerGap_IsCompleteLocalNameBeingUsed(void);

/*! \brief Set the appearance value for this device. */
void GattServerGap_SetAppearanceValue(uint16 appearance_value);

/*! \brief Get the  appearance value for this device.

    \returns The appearance value of the device.
*/
uint16 GattServerGap_GetAppearanceValue(void);

#endif /* GATT_SERVER_GAP_H_ */
/*! @} */