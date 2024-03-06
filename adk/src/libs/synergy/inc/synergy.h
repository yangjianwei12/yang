/*!
\copyright  Copyright (c) 2020-2023 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file
\brief      Synergy entry function prototypes and data.
*/

#ifndef SYNERGY_H_
#define SYNERGY_H_

#include "csr_synergy.h"
#include "csr_sched.h"
#include "csr_bt_profiles.h"
#include <trapsets.h>
#include <message.h>

#ifndef CL_MESSAGE_BASE
#define CL_MESSAGE_BASE                     0x5000
#endif
#define SYNERGY_EVENT_BASE                  (CL_MESSAGE_BASE - CSR_BT_CPL_PRIM_BASE)

/*! \brief Enable LE Audio Tasks and Message handlers based on the selected library configuration.
           Eg: 1. For sink based devices it enables Unicast server and Broadcast sink functionalities.
               2. For source based devices it enables Unicast client, Broadcast Source and
                  Broadcast assistant functionalities.

           This API should be called before initializing the Synergy's task scheduler using SynergyInit().

    \return None.
 
    \note Application can selectively use individual tasking functionalities also based on the requirement. 
          Eg: If application requirement is just to use Unicast server functionalities then it can invoke
              only SynergyEnableLEAUnicastServerTasks()
 */
void SynergyEnableLEATasks(void);


/*! \brief Enable only LE Audio Broadcast Source role Tasks and Message handlers.
           If Synergy's LE Audio Broadcast Source role Profiles are required,
           this API should be called, before initializing the Synergy's task scheduler using SynergyInit().
 
    \return None.
 */
void SynergyEnableLEABroadcastSourceTask(void);

/*! \brief Enable only LE Audio Unicast Server role Tasks and Message handlers.
           If Synergy's LE Audio Unicast Server role Profiles are required,
           this API should be called, before initializing the Synergy's task scheduler using SynergyInit().
 
    \return None.
 */
void SynergyEnableLEAUnicastServerTasks(void);

/*! \brief Enable only LE Audio Unicast Client role Tasks and Message handlers.
           If Synergy's LE Audio Unicast Client role Profiles are required,
           this API should be called, before initializing the Synergy's task scheduler using SynergyInit().
 
    \return None.
 */
void SynergyEnableLEAUnicastClientTasks(void);

/*! \brief Enable only LE Audio Broadcast Assistant role Tasks and Message handlers.
           If Synergy's LE Audio Broadcast Assistant role Profiles are required,
           this API should be called, before initializing the Synergy's task scheduler using SynergyInit().
 
    \return None.
 */
void SynergyEnableLEABroadcastAssistantTasks(void);

/*! \brief Enable only LE Audio Broadcast Sink role Tasks and Message handlers.
           If Synergy's LE Audio Broadcast Sink role Profiles are required,
           this API should be called, before initializing the Synergy's task scheduler using SynergyInit().
 
    \return None.
 */
void SynergyEnableLEABroadcastSinkTasks(void);

/*! \brief Enable HIDD Task. If Synergy's HIDD Profile is required, this API should be called, before
           initializing the Synergy's task scheduler using SynergyInit().

    \return None.
 */
void SynergyEnableHIDDTask(void);

/*! \brief Enable SPP multi-instance Tasks and Message handlers. If Synergy's SPP profile multiple instances are required,
           this API should be called, before initializing the Synergy's task scheduler using SynergyInit().

    \return None.
 */
void SynergyEnableSPPMultiInstanceTasks(void);
/*! \brief Initialize Synergy's default tasks and task scheduler. Some (Non-default) tasks such as LEA, HIDD needs to be
           enabled using their respective APIs prior to invoking this API.

    \param numTdlDevices Number of TDL devices

    \return None.
 */

#ifndef EXCLUDE_CSR_BT_HIDS_SERVER_MODULE
/*! \brief Enable HIDS Task. If Synergy's HIDS Service is required, this API should be called, before
           initializing the Synergy's task scheduler using SynergyInit().
    \return None.
 */
void SynergyEnableHIDSTask(void);
#endif

void SynergyInit(uint16 numTdlDevices);

/*! \brief Obtains an external task ID to use for posting message to trap tasks.

    \param task Destination task.

    \return External queue ID to use with trap messaging interfaces.

    \note This trap may be called from a high-priority task handler
 */
uint16 SynergyAdapterTaskIdGet(Task task);

/*! \brief Converts trap task to a queue-id

    \param task Destination task.

    \return External queue ID to use with trap messaging interfaces.
 */
#define TrapToOxygenTask                    SynergyAdapterTaskIdGet

/*! \brief Unregisters an external task ID from the trap task list.

    \param task Destination task.

    \return TRUE if task has been unregistered, FALSE if task in not there in trap task list.
 */
bool SynergyAdapterTaskUnregister(Task task);

/*! \brief Frees synergy upstream messages

    \param _event Event ID.
    \param _msg Message.

    \return NULL if message is freed else message itself.
 */
#define SynergyFreeUpstreamMessageContents(_event, _msg)                    \
    CsrBtFreeUpstreamMessageContents((_event) - SYNERGY_EVENT_BASE, _msg)

#endif /* SYNERGY_H_ */
