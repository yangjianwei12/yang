/*!
\copyright  Copyright (c) 2022 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       anc_trigger_manager.h
\brief      ANC Trigger Manager: Handle various types of triggers (Wind, Noise, Quiet) and manage attack and release with priority.
*/


#ifndef ANC_TRIGGER_MANAGER_H
#define ANC_TRIGGER_MANAGER_H

/*!
 * Trigger type
 */
typedef enum
{  
   ANC_TRIGGER_TYPE_INVALID              = 0,
   ANC_TRIGGER_TYPE_WIND_ATTACK          = 1,
   ANC_TRIGGER_TYPE_QUIET_MODE_ENABLE    = 2,
   ANC_TRIGGER_TYPE_NOISE_ID_CAT_CHANGE  = 3
   
} anc_trigger_type_t;

/*!
 * Trigger priorities
 */
typedef enum
{  
   ANC_TRIGGER_PRIORITY_INVALID = 0,
   ANC_TRIGGER_PRIORITY_HIGHEST = 1,
   ANC_TRIGGER_PRIORITY_MEDIUM  = 2,   
   ANC_TRIGGER_PRIORITY_LOW     = 3
   
} anc_trigger_priority_t;


/*!
    \brief Invoke Trigger Manager to decide which trigger to process based on priority
    \param mode   Trigger type
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncTriggerManager_Invoke(anc_trigger_type_t trigger_type);
#else
#define AncTriggerManager_Invoke(trigger_type) ((void)(0 * (trigger_type)))
#endif

/*!
    \brief Invoke Action post release of high priority triggers
    \param mode   None
*/
#ifdef ENABLE_ADAPTIVE_ANC
void AncTriggerManager_ActionPostRelease(void);
#else
#define AncTriggerManager_ActionPostRelease() ((void)(0))
#endif

#endif /* ANC_TRIGGER_MANAGER_H */
