/*!
    \copyright  Copyright (c) 2021 - 2023 Qualcomm Technologies International, Ltd.\n
                All Rights Reserved.\n
                Qualcomm Technologies International, Ltd. Confidential and Proprietary.
    \file       
    \addtogroup state_of_charge
    \brief      SoC internal data
    @{
*/

#ifndef STATE_OF_CHARGE_PRIVATE_H_
#define STATE_OF_CHARGE_PRIVATE_H_

#include "state_of_charge.h"
#include <task_list.h>

#define BATTERY_STATE_OF_CHARGE_KEY  (11)
#define SOC_MIN_INDEX 0

/*! Structure used internally to the soc module to store per-client state */
typedef struct soc_registered_client_item
{
    /*! The next client in the list */
    struct soc_registered_client_item *next;
    /*! The client's registration information */
    soc_registration_form_t form;    

    /*! The last percentage sent to the client */
    uint8 percent;    
} socRegisteredClient;

typedef struct
{
    uint16 state_of_charge;
	uint8 charger_connected;
    uint8 config_index;
    uint8 low_battery_percent;  /* Level below which the battery is "low", default 30 */
    uint8 full_battery_percent; /* Level above which the battery is "full", default 90 */
    /*! SoC task */
    TaskData         task;

    /*! A linked-list of clients */
    socRegisteredClient *client_list;     
    unsigned soc_context;
} soc_data_t;

/*! \brief Battery charge component task data. */
extern soc_data_t app_battery_charge;
/*! \brief Access the battery charge data. */
#define GetBatteryChargeData()    (&app_battery_charge)

#define SoC_GetClientTasks() (task_list_flexible_t *)(&app_battery_charge.client_tasks_list)
#define SoC_GetTask()        (&app_battery_charge.task) 

#define Soc_LowBatteryPercent (30)
#define Soc_FullBatteryPercent (90)

#endif /* STATE_OF_CHARGE_PRIVATE_H_ */

/*! @} */