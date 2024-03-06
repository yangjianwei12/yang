/****************************************************************************
 * Copyright (c) 2011 - 2019 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \defgroup HAL Hardware Abstraction Layer
 */
/**
 * \file hal_audio_aov.h
 * \ingroup HAL
 *
 * Public header file for HAL_AUDIO_AOV functions.
 * Currently just initialisation
 */

#ifndef HAL_AUDIO_AOV_H
#define HAL_AUDIO_AOV_H

/****************************************************************************
Private Type Declarations
*/

/**
 * Function type to handle an AoV EoD interrupt. This is handler will be
 * provided when EoD is enabled.
 */
typedef void (*AOV_EOD_INT_HANDLER)(void* interrupt_data);

/****************************************************************************
Private Constant Declarations
*/

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief Enable the individual AoV EoD for the given instance
 *
 * \param   instance AoV instance to enable
 * \param   handler  Monitor specific interrupt handler
 * \param   interrupt_data interrupt specific data passed to the handler
 *
 * \return  False if instance is not free.
 */
extern bool hal_aov_eod_enable(unsigned int instance,
                        AOV_EOD_INT_HANDLER handler,
                        void *interrupt_data);
/**
 * \brief Disable the individual AoV EoD for the given instance
 *
 * \param instance AoV instance to disable
 *
 * \return  False if invalid instance.
 */
extern bool hal_aov_eod_disable(unsigned int instance);

/**
 * \brief Trigger an on-demand transfer for the given instance
 *
 * \param   num_instances number of instances
 * \param   instances array of AoV instances
 * \param   amt_to_transfer How much data to transfer from an AoV RAM on demand.
 */
extern void hal_aov_eod_start_transfer(unsigned int num_instances,
                                       unsigned int *instances,
                                       unsigned int amt_to_transfer);

/**
 * \brief Find out how much data is in the Aov Buffer for the given instance
 *
 * \param   instance AoV instance to enable
 *
 * \return  Amount of data in words.
 */
extern unsigned int hal_aov_get_data_in_aov_buffer(unsigned int aov_instance);


#endif /* HAL_AUDIO_AOV_H */