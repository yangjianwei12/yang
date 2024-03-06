/*!
\copyright  Copyright (c) 2020-2021 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       
\brief      Header file for Connection Manager internal data types
*/

#ifndef __CON_MANAGER_DATA_H
#define __CON_MANAGER_DATA_H

#include <connection_manager.h>
#include <le_scan_manager.h>

/*! Connection Manager module task data. */
typedef struct
{
    /*! The task (message) information for the connection manager module */
    TaskData         task;

    /*! Flag indicating if incoming handset connections are allowed */
    bool handset_connect_allowed:1;

    cm_transport_t connectable_transports;
    
    bool  is_le_scan_paused;

    /*! Flag indicating if device is in handset pairing mode */
    bool    handset_pairing_mode;
    
    /*! BT address of handset requested opening ACL first, 0 otherwise */
    bdaddr   handset_to_pair_with_bdaddr;

    /*! Ensure only First ACL open requested handset can be authorised*/
    uint16   handset_authorise_lock;

    /*! List of registered clients to disconnect LE link */
    task_list_t *all_le_disconnect_requester_list;
	
	/*! List of registered client tasks */
    task_list_t *forced_disconnect_requester_list;

    /*! Callback for adjusting connection params */
    con_manager_connparams_callback_t connection_params_adjust_callback;

    /* page timeout in BT SLOTS, set by Handset Service. */
    uint16 page_timeout;
} conManagerTaskData;


/*! Return the task for connection manager */
Task ConManagerGetConManagerTask(void);

conManagerTaskData *ConManagerGetTaskData(void);


#endif
