/*!
    \copyright  Copyright (c) 2015 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \version    
    \file       gatt_handler.h
    \defgroup   gatt_handler    GATT Handler
    @{
    \ingroup    gatt_common_domain
    \brief      Header file for GATT, GATT Server and GAP Server
*/

#ifndef GATT_HANDLER_H_
#define GATT_HANDLER_H_

#include "le_advertising_manager.h"

#define APP_GATT_SERVER_INSTANCES   (1)

typedef enum {
    gatt_server_accessory_tracking,
} gatt_server_t;

/*! Structure holding information for the gatt task */
typedef struct
{
    /*! Task for handling messaging from GATT Manager */
    TaskData    gatt_task;
} gattTaskData;

/*!< App GATT component task */
extern gattTaskData    app_gatt;

/*! Get pointer to the GATT modules task data */
#define GattGetTaskData()                (&app_gatt)

/*! Get pointer to the GATT modules task */
#define GattGetTask()                    (&app_gatt.gatt_task)


/*! @brief Initialise GATT handler

    \param init_task    Not used

    \returns TRUE
*/
bool GattHandlerInit(Task init_task);


/*! @brief Finds the public address from a GATT connection ID.

    \param cid          The connection ID
    \param public_addr  The returned public address that was found from the connection ID.

    \returns TRUE if public address if could be found from the connection ID,
    otherwise FALSE.
*/
bool GattHandler_GetPublicAddressFromConnectId(uint16 cid, bdaddr *public_addr);

/*! @brief Gets the start handle of the given GATT server.

    \param gatt_server  The GATT server

    \returns the start handle of the GATT server.
*/
uint16 GattHandler_GetGattStartHandle(gatt_server_t gatt_server);


/*! @brief Gets the end handle of the given GATT server.

    \param gatt_server  The GATT server

    \returns the end handle of the GATT server.
*/
uint16 GattHandler_GetGattEndHandle(gatt_server_t gatt_server);

#endif /* GATT_HANDLER_H_ */
/*! @} */