/****************************************************************************
 * Copyright (c) 2020 - 2020 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  kip_mgr_for_adaptors.h
 * \ingroup kip_mgr
 *
 */

#ifndef KIP_MGR_FOR_ADAPTORS_H
#define KIP_MGR_FOR_ADAPTORS_H

/****************************************************************************
Include Files
*/
#if defined(SUPPORTS_MULTI_CORE)
#include "kip_mgr.h"

/****************************************************************************
Public Function Declarations
*/

/*
 * \brief  Determine whether an auxiliary processor is up-and-running or not.
 *         That is, it verifies whether the processor is in a state
 *         ready to communicate with other processors. If not, it is
 *         either off, or still establishing communications, or in
 *         teardown.
 *
 * \return Return TRUE if proc_id up-and-running, FALSE if not.
 */
static inline bool kip_aux_processor_is_enabled(void)
{
    return kip_aux_processor_has_started(PROC_PROCESSOR_1);
}

/**
 * \brief Disables secondary processor after boot
 *        but before it has been started.
 *
 * \param processor_id Identifier of processor to disable.
 *
 * \return TRUE if successful and FALSE in case of failure.
 */
extern bool kip_aux_processor_disable(PROC_ID_NUM processor_id);

/**
 * \brief Start a secondary processor.
 *
 * \param con_id       Connection identifier to use in the callback.
 * \param processor_id Identifier of processor to start.
 * \param callback     Pointer to a function to be called once the starting
 *                     process is completed.
 */
extern void kip_aux_processor_start(CONNECTION_LINK con_id,
                                    PROC_ID_NUM processor_id,
                                    KIP_STATUS_CBACK callback);

/**
 * \brief Stop a secondary processor.
 *
 * \param con_id       Connection identifier to use in the callback.
 * \param processor_id Identifier of processor to stop.
 * \param callback     Pointer to a function to be called once the stopping
 *                     process is completed.
 */
extern void kip_aux_processor_stop(CONNECTION_LINK con_id,
                                   PROC_ID_NUM processor_id,
                                   KIP_STATUS_CBACK callback);
#else
#define kip_aux_processor_is_enabled()    FALSE
#endif /* SUPPORTS_MULTI_CORE */

#endif /* KIP_MGR_FOR_ADAPTORS_H */
