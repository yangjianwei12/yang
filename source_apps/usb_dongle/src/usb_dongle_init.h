/*!
\copyright  Copyright (c) 2021 - 2022 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Header file for initialisation module
*/


#ifndef USB_DONGLE_INIT_H
#define USB_DONGLE_INIT_H

 /*! \brief Initialisation module data */
typedef struct
{
    TaskData task;              /*!< Init's local task */
} initData;

/*!< Structure used while initialising */
extern initData    usb_dongle_init;

/*! Get pointer to init data structure */
#define InitGetTaskData()        (&usb_dongle_init)


/*! \brief Initialise init module
    This function is the start of all initialisation.

    When the initialisation sequence has completed #INIT_CFM will
    be sent to the main app Task.
*/
void UsbDongleInit_StartInitialisation(void);


/*! Let the application initialisation function know that initialisation is complete

    This should be called on receipt of the #APPS_COMMON_INIT_CFM message.
 */
void UsbDongleInit_CompleteInitialisation(void);


#endif /* USB_DONGLE_INIT_H */

