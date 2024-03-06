/*******************************************************************************

Copyright (C) 2018-2022 Qualcomm Technologies International, Ltd.
All Rights Reserved.
Qualcomm Technologies International, Ltd. Confidential and Proprietary.

*******************************************************************************/

/*! \file
 *
 *  \brief BAP interface.
 */

/**
 * \defgroup BAP BAP
 * @{
 */

#ifndef BAP_CLIENT_LIST_UTIL_H_
#define BAP_CLIENT_LIST_UTIL_H_

#include "bap_client_lib.h"
#include "bap_client_list_util_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Send a message to the BAP object when code is executed in a task context.
 *
 *  \param [in] prim A pointer to a BapUPrim structure.
 *
 *  \return Nothing.
 */
void bapClientRcvMessage(BapUPrim * const prim);

/*! \brief Send a message to the BAP object when code is not executed in a task context
 *
 *  \param [in] prim A pointer to a BapUPrim structure.
 *
 *  \return Nothing.
 */
void bapClientRcvIntercontextMessage(BapUPrim * const prim);

/*! \brief Free BAP primitive.
 *
 *  \param [in] prim A pointer to a BapUPrim structure.
 *
 *  \return Nothing.
 */
void bapClientFreePrimitive(BapUPrim * const prim);

/*! \brief get BAP instance.
 *
 *	\return void *
 */

struct BAP* bapGetInstance(void);

#ifdef __cplusplus
}
#endif

#endif /* QBL_BAP_H_ */

/**@}*/
