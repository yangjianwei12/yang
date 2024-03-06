/****************************************************************************
 * Copyright (c) 2021 Qualcomm Technologies International, Ltd.
****************************************************************************/
/**
 * \file  anc_hw_manager_list.h
 * \ingroup anc_hw_manager
 *
 * ANC HW Manager (AHM) definitions for linked lists that describe shared
 * ANC attributes.
 *
 */

#ifndef _ANC_HW_MANAGER_SHARED_H_
#define _ANC_HW_MANAGER_SHARED_H_

/******************************************************************************
Include Files
*/
#include "pmalloc/pl_malloc.h"
#include "opmgr/opmgr_operator_data.h"
#include "anc_hw_manager_gain.h"

/******************************************************************************
Public Definitions
*/

/******************************************************************************
Public Type Declarations
*/

/* Linked list element type for shared coarse gains */
typedef struct _AHM_COARSE_GAIN_NODE
{
    struct _AHM_COARSE_GAIN_NODE *p_next;
    AHM_SHARED_COARSE_GAIN data;
} AHM_COARSE_GAIN_NODE;

/* Linked list element type for shared fine gain */
typedef struct _AHM_FINE_GAIN_NODE
{
    struct _AHM_FINE_GAIN_NODE *p_next;
    AHM_SHARED_FINE_GAIN data;
} AHM_FINE_GAIN_NODE;

/****************************************************************************
Public Function Definitions
*/

/**
 * \brief  Add a fine gain element to the linked list
 *
 * \param  p_head       Pointer to the head of the linked list
 * \param  ext_op_id    Operator id
 * \return - Pointer to the gain struct in the newly allocated element
 *
 */
extern AHM_SHARED_FINE_GAIN* ahm_list_fine_gain_add(AHM_FINE_GAIN_NODE **p_head, EXT_OP_ID ext_op_id);

/**
 * \brief  Remove a fine gain element from the linked list
 *
 * \param  p_head       Pointer to the head of the linked list
 * \param  p_gain       Pointer to the gain value to remove
 * \param  ext_op_id    Operator id
 *
 * \return - None
 *
 */
extern void ahm_list_fine_gain_remove(AHM_FINE_GAIN_NODE **p_head,
                                      AHM_SHARED_FINE_GAIN *p_gain,
                                      EXT_OP_ID ext_op_id);

/**
 * \brief  Destroy a linked list
 *
 * \param  p_head       Pointer to the head of the linked list
 *
 * \return - None
 *
 */
extern void ahm_list_destroy(AHM_FINE_GAIN_NODE **p_head);
#endif /* _ANC_HW_MANAGER_SHARED_H_ */
