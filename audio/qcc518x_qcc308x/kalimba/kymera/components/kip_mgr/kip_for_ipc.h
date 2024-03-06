/*****************************************************************************
*
* Copyright (c) 2020 Qualcomm Technologies International, Ltd.
*
*****************************************************************************/
/** \file kip_for_ipc.h
 *
 *  semi-private header file since KIP and IPC have a "special relationship" .
 *
 */

#ifndef KIP_FOR_IPC_H
#define KIP_FOR_IPC_H

/****************************************************************************
Include Files
*/

/****************************************************************************
Public Type Declarations
*/

/****************************************************************************
Public Constant and macros
*/

/****************************************************************************
Public Variable Declarations
*/

/****************************************************************************
Public Function Declarations
*/


/**
 * \brief  IPC call back to KIP.
 *
 * \param[in] ipc_event - the event IPC is reporting back about.
 * \param[in] params    - pointer to a table of parameters providing details of specific event.
 *
 * \return  IPC_SUCCESS if the IPC event was successful.
 */
extern IPC_STATUS kip_if_callback(IPC_EVENT ipc_event, void *params);

#endif /* KIP_FOR_IPC_H */
